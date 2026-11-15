// Ghost Aquarium firmware: owns memory and timing, delegates everything else.
//   lib/tanksim    → the simulation (fixed 60 Hz steps)
//   lib/tankrender → draws the tank into a plain RGB565 framebuffer
//   here           → allocate buffers, step, render, push to the panel, watchdog, stats
// See docs/DESIGN.md §8 and ADR 0008 for the budget and pipeline.

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_random.h>
#include <esp_task_wdt.h>

#include <cstdarg>
#include <cstdio>
#include <new>

#include "background.h"
#include "display/lgfx_config.h"
#include "drift.h"
#include "renderer.h"
#include "stepper.h"
#include "tank.h"

namespace {
constexpr int kW = tank::kScreenW, kH = tank::kScreenH;  // 128×128 (ADR 0010)
constexpr size_t kFrameBytes = kW * kH * sizeof(uint16_t);   // 32 KB
constexpr float kStep = 1.f / 60.f;
constexpr uint32_t kMinFrameUs = 1000000 / 30;  // render at ≤ 30 fps (the SPI push limit)
#ifndef GHOST_MIRROR
#define GHOST_MIRROR 1  // Pepper's Ghost: the reflector flips left/right. -DGHOST_MIRROR=0 for a readable sim view.
#endif
constexpr bool kMirrored = GHOST_MIRROR != 0;
constexpr uint32_t kWatchdogSec = 5;
#ifndef GHOST_BANDED
#define GHOST_BANDED 1  // DMA bands (ADR 0008 §7); -DGHOST_BANDED=0 for the full-frame path
#endif
constexpr int kBandH = 32, kBands = kH / kBandH;
static_assert(kH % kBandH == 0, "bands must tile the frame");
constexpr size_t kBandBytes = kW * kBandH * sizeof(uint16_t);  // 8 KB
constexpr uint32_t kDriftStepMs = 2 * 60 * 1000;  // burn-in drift: 1 px every 2 min (a 96-min cycle)
char logBuf[192];  // stats lines are formatted here: Print::printf mallocs for lines > 64 chars

void logLine(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  const int n = vsnprintf(logBuf, sizeof(logBuf), fmt, args);
  va_end(args);
  if (n > 0) Serial.write(reinterpret_cast<const uint8_t*>(logBuf), n < static_cast<int>(sizeof(logBuf)) ? n : sizeof(logBuf) - 1);
}

LGFX lcd;
uint16_t* bandBuf[2] = {nullptr, nullptr};  // 2 × 8 KB DMA-capable SRAM: draw one while DMA sends the other
uint16_t* frame = nullptr;       // full-frame fallback only (no band buffers)
tank::render::DisplayList displayList;  // ~8 KB, global: built once per frame, rasterised per band
uint16_t* background = nullptr;  // PSRAM; nullptr → render on black
uint16_t* backdrop = nullptr;    // PSRAM: background + rays, refreshed one band per frame (ADR 0008)
int rayBand = 0;
alignas(tank::Tank) uint8_t tankStorage[sizeof(tank::Tank)];  // no heap for the sim
tank::Tank* tankPtr = nullptr;
tank::FixedStepper stepper(kStep, 5);
int homeX = 0, homeY = 0;  // panel position of the frame's top-left: 0,0 on the OLED, centred in the sim
int driftX = 0, driftY = 0;
uint32_t driftStep = UINT32_MAX;

uint32_t lastUs = 0, lastFrameUs = 0, statsAtMs = 0;
uint32_t frames = 0, renderUsSum = 0, pushUsSum = 0, glyphsMax = 0;
uint32_t stageSum[tank::render::Frame::kStageCount] = {};
uint32_t clockUs() { return micros(); }

// Task watchdog (IDF 5 API; the platform is pinned in platformio.ini). Keeps the CPU0 idle-task
// check that sdkconfig enables; never CPU1, where loop() runs without yielding. Failures are
// logged, never silent: a 24/7 piece must not run without a watchdog unnoticed.
void startWatchdog() {
  const esp_task_wdt_config_t cfg = {kWatchdogSec * 1000, 1u << 0, true};
  esp_err_t err = esp_task_wdt_reconfigure(&cfg);
  if (err == ESP_ERR_INVALID_STATE) err = esp_task_wdt_init(&cfg);  // not initialised yet
  if (err == ESP_OK) err = esp_task_wdt_add(nullptr);
  if (err != ESP_OK) logLine("WARN: task watchdog not active (err=0x%x)\n", static_cast<unsigned>(err));
}

// Burn-in protection (ADR 0010, DESIGN §3): every kDriftStepMs the whole image moves one pixel
// along tank::render::driftOffset's path. The ≤ 2 px strips it uncovers are blacked out once,
// on the move; off-panel parts of the push and the fills are clipped by LovyanGFX.
void updateDrift() {
  const uint32_t step = millis() / kDriftStepMs;
  if (step == driftStep) return;
  driftStep = step;
  const auto o = tank::render::driftOffset(step);
  driftX = homeX + o.dx;
  driftY = homeY + o.dy;
  constexpr int r = tank::render::kDriftRadius;
  const int left = homeX - r, top = homeY - r, right = homeX + kW + r, bottom = homeY + kH + r;
  auto black = [](int x, int y, int w, int h) { if (w > 0 && h > 0) lcd.fillRect(x, y, w, h, TFT_BLACK); };
  black(left, top, right - left, driftY - top);                    // above
  black(left, driftY + kH, right - left, bottom - (driftY + kH));  // below
  black(left, driftY, driftX - left, kH);                          // left
  black(driftX + kW, driftY, right - (driftX + kW), kH);           // right
}
}  // namespace

void setup() {
  Serial.begin(115200);
  lcd.init();
  // Left-right mirroring is free in the panel (ADR 0003); the rotation pair per panel is in
  // display/lgfx_config.h. Confirm on the real reflector and mounting in Phase 1.
  lcd.setRotation(kMirrored ? kRotationMirrored : kRotationNormal);
  // The renderer writes native RGB565; a plain uint16_t* is read as byte-swapped unless this is
  // set (LGFXBase::create_pc). Without it every colour except black/white is garbled.
  lcd.setSwapBytes(true);
#if GHOST_PANEL_SIM
  lcd.setBrightness(200);  // the ILI9341's backlight; the OLED has none
#endif
  lcd.fillScreen(TFT_BLACK);
  homeX = (lcd.width() - kW) / 2;  // 0 on the 128×128 OLED; the middle of the simulator's 320×240
  homeY = (lcd.height() - kH) / 2;

  if (GHOST_BANDED) {
    for (auto& b : bandBuf) b = static_cast<uint16_t*>(heap_caps_malloc(kBandBytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    if (!bandBuf[0] || !bandBuf[1]) { heap_caps_free(bandBuf[0]); heap_caps_free(bandBuf[1]); bandBuf[0] = bandBuf[1] = nullptr; }
  }
  if (!bandBuf[0]) {  // full-frame fallback
    frame = static_cast<uint16_t*>(heap_caps_malloc(kFrameBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (!frame) frame = static_cast<uint16_t*>(heap_caps_malloc(kFrameBytes, MALLOC_CAP_SPIRAM));
  }
  background = static_cast<uint16_t*>(heap_caps_malloc(kFrameBytes, MALLOC_CAP_SPIRAM));
  if (!bandBuf[0] && !frame) {
    Serial.println("FATAL: no memory for the framebuffer");
    while (true) delay(1000);
  }
  const tank::Tuning tuning{};
  const tank::FxTuning fx{};
  tankPtr = new (tankStorage) tank::Tank(tuning, fx, esp_random());
  if (background) {
    tank::render::buildBackground(background, fx);
    tank::render::bakeStaticScenery(*tankPtr, background);  // rocks + sand never move
    backdrop = static_cast<uint16_t*>(heap_caps_malloc(kFrameBytes, MALLOC_CAP_SPIRAM));
    if (backdrop)
      for (int b = 0; b < tank::render::kRayBands; ++b) tank::render::refreshBackdrop(*tankPtr, background, backdrop, b);
  } else {
    Serial.println("WARN: no PSRAM for the background; rendering on black");
  }
  startWatchdog();
  lastUs = micros();
  statsAtMs = millis();
  logLine("ghost-aquarium up: frame=%s background=%s heap=%lu psram=%lu\n",
                bandBuf[0] ? "DMA bands" : esp_ptr_external_ram(frame) ? "PSRAM" : "SRAM", backdrop ? "PSRAM+backdrop" : background ? "PSRAM" : "none",
                static_cast<unsigned long>(ESP.getFreeHeap()), static_cast<unsigned long>(ESP.getFreePsram()));
}

void loop() {
  const uint32_t now = micros();
  const int steps = stepper.advance((now - lastUs) / 1e6f);
  lastUs = now;
  for (int i = 0; i < steps; ++i) tankPtr->step(kStep);

  if (now - lastFrameUs >= kMinFrameUs) {
    lastFrameUs = now;
    const uint32_t t0 = micros();
    const uint16_t* source = background;
    uint32_t flags = tank::render::kLive;
    if (backdrop) {  // one ray band per frame; scenery already baked into the backdrop
      tank::render::refreshBackdrop(*tankPtr, background, backdrop, rayBand);
      rayBand = (rayBand + 1) % tank::render::kRayBands;
      source = backdrop;
      flags = tank::render::kBackdropHasRaysAndScenery;
    }
    uint32_t glyphs = 0;
    updateDrift();
    if (bandBuf[0]) {
      // Build once, then rasterise 4 bands of 32 rows: the CPU draws band k+1 while DMA sends band k.
      displayList.clock = clockUs;
      tank::render::buildDisplayList(*tankPtr, displayList, flags);
      const uint32_t t1 = micros();
      lcd.startWrite();
      for (int b = 0; b < kBands; ++b) {
        uint16_t* buf = bandBuf[b & 1];  // its previous DMA (band b-2) finished at the last waitDMA
        tank::render::Frame band{buf, kW, kBandH};
        band.originY = b * kBandH;
        tank::render::rasterize(*tankPtr, displayList, source, band, flags);
        tank::render::swapBytes(buf, kW * kBandH);  // panel byte order: DMA sends it as-is
        lcd.waitDMA();  // band b-1 done → the other buffer is free for band b+1
        lcd.pushImageDMA(driftX, driftY + band.originY, kW, kBandH, reinterpret_cast<const lgfx::swap565_t*>(buf));
      }
      lcd.waitDMA();
      lcd.endWrite();
      glyphs = static_cast<uint32_t>(displayList.n);  // ops per frame (bands would count each 4×)
      const uint32_t t2 = micros();
      renderUsSum += t1 - t0;   // refresh + build (colour/animation math)
      pushUsSum += t2 - t1;     // rasterise + DMA, overlapped
      for (int i = 0; i < tank::render::Frame::kStageCount; ++i) stageSum[i] += displayList.stageUs[i];
    } else {
      tank::render::Frame f{frame, kW, kH};
      f.clock = clockUs;
      tank::render::renderFrame(*tankPtr, source, f, flags);
      const uint32_t t1 = micros();
      lcd.pushImage(driftX, driftY, kW, kH, frame);
      const uint32_t t2 = micros();
      renderUsSum += t1 - t0;
      pushUsSum += t2 - t1;
      glyphs = f.glyphs;
      for (int i = 0; i < tank::render::Frame::kStageCount; ++i) stageSum[i] += f.stageUs[i];
    }
    ++frames;
    if (glyphs > glyphsMax) glyphsMax = glyphs;
  }

  if (millis() - statsAtMs >= 5000 && frames > 0) {
    const float secs = (millis() - statsAtMs) / 1000.f;
    logLine("fps=%.1f build_us=%lu raster_push_us=%lu glyphs_max=%lu away=%d heap=%lu psram=%lu\n", frames / secs,
                  static_cast<unsigned long>(renderUsSum / frames), static_cast<unsigned long>(pushUsSum / frames),
                  static_cast<unsigned long>(glyphsMax), tankPtr->awayCount(), static_cast<unsigned long>(ESP.getFreeHeap()),
                  static_cast<unsigned long>(ESP.getFreePsram()));
    logLine("stages_us bg=%lu rays=%lu plankton=%lu rocks=%lu weed=%lu coral=%lu sand=%lu fish=%lu ambient=%lu\n",
            stageSum[0] / frames, stageSum[1] / frames, stageSum[2] / frames, stageSum[3] / frames, stageSum[4] / frames,
            stageSum[5] / frames, stageSum[6] / frames, stageSum[7] / frames, stageSum[8] / frames);
    for (auto& s : stageSum) s = 0;
    frames = renderUsSum = pushUsSum = glyphsMax = 0;
    statsAtMs = millis();
  }
  esp_task_wdt_reset();
}
