#pragma once
// RGB565 / HSL helpers (same math as the prototype's device renderer).

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace tank::render {

inline int clampi(int v, int lo, int hi) { return std::min(hi, std::max(lo, v)); }

inline uint16_t rgb565(int r, int g, int b) {
  r = clampi(r, 0, 255); g = clampi(g, 0, 255); b = clampi(b, 0, 255);
  return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

struct Hsl {
  float h, s, l;  // degrees, 0..1, 0..1
};

inline Hsl rgbToHsl(uint32_t rgb) {
  const float r = ((rgb >> 16) & 255) / 255.f, g = ((rgb >> 8) & 255) / 255.f, b = (rgb & 255) / 255.f;
  const float mx = std::max({r, g, b}), mn = std::min({r, g, b}), l = (mx + mn) / 2.f;
  if (mx == mn) return {0.f, 0.f, l};
  const float d = mx - mn, s = l > 0.5f ? d / (2.f - mx - mn) : d / (mx + mn);
  float h = mx == r ? (g - b) / d + (g < b ? 6.f : 0.f) : mx == g ? (b - r) / d + 2.f : (r - g) / d + 4.f;
  return {h * 60.f, s, l};
}

inline uint16_t hsl565(float h, float s, float l) {
  s = std::min(1.f, std::max(0.f, s));
  l = std::min(1.f, std::max(0.f, l));
  const float c = (1.f - std::fabs(2.f * l - 1.f)) * s;
  const float hp = std::fmod(std::fmod(h, 360.f) + 360.f, 360.f) / 60.f;
  const float x = c * (1.f - std::fabs(std::fmod(hp, 2.f) - 1.f)), m = l - c / 2.f;
  float r = 0, g = 0, b = 0;
  if (hp < 1) { r = c; g = x; } else if (hp < 2) { r = x; g = c; } else if (hp < 3) { g = c; b = x; }
  else if (hp < 4) { g = x; b = c; } else if (hp < 5) { r = x; b = c; } else { r = c; b = x; }
  return rgb565(static_cast<int>((r + m) * 255.f), static_cast<int>((g + m) * 255.f), static_cast<int>((b + m) * 255.f));
}

// Shortest-way hue interpolation.
inline float lerpHue(float a, float b, float t) {
  const float d = std::fmod(b - a + 540.f, 360.f) - 180.f;
  return std::fmod(a + d * t + 360.f, 360.f);
}

}  // namespace tank::render
