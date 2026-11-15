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
// THE screen size: everything below, the renderer and the firmware derive from it. 1.5" SSD1351
// OLED, 128×128 (ADR 0010). The tuned 320×240 look of the browser prototype was scaled to this.
constexpr int kScreenW = 128, kScreenH = 128;
constexpr Bounds kScreen{static_cast<float>(kScreenW), static_cast<float>(kScreenH)};
constexpr float kCellW = 6.f, kCellH = 8.f;  // 5×7 glyph in a 6×8 cell: a 21 × 16 character grid
constexpr float kSurfaceY = 12.f;            // water line (room above for the '~' glints)
constexpr float kFloorY = kScreen.h - 22.f;  // bottom edge for fish
constexpr float kGravity = 120.f;            // px/s², above water and for droplets
constexpr float kLeapVy = 52.f;              // px/s exit speed of a leap: ≈11 px above the surface
constexpr float kHomeMin = kSurfaceY + 16.f; // preferred-depth range (fish stay above the reef)
constexpr float kHomeMax = kScreen.h - 34.f;
constexpr float kLeapXMin = kScreen.w * 0.22f, kLeapXMax = kScreen.w * 0.78f;  // leaps away from the side walls
constexpr float kLeapMaxDepth = kScreen.h * 0.25f;  // only fish this close below the surface leap
constexpr float kEdgeFade = 14.f;            // px: the water glow fades to black over this at each edge
constexpr float kFontScale[kLayerCount] = {1.f, 1.f, 1.f};  // one pixel per font pixel: crisp at 128 px
constexpr int kGlyphLen[kLayerCount] = {3, 5, 7};  // ><>  ><('>  ><((('>
constexpr int kMaxFish = 24;

// Core tuning (positional order pinned by test_tuning).
struct Tuning {
  float maxSpeed[kLayerCount] = {7.f, 10.f, 13.f};   // px/s, Far/Mid/Near
  float maxForce = 18.f;                             // px/s², steering cap
  float neighborRadius = 32.f;                       // px, schooling range
  float separationRadius = 14.f;                     // px (× 0.7/1.0/1.35 by layer)
  float cohesionWeight = 0.35f;
  float alignmentWeight = 1.0f;
  float separationWeight = 2.2f;
  float wanderWeight = 0.6f;                         // × maxForce
  float wanderJitter = 1.6f;                         // rad/s
  float edgeMargin = 18.f;                           // px from a wall where turning starts
  float edgeWeight = 2.8f;                           // × maxForce
  float facingThreshold = 1.f;                       // px/s of |vel.x|
  uint8_t fishPerLayer[kLayerCount] = {2, 2, 1};
  float bubblesPerSec = 0.5f;
  float bubbleRise = 14.f;                           // px/s
  float bubbleWobbleAmp = 1.5f;                      // px
  float bubbleWobbleHz = 1.2f;
  float seaweedSwayAmp = 3.5f;                       // px at the tip
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
  float bodyWaveAmp = 0.6f;     // px tail wiggle
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
  int splashDroplets = 8;
};

}  // namespace tank
