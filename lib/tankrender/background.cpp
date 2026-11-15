#include "background.h"

#include <algorithm>
#include <cmath>

#include "color.h"
#include "palette.h"

namespace tank::render {

namespace {
constexpr int kBayer4[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};

float smoothstep(float e0, float e1, float x) {
  const float t = std::min(1.f, std::max(0.f, (x - e0) / (e1 - e0)));
  return t * t * (3.f - 2.f * t);
}
float channel(uint32_t rgb, int shift) { return static_cast<float>((rgb >> shift) & 255); }
}  // namespace

void buildBackground(uint16_t* out, const FxTuning& fx) {
  const int W = static_cast<int>(kScreen.w), H = static_cast<int>(kScreen.h);
  const float k = 0.32f * fx.bgGlow;
  for (int y = 0; y < H; ++y) {
    const float u = std::min(1.f, std::max(0.f, (y - kSurfaceY) / (H - kSurfaceY)));
    float c[3];
    for (int i = 0; i < 3; ++i) {
      const int sh = 16 - 8 * i;
      const float top = channel(kScene.waterTop, sh), mid = channel(kScene.waterMid, sh), deep = channel(kScene.waterDeep, sh);
      c[i] = u < 0.5f ? top + (mid - top) * (u / 0.5f) : mid + (deep - mid) * ((u - 0.5f) / 0.5f);
    }
    for (int x = 0; x < W; ++x) {
      const float edge = std::min({static_cast<float>(x), static_cast<float>(W - 1 - x), y - kSurfaceY, static_cast<float>(H - 1 - y)});
      const float fade = y < kSurfaceY ? 0.f : smoothstep(0.f, 38.f, edge);
      const float center = 1.f - 0.35f * std::hypot((x - W / 2.f) / (W / 2.f), (y - H * 0.45f) / (H / 2.f));
      const float m = k * fade * std::min(1.f, std::max(0.f, center)) * (1.15f - 0.45f * u);
      const float d = kBayer4[(y & 3) * 4 + (x & 3)] / 16.f - 0.5f;
      out[y * W + x] = rgb565(static_cast<int>(c[0] * m + d * 8.f), static_cast<int>(c[1] * m + d * 4.f), static_cast<int>(c[2] * m + d * 8.f));
    }
  }
}

}  // namespace tank::render
