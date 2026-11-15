#pragma once
// Seeded xorshift32 RNG. All simulation randomness goes through this, so the same seed always
// gives the same tank (ADR 0005). Never use rand()/millis()/esp_random() inside tanksim.

#include <cstdint>

namespace tank {

class Rng {
 public:
  // Seed 0 is a fixed point of xorshift (it would return 0 forever), so it's replaced.
  explicit Rng(uint32_t seed);

  uint32_t next();

  // Uniform float in [lo, hi).
  float uniform(float lo, float hi);

 private:
  uint32_t state_;
};

// Uniform index in [0, n).
inline int randomIndex(Rng& rng, int n) {
  const int k = static_cast<int>(rng.uniform(0.f, static_cast<float>(n)));
  return k < n ? k : n - 1;
}

}  // namespace tank
