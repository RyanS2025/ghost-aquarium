#include "rng.h"

namespace tank {

namespace {
constexpr uint32_t kZeroSeedReplacement = 0x9E3779B9u;  // golden-ratio constant
}

Rng::Rng(uint32_t seed) : state_(seed != 0 ? seed : kZeroSeedReplacement) {}

uint32_t Rng::next() {
  uint32_t x = state_;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  state_ = x;
  return x;
}

float Rng::uniform(float lo, float hi) {
  // Keep the top 24 bits: they fit a float's mantissa exactly, so the unit value is < 1.
  // Dividing the full 32 bits as a float can round up to 1.0 and break the [lo, hi) contract.
  float unit = static_cast<float>(next() >> 8) * (1.0f / 16777216.0f);
  float x = lo + (hi - lo) * unit;
  return x < hi ? x : lo;  // guard the rare float rounding of lo + span*unit up to hi
}

}  // namespace tank
