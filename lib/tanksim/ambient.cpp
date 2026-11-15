#include "ambient.h"

#include <algorithm>
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
    const float age = r.age, d = std::fabs(x - r.x);
    if (d < std::min(70.f * age, kRippleReach)) y += r.amp * std::exp(-age * 1.1f) * std::exp(-d * 0.018f) * std::sin(0.28f * d - 8.f * age);
  }
  return y;
}

void surfaceProfile(const RipplePool& ripples, float t, float x0, float dx, int n, float* out) {
  // Base waves: sin(a + k·δ) by rotation, 4 trig calls per frame instead of 2 per point.
  float s1 = std::sin(x0 * 0.07f + t * 1.3f), c1 = std::cos(x0 * 0.07f + t * 1.3f);
  float s2 = std::sin(x0 * 0.19f - t * 0.9f), c2 = std::cos(x0 * 0.19f - t * 0.9f);
  const float cd1 = std::cos(dx * 0.07f), sd1 = std::sin(dx * 0.07f);
  const float cd2 = std::cos(dx * 0.19f), sd2 = std::sin(dx * 0.19f);
  for (int k = 0; k < n; ++k) {
    out[k] = kSurfaceY + 1.2f * s1 + 0.6f * s2;
    const float n1 = s1 * cd1 + c1 * sd1, n2 = s2 * cd2 + c2 * sd2;
    c1 = c1 * cd1 - s1 * sd1; s1 = n1;
    c2 = c2 * cd2 - s2 * sd2; s2 = n2;
  }
  for (const Ripple& r : ripples) {
    const float amp = r.amp * std::exp(-r.age * 1.1f), reach = std::min(70.f * r.age, kRippleReach);
    for (int k = 0; k < n; ++k) {
      const float d = std::fabs(x0 + dx * k - r.x);
      if (d < reach) out[k] += amp * std::exp(-d * 0.018f) * std::sin(0.28f * d - 8.f * r.age);
    }
  }
}

void addRipple(RipplePool& ripples, float x, float amp) {
  if (ripples.count >= RipplePool::kCapacity) ripples.removeAt(0);
  ripples.add({x, 0.f, amp});
}

}  // namespace tank
