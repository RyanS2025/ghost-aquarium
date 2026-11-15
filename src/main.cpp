// Ghost Aquarium firmware: owns memory and timing, delegates everything else.
//   lib/tanksim    → the simulation (fixed 60 Hz steps)
//   lib/tankrender → draws the tank into a plain RGB565 framebuffer
//   here           → allocate buffers, step, render, push to the panel, watchdog, stats
// See docs/DESIGN.md §8 and ADR 0008 for the budget and pipeline.

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_random.h>
#include <esp_task_wdt.h>

#include <new>

#include "background.h"
#include "display/lgfx_config.h"
#include "renderer.h"
#include "stepper.h"
#include "tank.h"

namespace {
constexpr int kW = 320, kH = 240;
constexpr size_t kFrameBytes = kW * kH * sizeof(uint16_t);
constexpr float kStep = 1.f / 60.f;
constexpr uint32_t kMinFrameUs = 1000000 / 30;  // render at ≤ 30 fps (the SPI push limit)
constexpr bool kMirrored = true;                 // Pepper's Ghost: the reflector flips left/right
constexpr uint32_t kWatchdogSec = 5;

LGFX lcd;
uint16_t* frame = nullptr;       // internal SRAM (fast); PSRAM fallback
uint16_t* background = nullptr;  // PSRAM; nullptr → render on black
alignas(tank::Tank) uint8_t tankStorage[sizeof(tank::Tank)];  // no heap for the sim
tank::Tank* tankPtr = nullptr;
tank::FixedStepper stepper(kStep, 5);

uint32_t lastUs = 0, lastFrameUs = 0, statsAtMs = 0;
uint32_t frames = 0, renderUsSum = 0, pushUsSum = 0, glyphsMax = 0;

void startWatchdog() {
#if ESP_IDF_VERSION_MAJOR >= 5
  const esp_task_wdt_config_t cfg = {kWatchdogSec * 1000, 0, true};
  esp_task_wdt_reconfigure(&cfg);
#else
  esp_task_wdt_init(kWatchdogSec, true);
#endif
  esp_task_wdt_add(nullptr);
}
}  // namespace

void setup() {
  Serial.begin(115200);
  lcd.init();
  // LovyanGFX rotations 4..7 are the mirrored versions of 0..3: the panel flips the image in
  // hardware for free (ADR 0003). Verify the mirror direction on the real reflector (Phase 1).
  lcd.setRotation(kMirrored ? 5 : 1);
  lcd.setBrightness(200);
  lcd.fillScreen(TFT_BLACK);

  frame = static_cast<uint16_t*>(heap_caps_malloc(kFrameBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (!frame) frame = static_cast<uint16_t*>(heap_caps_malloc(kFrameBytes, MALLOC_CAP_SPIRAM));
  background = static_cast<uint16_t*>(heap_caps_malloc(kFrameBytes, MALLOC_CAP_SPIRAM));
  if (!frame) {
    Serial.println("FATAL: no memory for the framebuffer");
    while (true) delay(1000);
  }
  const tank::Tuning tuning{};
  const tank::FxTuning fx{};
  if (background) tank::render::buildBackground(background, fx);
  else Serial.println("WARN: no PSRAM for the background; rendering on black");

  tankPtr = new (tankStorage) tank::Tank(tuning, fx, esp_random());
  startWatchdog();
  lastUs = micros();
  statsAtMs = millis();
  Serial.printf("ghost-aquarium up: frame=%s background=%s heap=%lu psram=%lu\n",
                esp_ptr_external_ram(frame) ? "PSRAM" : "SRAM", background ? "PSRAM" : "none",
                static_cast<unsigned long>(ESP.getFreeHeap()), static_cast<unsigned long>(ESP.getFreePsram()));
}

void loop() {
  const uint32_t now = micros();
  const int steps = stepper.advance((now - lastUs) / 1e6f);
  lastUs = now;
  for (int i = 0; i < steps; ++i) tankPtr->step(kStep);

  if (now - lastFrameUs >= kMinFrameUs) {
    lastFrameUs = now;
    tank::render::Frame f{frame, kW, kH};
    const uint32_t t0 = micros();
    tank::render::renderFrame(*tankPtr, background, f);
    const uint32_t t1 = micros();
    lcd.pushImage(0, 0, kW, kH, frame);
    const uint32_t t2 = micros();
    ++frames;
    renderUsSum += t1 - t0;
    pushUsSum += t2 - t1;
    if (f.glyphs > glyphsMax) glyphsMax = f.glyphs;
  }

  if (millis() - statsAtMs >= 5000 && frames > 0) {
    const float secs = (millis() - statsAtMs) / 1000.f;
    Serial.printf("fps=%.1f render_us=%lu push_us=%lu glyphs_max=%lu away=%d heap=%lu psram=%lu\n", frames / secs,
                  static_cast<unsigned long>(renderUsSum / frames), static_cast<unsigned long>(pushUsSum / frames),
                  static_cast<unsigned long>(glyphsMax), tankPtr->awayCount(), static_cast<unsigned long>(ESP.getFreeHeap()),
                  static_cast<unsigned long>(ESP.getFreePsram()));
    frames = renderUsSum = pushUsSum = glyphsMax = 0;
    statsAtMs = millis();
  }
  esp_task_wdt_reset();
}
