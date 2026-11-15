#include "renderer.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "color.h"
#include "palette.h"

namespace tank::render {

namespace {
constexpr float kTau = 6.28318530718f;
constexpr const char* kGlyphRight[kLayerCount] = {"><>", "><('>", "><((('>"};
constexpr const char* kGlyphLeft[kLayerCount] = {"<><", "<')><", "<')))><"};

// Depth grading per layer (DESIGN §7): lightness ×, saturation ×, pull toward hue 215°.
struct Depth { float l, s, blue; };
constexpr Depth kDepth[kLayerCount] = {{0.55f, 0.80f, 0.30f}, {0.80f, 0.92f, 0.10f}, {1.f, 1.f, 0.f}};

float clampf(float x, float lo, float hi) { return std::min(hi, std::max(lo, x)); }

void drawRays(const Tank& tk, Frame& f, int rowBegin = 0, int rowEnd = 1 << 30) {
  const FxTuning& fx = tk.fx();
  if (fx.rays <= 0.f) return;
  const float t = tk.animTime();
  const float rr = (kScene.ray >> 16) & 255, rg = (kScene.ray >> 8) & 255, rb = kScene.ray & 255;
  const int yb = static_cast<int>(std::floor(kScreen.h * 0.88f)), y0 = static_cast<int>(kSurfaceY);
  for (const Ray& ray : tk.seabed().rays) {
    const float sway = 3.f * std::sin(t * 0.21f + ray.phase);
    const float a = 0.11f * fx.rays * (0.6f + 0.4f * std::sin(t * 0.5f + ray.phase * 2.f));
    const float x0 = ray.x + sway;
    for (int y = std::max({y0, rowBegin, f.originY}), ye = std::min({yb, rowEnd, f.originY + f.h}); y < ye; ++y) {
      const float u = static_cast<float>(y - y0) / (yb - y0), k = a * (1.f - u);
      const int ar = Frame::roundHalfUp(rr * k / 8.f), ag = Frame::roundHalfUp(rg * k / 4.f), ab = Frame::roundHalfUp(rb * k / 8.f);
      if (!(ar | ag | ab)) continue;
      const float cx = x0 + ray.slant * u, hw = ray.w / 2.f + (ray.w * 0.9f - ray.w / 2.f) * u;
      const int xa = std::max(0, Frame::roundHalfUp(cx - hw)), xe = std::min(f.w, Frame::roundHalfUp(cx + hw));
      for (int x = xa; x < xe; ++x) f.add565((y - f.originY) * f.w + x, ar, ag, ab);
    }
  }
}

template <class Sink>
void drawPlankton(const Tank& tk, Sink& f) {  // single twinkling pixels
  const float t = tk.animTime(), vib = tk.fx().vibrance;
  for (int i = 0; i < kPlanktonCount; ++i) {
    const Plankton& p = tk.plankton()[i];
    const Hsl c = rgbToHsl(p.rgb);
    const float tw = 0.5f + 0.5f * std::sin(t * 1.9f + p.phase);
    f.plot(p.pos.x, p.pos.y, hsl565(c.h, c.s * vib, c.l * (0.25f + 0.55f * tw * tw)));
  }
}

template <class Sink>
void drawRocks(const Tank& tk, Sink& f) {
  const FxTuning& fx = tk.fx();
  const float baseY = kScreen.h - 3.f;
  const Hsl mossC = rgbToHsl(kScene.seaweedTip);
  const uint16_t moss = hsl565(mossC.h, mossC.s * fx.vibrance * 0.8f, mossC.l * 0.6f);
  for (const Rock& r : tk.seabed().rocks) {
    const Hsl base = rgbToHsl(kRockTones[r.tone % 3]);
    const float h = wrapDeg(base.h + r.hueShift), s = base.s * fx.vibrance * 0.85f, l = base.l * r.lMul;
    const RockCell* peak = r.cellN ? &r.cells[0] : nullptr;
    for (int i = 0; i < r.cellN; ++i) {
      const RockCell& c = r.cells[i];
      if (c.y < peak->y) peak = &c;
      const float u = c.x / r.w + 0.5f, up = -c.y / r.h;
      const float light = 0.45f + 0.75f * up + 0.3f * (0.5f - u);  // lit from above-left
      f.blit(c.moss ? '"' : c.ch, r.x + c.x, baseY + c.y, 1.f, c.moss ? moss : hsl565(h, s, l * light * (c.top ? 1.25f : 1.f)));
    }
    if (r.star >= 0 && peak) {
      const Hsl sc = rgbToHsl(kReefColors[r.star % kReefColorCount]);
      f.blit('*', r.x + peak->x, baseY + peak->y - 6.f, 1.f, hsl565(sc.h, sc.s * fx.vibrance, sc.l));
    }
  }
}

template <class Sink>
void drawSeaweed(const Tank& tk, Sink& f) {
  const float t = tk.animTime(), vib = tk.fx().vibrance;
  const Hsl b = rgbToHsl(kScene.seaweedBase), tip = rgbToHsl(kScene.seaweedTip);
  for (const Seaweed& s : tk.seabed().seaweed) {
    const int segs = std::max(2, static_cast<int>(s.height / kCellH));
    for (int i = 0; i < segs; ++i) {
      const float hf = static_cast<float>(i) / (segs - 1);
      const float hue = lerpHue(lerpHue(b.h, tip.h, hf), 170.f, s.tint * 0.5f);
      const uint16_t c = hsl565(hue, (b.s + (tip.s - b.s) * hf) * vib, b.l + (tip.l - b.l) * hf * 0.85f);
      f.blit(i % 2 ? ')' : '(', s.x + seaweedSway(hf, t, s.phase, tk.tuning()), kScreen.h - 4.f - i * kCellH, 1.f, c);
    }
  }
}

template <class Sink>
void drawCoral(const Tank& tk, Sink& f) {
  if (!tk.fx().reef) return;
  const float t = tk.animTime(), vib = tk.fx().vibrance;
  for (const Coral& c : tk.seabed().coral) {
    const CoralTemplate& tpl = kCoral[c.tpl % kCoralTemplates];
    const Hsl col = rgbToHsl(kReefColors[c.color % kReefColorCount]);
    for (int r = 0; r < tpl.rowN; ++r) {
      const float up = static_cast<float>(tpl.rowN - 1 - r) / std::max(1, tpl.rowN - 1);
      const int len = static_cast<int>(std::strlen(tpl.rows[r]));
      for (int k = 0; k < len; ++k) {
        const char ch = tpl.rows[r][k];
        if (ch == ' ') continue;
        const float sway = tpl.flex ? std::sin(t * 1.1f + c.phase + k * 0.35f) * 1.8f * up : 0.f;
        const float glow = tpl.glow ? 0.12f * std::sin(t * 2.2f + c.phase + k) * up : 0.f;
        f.blit(ch, c.x + (k - len / 2.f) * kCellW + sway, kScreen.h - 5.f - (tpl.rowN - 1 - r) * kCellH, 1.f,
               hsl565(col.h, col.s * vib, col.l * (0.45f + 0.55f * up) + glow));
      }
    }
  }
}

template <class Sink>
void drawSand(const Tank& tk, Sink& f) {
  const float vib = tk.fx().vibrance;
  const Hsl sand = rgbToHsl(kScene.sand);
  const uint16_t sandC = hsl565(sand.h, sand.s, sand.l * 0.6f);
  const Seabed& sb = tk.seabed();
  for (int i = 0; i < sb.sandN; ++i) {
    const Sand& g = sb.sand[i];
    if (g.pebble >= 0) {
      const Hsl p = rgbToHsl(kReefColors[g.pebble % kReefColorCount]);
      f.blit('o', g.x, g.y, 1.f, hsl565(p.h, p.s * vib, p.l * 0.75f));
    } else {
      f.blit(g.ch, g.x, g.y, 1.f, sandC);
    }
  }
}

template <class Sink>
void drawFish(const Tank& tk, Sink& f) {
  const float t = tk.animTime();
  const FxTuning& fx = tk.fx();
  for (int L = 0; L < kLayerCount; ++L) {
    const Depth& d = kDepth[L];
    for (int i = 0; i < tk.fishCount(); ++i) {
      const Fish& fish = tk.fish()[i];
      if (static_cast<int>(fish.layer) != L || fish.away) continue;
      const int n = kGlyphLen[L];
      char headFirst[kMaxBodyPoints];
      for (int j = 0; j < n; ++j) headFirst[j] = fish.facingRight ? kGlyphRight[L][n - 1 - j] : kGlyphLeft[L][j];
      const float squeeze = flipSqueeze(fish);
      Vec2 body[kMaxBodyPoints];
      rigidBody(fish, n, charAdvance(fish.layer), squeeze, body);
      const Hsl a = rgbToHsl(kFishPalette[fish.palette % kPaletteCount][0]);
      const Hsl b = rgbToHsl(kFishPalette[fish.palette % kPaletteCount][1]);
      for (int j = 0; j < n; ++j) {
        const float u = n > 1 ? static_cast<float>(j) / (n - 1) : 0.f;
        const float wig = fx.bodyWaveAmp * squeeze * u * std::sqrt(u) * std::sin(fish.wavePhase - j * 0.9f);
        const float h = lerpHue(lerpHue(a.h, b.h, u), 215.f, d.blue);
        const float s = (a.s + (b.s - a.s) * u) * d.s * fx.vibrance;
        const float l = (a.l + (b.l - a.l) * u) * d.l + fx.shimmer * std::sin(t * 3.1f - j * 0.85f + fish.id * 1.7f) * d.l;
        f.blit(headFirst[j], body[j].x, body[j].y + wig, kFontScale[L], hsl565(h, s, l));
      }
    }
  }
}

template <class Sink>
void drawAmbient(const Tank& tk, Sink& f) {
  const float t = tk.animTime(), vib = tk.fx().vibrance;
  const Tuning& tu = tk.tuning();
  const Hsl bub = rgbToHsl(kScene.bubble);
  const uint16_t bubC = hsl565(bub.h, bub.s, bub.l * 0.8f);
  for (const Bubble& b : tk.bubbles())
    f.blit('o', b.pos.x + tu.bubbleWobbleAmp * std::sin(kTau * tu.bubbleWobbleHz * t + b.phase), b.pos.y, 1.f, bubC);
  for (const Particle& p : tk.particles()) {
    const float k = clampf(p.life / p.maxLife, 0.f, 1.f);
    const Hsl c = rgbToHsl(p.rgb);
    f.blit(p.ch, p.pos.x, p.pos.y, 1.f, hsl565(c.h, c.s * vib, c.l * (0.25f + 0.75f * k)));
  }
  const Hsl sc = rgbToHsl(kScene.surface);
  constexpr int kSurfaceN = static_cast<int>((kScreen.w - 3.f) / kCellW) + 1;  // x = 3, 9, ... (21 at 128 px)
  float ys[kSurfaceN];
  surfaceProfile(tk.ripples(), t, 3.f, kCellW, kSurfaceN, ys);
  const uint32_t tick = static_cast<uint32_t>(t * 3.f);  // glints re-roll 3× per second
  for (int k = 0; k < kSurfaceN; ++k) {
    const float x = 3.f + kCellW * k, y = ys[k], lift = clampf((kSurfaceY - y) / 3.f, -1.f, 1.f);
    f.blit('~', x, y, 1.f, hsl565(sc.h, sc.s * vib, sc.l * (0.6f + 0.35f * lift)));
    uint32_t h = static_cast<uint32_t>(k) * 73856093u ^ tick * 19349663u;  // cheap integer hash
    h ^= h >> 13; h *= 0x5BD1E995u; h ^= h >> 15;
    if ((h & 1023u) < 61u) f.blit('\'', x, y - 5.f, 1.f, 0xFFFF);  // ~6% of points glint
  }
}
}  // namespace

void bakeStaticScenery(const Tank& tk, uint16_t* background) {
  Frame f{background, static_cast<int>(kScreen.w), static_cast<int>(kScreen.h)};
  drawRocks(tk, f);
  drawSand(tk, f);
}

void refreshBackdrop(const Tank& tk, const uint16_t* base, uint16_t* backdrop, int band) {
  const int W = static_cast<int>(kScreen.w), H = static_cast<int>(kScreen.h);
  const int rows = H / kRayBands, y0 = band * rows, y1 = band == kRayBands - 1 ? H : y0 + rows;
  if (band < 0 || band >= kRayBands) return;
  std::memcpy(backdrop + y0 * W, base + y0 * W, static_cast<size_t>(y1 - y0) * W * sizeof(uint16_t));
  Frame f{backdrop, W, H};
  drawRays(tk, f, y0, y1);
}

void DisplayList::blit(char ch, float cx, float cy, float s, uint16_t color) {
  if (!glyphRows(ch)) return;
  if (n >= kMaxOps) { ++dropped; return; }
  ops[n++] = {static_cast<int16_t>(Frame::roundHalfUp(cx - 6.f * s / 2.f)), static_cast<int16_t>(Frame::roundHalfUp(cy - 8.f * s / 2.f)),
              color, ch, static_cast<uint8_t>(Frame::roundHalfUp(s * 2.f))};
}

void DisplayList::plot(float x, float y, uint16_t color) {
  if (n >= kMaxOps) { ++dropped; return; }
  ops[n++] = {static_cast<int16_t>(Frame::roundHalfUp(x)), static_cast<int16_t>(Frame::roundHalfUp(y)), color, 0, 0};
}

void buildDisplayList(const Tank& tk, DisplayList& list, uint32_t flags) {
  list.n = 0;
  list.dropped = 0;
  uint32_t last = list.clock ? list.clock() : 0;
  auto mark = [&list, &last](Frame::Stage s) {
    if (!list.clock) return;
    const uint32_t now = list.clock();
    list.stageUs[s] = now - last;
    last = now;
  };
  const bool baked = (flags & kBackdropHasRaysAndScenery) != 0;
  drawPlankton(tk, list); mark(Frame::kPlankton);
  if (!baked) drawRocks(tk, list);
  mark(Frame::kRocks);
  drawSeaweed(tk, list);  mark(Frame::kSeaweed);
  drawCoral(tk, list);    mark(Frame::kCoral);
  if (!baked) drawSand(tk, list);
  mark(Frame::kSand);
  drawFish(tk, list);     mark(Frame::kFish);
  drawAmbient(tk, list);  mark(Frame::kAmbient);
}

void rasterize(const Tank& tk, const DisplayList& list, const uint16_t* background, Frame& f, uint32_t flags) {
  const int W = static_cast<int>(kScreen.w), H = static_cast<int>(kScreen.h);
  if (f.w != W || f.originY < 0 || f.originY + f.h > H) return;  // bands of the full-width screen only
  const size_t bytes = static_cast<size_t>(f.w) * f.h * sizeof(uint16_t);
  if (background) std::memcpy(f.px, background + static_cast<size_t>(f.originY) * W, bytes);
  else std::memset(f.px, 0, bytes);
  if (!(flags & kBackdropHasRaysAndScenery)) drawRays(tk, f);
  for (int i = 0; i < list.n; ++i) {
    const DrawOp& op = list.ops[i];
    if (op.ch) f.blitAt(op.ch, op.x, op.y, op.scale2 * 0.5f, op.color);
    else f.plotAt(op.x, op.y, op.color);
  }
}

void swapBytes(uint16_t* px, size_t n) {
  for (size_t i = 0; i < n; ++i) px[i] = static_cast<uint16_t>((px[i] >> 8) | (px[i] << 8));
}

void renderFrame(const Tank& tk, const uint16_t* background, Frame& f, uint32_t flags) {
  f.glyphs = 0;
  f.writes = 0;
  if (f.w != static_cast<int>(kScreen.w) || f.h != static_cast<int>(kScreen.h)) return;  // full frames only
  static DisplayList list;  // ~8 KB: static, never on the (8 KB) loop-task stack
  list.clock = f.clock;
  buildDisplayList(tk, list, flags);
  const uint32_t t0 = f.clock ? f.clock() : 0;
  rasterize(tk, list, background, f, flags);
  if (f.clock) {
    for (int i = 0; i < Frame::kStageCount; ++i) f.stageUs[i] = list.stageUs[i];
    f.stageUs[Frame::kRaster] = f.clock() - t0;
  }
}

}  // namespace tank::render
