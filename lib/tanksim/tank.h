#pragma once
// The whole tank: fish, ambient life and seabed, advanced one fixed step at a time.
// Port of the prototype's makeTank() + step() (DESIGN §4–§5, ADR 0009). Deterministic per seed;
// no heap allocation (fixed pools).

#include <cstdint>

#include "ambient.h"
#include "fish.h"
#include "rng.h"
#include "seabed.h"
#include "tuning.h"

namespace tank {

class Tank {
 public:
  Tank(const Tuning& t, const FxTuning& fx, uint32_t seed);

  void step(float dt);

  // Starts a leap if some fish qualifies (Mid/Near, near the surface, away from the side
  // walls, heading toward the middle). Returns whether one leapt.
  bool tryLeap();

  // Splash at the surface (exposed for tests and future interaction).
  void splash(float x, float strength, uint32_t rgb);

  const Fish* fish() const { return fish_; }
  int fishCount() const { return fishN_; }
  int awayCount() const;
  const BubblePool& bubbles() const { return bubbles_; }
  const ParticlePool& particles() const { return particles_; }
  const RipplePool& ripples() const { return ripples_; }
  const Plankton* plankton() const { return plankton_; }
  const Seabed& seabed() const { return seabed_; }
  float time() const { return time_; }
  const Tuning& tuning() const { return t_; }
  const FxTuning& fx() const { return fx_; }

 private:
  void reenter(Fish& f);
  void updateBody(Fish& f);
  void emit(const Particle& p) { particles_.add(p); }

  Tuning t_;
  FxTuning fx_;
  Rng rng_;
  Fish fish_[kMaxFish];
  int fishN_ = 0;
  BubblePool bubbles_;
  ParticlePool particles_;
  RipplePool ripples_;
  Plankton plankton_[kPlanktonCount];
  Seabed seabed_;
  float time_ = 0.f;
  float bubbleAcc_ = 0.f;
};

}  // namespace tank
