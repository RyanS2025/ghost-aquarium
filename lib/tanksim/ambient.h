#pragma once
// Ambient life: bubbles, splash particles, surface ripples, plankton (DESIGN §4).

#include <cstdint>

#include "pool.h"
#include "tuning.h"
#include "vec2.h"

namespace tank {

constexpr int kMaxParticles = 96, kMaxBubbles = 24, kMaxRipples = 12, kPlanktonCount = 32;

struct Bubble {
  Vec2 pos;
  float phase, rise;
};

enum class ParticleKind : uint8_t { Drop, Spark, Bubble };

struct Particle {
  Vec2 pos, vel;
  float life, maxLife;
  uint32_t rgb;
  char ch;
  ParticleKind kind;
};

struct Ripple {
  float x, t0, amp;
};

struct Plankton {
  Vec2 pos, vel;
  float phase;
  uint32_t rgb;
};

using BubblePool = Pool<Bubble, kMaxBubbles>;
using ParticlePool = Pool<Particle, kMaxParticles>;
using RipplePool = Pool<Ripple, kMaxRipples>;

// Seaweed sway at height fraction hf (0 root .. 1 tip): two sines, growing toward the tip.
float seaweedSway(float hf, float t, float phase, const Tuning& tu);

// Water-surface height at x: gentle waves plus decaying ripples.
float surfaceAt(const RipplePool& ripples, float x, float t);

// Adds a ripple; when full, the oldest is dropped.
void addRipple(RipplePool& ripples, float x, float t, float amp);

}  // namespace tank
