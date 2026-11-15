#pragma once
// Every number that controls the tank's look and feel (docs/DESIGN.md §6), plus world constants.
// Field ORDER matters: prototype/index.html's "Copy as C++" emits positional initializers in
// exactly this order (pinned by test/test_tuning). Paste tuned values over these defaults.

#include <cstdint>

namespace tank {

enum class Layer : uint8_t { Far = 0, Mid = 1, Near = 2 };
constexpr int kLayerCount = 3;

struct Bounds {
  float w, h;  // pixels
};

// ---- world (DESIGN §4, §5, §7)
constexpr Bounds kScreen{320.f, 240.f};
constexpr float kSurfaceY = 20.f;            // water line
constexpr float kFloorY = 210.f;             // bottom edge for fish (screen height − 30)
constexpr float kGravity = 170.f;            // px/s², above water and for droplets
constexpr float kLeapVy = 100.f;             // px/s exit speed of a leap
constexpr float kHomeMin = kSurfaceY + 32.f; // preferred-depth range (fish stay above the reef)
constexpr float kHomeMax = 240.f - 48.f;
constexpr float kCellW = 6.f, kCellH = 8.f;  // 5×7 glyph in a 6×8 cell
constexpr float kFontScale[kLayerCount] = {1.f, 1.5f, 2.f};
constexpr int kGlyphLen[kLayerCount] = {3, 5, 7};  // ><>  ><('>  ><((('>
constexpr int kMaxFish = 24;

// Core tuning (positional order pinned by test_tuning).
struct Tuning {
  float maxSpeed[kLayerCount] = {16.f, 26.f, 38.f};  // px/s, Far/Mid/Near
  float maxForce = 40.f;                             // px/s², steering cap
  float neighborRadius = 60.f;                       // px, schooling range
  float separationRadius = 26.f;                     // px (× 0.7/1.0/1.35 by layer)
  float cohesionWeight = 0.35f;
  float alignmentWeight = 1.0f;
  float separationWeight = 2.2f;
  float wanderWeight = 0.6f;                         // × maxForce
  float wanderJitter = 1.6f;                         // rad/s
  float edgeMargin = 36.f;                           // px from a wall where turning starts
  float edgeWeight = 2.8f;                           // × maxForce
  float facingThreshold = 2.f;                       // px/s of |vel.x|
  uint8_t fishPerLayer[kLayerCount] = {5, 4, 3};
  float bubblesPerSec = 1.0f;
  float bubbleRise = 25.f;                           // px/s
  float bubbleWobbleAmp = 2.f;                       // px
  float bubbleWobbleHz = 1.2f;
  float seaweedSwayAmp = 7.f;                        // px at the tip
  float seaweedHz[2] = {0.31f, 0.83f};
};

// Motion & look tuning (ADR 0008/0009; positional order pinned by test_tuning).
struct FxTuning {
  float maxTurnRate = 1.6f;     // rad/s
  float maxTilt = 8.f;          // degrees (visual)
  float turnTime = 0.4f;        // s, flip-turn animation
  float flipAngle = 100.f;      // degrees: navigation this far behind → reverse
  float maxPitch = 25.f;        // degrees from horizontal
  float returnDelayMin = 2.f;   // s away before coming back
  float returnDelayMax = 7.f;
  float maxAwayFrac = 0.35f;    // of all fish, counting fish already crossing out
  float minSpeedFrac = 0.3f;    // × maxSpeed
  float bodyWaveAmp = 0.8f;     // px tail wiggle
  float bodyWaveHz = 1.4f;
  float levelPull = 0.7f;
  float depthPull = 0.8f;
  float layerSpacing = 0.4f;    // cross-layer separation weight
  float vibrance = 1.0f;
  float shimmer = 0.12f;
  float bgGlow = 0.9f;
  float rays = 0.7f;
  int reef = 1;
  float dartPerSec = 0.015f;    // per fish
  float leapPerSec = 0.08f;     // per tank
  int splashDroplets = 16;
};

}  // namespace tank
