#pragma once
// Every number that controls the tank's look and feel (docs/DESIGN.md §6).
// Field ORDER matters: prototype/index.html's "Copy as C++" emits positional initializers in
// this exact order. Paste tuned values over these defaults.

#include <cstdint>

namespace tank {

enum class Layer : uint8_t { Far = 0, Mid = 1, Near = 2 };
constexpr int kLayerCount = 3;

struct Bounds {
  float w, h;  // pixels
};

struct Tuning {
  float maxSpeed[kLayerCount] = {18.f, 28.f, 40.f};  // px/s, Far/Mid/Near
  float maxForce = 30.f;                             // px/s^2, steering cap
  float neighborRadius = 40.f;                       // px, schooling range
  float separationRadius = 14.f;                     // px, personal space
  float cohesionWeight = 0.6f;
  float alignmentWeight = 1.0f;
  float separationWeight = 1.6f;
  float wanderWeight = 0.8f;
  float wanderJitter = 2.0f;                         // rad/s
  float edgeMargin = 24.f;                           // px from a wall where turning starts
  float edgeWeight = 2.5f;
  float facingThreshold = 3.f;                       // px/s of |vel.x| needed to flip the glyph
  uint8_t fishPerLayer[kLayerCount] = {6, 5, 3};
  float bubblesPerSec = 1.5f;
  float bubbleRise = 25.f;                           // px/s
  float bubbleWobbleAmp = 2.f;                       // px
  float bubbleWobbleHz = 1.2f;
  float seaweedSwayAmp = 6.f;                        // px at the tip
  float seaweedHz[2] = {0.31f, 0.83f};
};

}  // namespace tank
