#include "ambient.h"

#include <cmath>

namespace tank {

namespace {
constexpr float kTau = 6.28318530718f;
}

float seaweedSway(float hf, float t, float phase, const Tuning& tu) {
  return tu.seaweedSwayAmp * hf * hf *
         (0.7f * std::sin(kTau * tu.seaweedHz[0] * t + phase) + 0.3f * std::sin(kTau * tu.seaweedHz[1] * t + 1.7f * phase));
}

float surfaceAt(const RipplePool& ripples, float x, float t) {
  float y = kSurfaceY + 1.2f * std::sin(x * 0.07f + t * 1.3f) + 0.6f * std::sin(x * 0.19f - t * 0.9f);
  for (const Ripple& r : ripples) {
    const float age = t - r.t0, d = std::fabs(x - r.x);
    if (d < 70.f * age) y += r.amp * std::exp(-age * 1.1f) * std::exp(-d * 0.018f) * std::sin(0.28f * d - 8.f * age);
  }
  return y;
}

void addRipple(RipplePool& ripples, float x, float t, float amp) {
  if (ripples.count >= RipplePool::kCapacity) ripples.removeAt(0);
  ripples.add({x, t, amp});
}

}  // namespace tank
