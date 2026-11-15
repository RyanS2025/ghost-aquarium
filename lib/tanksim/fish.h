#pragma once
// One fish: state + the motion rules of DESIGN §5 / ADR 0009, ported from prototype step().

#include <cstdint>

#include "rng.h"
#include "tuning.h"
#include "vec2.h"

namespace tank {

constexpr int kMaxBodyPoints = 7;

struct Fish {
  Vec2 pos{}, vel{};
  Layer layer = Layer::Far;
  uint8_t palette = 0;
  uint8_t id = 0;
  bool facingRight = true;
  bool airborne = false;      // above the water during a leap
  bool away = false;          // swam out of view; waiting to come back
  bool crossing = false;      // partly outside a side edge (leaving or entering)
  bool leaving = false;       // crossing outward
  bool sideWalls = false;     // side walls apply this step (the away cap is full)
  bool flipTurn = false;      // currently doing a flip-turn
  bool turnSwapped = false;   // flip already reversed at its midpoint
  float wanderAngle = 0.f;    // heading-relative wander circle angle
  float wavePhase = 0.f;      // tail wiggle phase
  float tilt = 0.f;           // radians, smoothed visual pitch
  float turnP = 0.f;          // flip-turn progress 0..1 (0 = not turning)
  float boostT = 0.f;         // s of dart/leap boost left
  float leapT = 0.f;          // s of leap left
  float flipCooldown = 0.f;   // s until another navigation flip is allowed
  float reverseT = 0.f;       // s the navigation force has pointed backward
  float homeBase = 0.f;       // this fish's depth band
  float homeY = 0.f;          // drifting preferred depth
  float returnT = 0.f;        // s until an away fish comes back
  Vec2 body[kMaxBodyPoints]{};  // character centres (head first), for spacing
  uint8_t bodyN = 0;
};

// Character advance and body length for a layer's font scale (6 / 9 / 12 px).
float charAdvance(Layer layer);
float bodyLength(Layer layer);

// Rigid body: n points `spacing` apart behind the head, tilted by f.tilt. `squeeze` (0..1]
// shortens the row around its middle point (the flip-turn animation).
void rigidBody(const Fish& f, int n, float spacing, float squeeze, Vec2* out);

// Flip-turn squeeze factor for drawing: 1 when not turning, down to 0.12 mid-turn.
float flipSqueeze(const Fish& f);

// Wall forces: surface (off while leaping), floor, and side walls only when f.sideWalls.
Vec2 steerEdges(const Fish& f, const Tuning& t);

// Reynolds wander: steer toward a point drifting on a circle (radius 1) placed 2 ahead.
// Never points more than 30° away from the heading.
Vec2 steerWander(Fish& f, Rng& rng, float dt, const Tuning& t);

// Drifts f.homeY and returns the level-swimming + preferred-depth force.
Vec2 steerLevel(Fish& f, Rng& rng, float dt, const FxTuning& fx);

// Wall priority: limits `others` to maxForce·boostMul·(1 − 0.6e) and adds `edges`.
// e = |edges| / (edgeWeight·maxForce), written to *edgeFrac.
Vec2 prioritize(Vec2 others, Vec2 edges, float boostMul, const Tuning& t, float* edgeFrac);

// Integrate one step: turn-rate + pitch limits, minimum speed, flip trigger (navigation
// force behind for 0.25 s, 3 s cooldown unless near a wall), leap and airborne cases.
void swim(Fish& f, Vec2 force, Vec2 nav, float edgeFrac, float dt, const Tuning& t, const FxTuning& fx);

// Flip-turn animation (reverses vx and swaps the glyph at the midpoint), the facing safety
// net, and visual tilt smoothing.
void advanceTurn(Fish& f, float dt, const Tuning& t, const FxTuning& fx);

}  // namespace tank
