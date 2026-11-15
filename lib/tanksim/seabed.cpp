#include "seabed.h"

#include <algorithm>
#include <cmath>

#include "palette.h"
#include "vec2.h"

namespace tank {

namespace {
constexpr float kPi = 3.14159265358979f;
constexpr float kTau = 2.f * kPi;
}  // namespace

const CoralTemplate kCoral[kCoralTemplates] = {
    {{" \\|/ ", "\\ Y /", " \\|/ ", "  |  "}, 4, true, false},   // branch coral
    {{"}{}{}", "{}{}{", " }|{ ", "  |  "}, 4, true, false},       // sea fan
    {{"\\\\|//", " \\|/ ", " (_) ", nullptr}, 3, true, true},     // anemone (glows)
    {{" @@@ ", "@@@@@", nullptr, nullptr}, 2, false, false},      // brain coral
    {{"o    ", "| o  ", "| | o", "|_|_|"}, 4, false, false},       // tube sponges
};

void buildRock(Rng& rng, float x, float w, float h, Rock& out) {
  constexpr int n = 11;
  Vec2 pts[n + 1];
  for (int k = 0; k <= n; ++k) {
    const float a = kPi + kPi * k / n;
    const float j = (k == 0 || k == n) ? 1.f : rng.uniform(0.8f, 1.08f);
    pts[k] = {std::cos(a) * w / 2.f * j, std::min(0.f, std::sin(a) * h * j)};
  }
  auto topAt = [&](float px) {
    for (int k = 0; k < n; ++k) {
      const Vec2 a = pts[k], b = pts[k + 1];
      if (a.x != b.x && (px - a.x) * (px - b.x) <= 0.f) return a.y + (b.y - a.y) * (px - a.x) / (b.x - a.x);
    }
    return 0.f;
  };
  out = Rock{};
  out.x = x; out.w = w; out.h = h;
  for (float cx = -w / 2.f + kCellW / 2.f; cx < w / 2.f && out.cellN < kMaxRockCells; cx += kCellW) {
    const float slope = (topAt(cx + 1.5f) - topAt(cx - 1.5f)) / 3.f;
    const int rows = std::max(1, static_cast<int>(std::lround(-topAt(cx) / kCellH)));
    for (int r = 0; r < rows && out.cellN < kMaxRockCells; ++r) {
      const bool top = r == rows - 1, edgeL = cx < -w / 2.f + kCellW, edgeR = cx > w / 2.f - kCellW;
      char ch;
      if (top) ch = slope < -0.45f ? '/' : slope > 0.45f ? '\\' : '-';
      else if (edgeL) ch = '(';
      else if (edgeR) ch = ')';
      else { const float q = rng.uniform(0.f, 1.f); ch = q < 0.7f ? '#' : q < 0.88f ? '%' : ':'; }  // calm stone
      const bool moss = top && std::fabs(slope) < 0.6f && rng.uniform(0.f, 1.f) < 0.35f;
      out.cells[out.cellN++] = {cx, -kCellH * (r + 0.5f), ch, top, moss};
    }
  }
  out.tone = static_cast<uint8_t>(randomIndex(rng, 3));
  out.lMul = rng.uniform(0.85f, 1.2f);
  out.hueShift = rng.uniform(-12.f, 12.f);
}

void buildSeabed(Rng& rng, Seabed& s) {
  enum Kind : uint8_t { kRock, kWeed, kCoralPiece };
  struct Piece { Kind kind; float half, w, h; };
  constexpr float kRockSizes[kRocks][2] = {{30, 18}, {18, 11}};
  Piece bed[kRocks + kSeaweeds + kCorals];
  int n = 0;
  for (const auto& sz : kRockSizes) {
    const float w = sz[0] * rng.uniform(0.85f, 1.15f), h = sz[1] * rng.uniform(0.85f, 1.2f);
    bed[n++] = {kRock, w / 2.f, w, h};
  }
  for (int i = 0; i < kSeaweeds; ++i) bed[n++] = {kWeed, 4.f, 0, 0};
  for (int i = 0; i < kCorals; ++i) bed[n++] = {kCoralPiece, 15.f, 0, 0};  // 5 chars × 6 px
  for (int i = n - 1; i > 0; --i) std::swap(bed[i], bed[randomIndex(rng, i + 1)]);

  float used = 0.f;
  for (int i = 0; i < n; ++i) used += 2.f * bed[i].half;
  const float gap = std::max(0.f, (kScreen.w - 4.f - used) / (n + 1));
  float cursor = 2.f + gap;
  int ri = 0, wi = 0, ci = 0;
  for (int i = 0; i < n; ++i) {
    const Piece& b = bed[i];
    const float x = cursor + b.half + rng.uniform(-0.35f, 0.35f) * gap;
    cursor += 2.f * b.half + gap;
    if (b.kind == kRock) {
      buildRock(rng, x, b.w, b.h, s.rocks[ri++]);
    } else if (b.kind == kWeed) {
      s.seaweed[wi++] = {x, rng.uniform(22.f, 46.f), rng.uniform(0.f, kTau), rng.uniform(0.f, 1.f)};
    } else {
      const auto tpl = static_cast<uint8_t>(randomIndex(rng, kCoralTemplates));
      const auto color = static_cast<uint8_t>(randomIndex(rng, kReefColorCount));
      s.coral[ci++] = {x, tpl, color, rng.uniform(0.f, kTau)};
    }
  }
  std::sort(s.rocks, s.rocks + kRocks, [](const Rock& a, const Rock& b) { return a.h > b.h; });  // big first
  s.rocks[0].star = static_cast<int8_t>(randomIndex(rng, kReefColorCount));
  if (rng.uniform(0.f, 1.f) < 0.6f) s.rocks[1].star = static_cast<int8_t>(randomIndex(rng, kReefColorCount));

  s.sandN = 0;
  for (float x = 3.f; x < kScreen.w && s.sandN < kMaxSand; x += kCellW) {
    static const char kSandChars[4] = {',', '.', '`', '.'};
    const float sx = x + rng.uniform(-1.f, 1.f), sy = kScreen.h - 2.f - rng.uniform(0.f, 3.f);
    const char ch = kSandChars[randomIndex(rng, 4)];
    const int8_t pebble = rng.uniform(0.f, 1.f) < 0.08f ? static_cast<int8_t>(randomIndex(rng, kReefColorCount)) : -1;
    s.sand[s.sandN++] = {sx, sy, ch, pebble};
  }
  for (int i = 0; i < kRays; ++i) {
    const float x = kScreen.w * (0.15f + 0.7f * i / (kRays - 1)) + rng.uniform(-8.f, 8.f);
    s.rays[i] = {x, rng.uniform(6.f, 12.f), rng.uniform(10.f, 22.f), rng.uniform(0.f, kTau)};
  }
}

}  // namespace tank
