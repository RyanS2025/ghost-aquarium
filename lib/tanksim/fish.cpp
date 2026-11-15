#include "fish.h"

#include <algorithm>
#include <cmath>

namespace tank {

namespace {
constexpr float kPi = 3.14159265358979f;
constexpr float kTau = 2.f * kPi;
constexpr float kDeg = kPi / 180.f;
constexpr float kWanderAhead = 2.f, kWanderRadius = 1.f;

float clampf(float x, float lo, float hi) { return std::min(hi, std::max(lo, x)); }
float wrapAngle(float a) { return std::remainder(a, kTau); }  // → [-π, π]
int sgn(float x) { return (x > 0.f) - (x < 0.f); }
Vec2 heading(const Fish& f) { return f.vel.length() > 1e-3f ? f.vel.normalized() : Vec2{f.facingRight ? 1.f : -1.f, 0.f}; }

void startFlip(Fish& f) {
  f.turnP = 1e-6f;
  f.flipTurn = true;
  f.turnSwapped = false;
  f.reverseT = 0.f;
  f.flipCooldown = 3.f;
}
}  // namespace

float charAdvance(Layer layer) { return kCellW * kFontScale[static_cast<int>(layer)]; }
float bodyLength(Layer layer) { return kGlyphLen[static_cast<int>(layer)] * charAdvance(layer); }

void rigidBody(const Fish& f, int n, float spacing, float squeeze, Vec2* out) {
  const float back = f.facingRight ? -1.f : 1.f, slope = std::tan(f.tilt);
  for (int j = 0; j < n; ++j) out[j] = {f.pos.x + back * j * spacing, f.pos.y - j * spacing * slope};
  if (squeeze == 1.f) return;
  const Vec2 mid = out[(n - 1) / 2];
  for (int j = 0; j < n; ++j) out[j] = mid + (out[j] - mid) * squeeze;
}

float flipSqueeze(const Fish& f) {
  return f.turnP > 0.f ? std::max(0.12f, std::fabs(1.f - 2.f * f.turnP)) : 1.f;
}

Vec2 steerEdges(const Fish& f, const Tuning& t) {
  const float m = t.edgeMargin, k = t.edgeWeight * t.maxForce;
  auto fac = [m](float d) { return d < 0.f ? 1.f : clampf((m - d) / m, 0.f, 1.f); };
  const float top = f.leapT > 0.f ? 0.f : fac(f.pos.y - kSurfaceY);
  const float side = f.sideWalls ? fac(f.pos.x) * k - fac(kScreen.w - f.pos.x) * k : 0.f;
  return {side, top * k - fac(kFloorY - f.pos.y) * k};
}

Vec2 steerWander(Fish& f, Rng& rng, float dt, const Tuning& t) {
  f.wanderAngle = wrapAngle(f.wanderAngle + rng.uniform(-1.f, 1.f) * t.wanderJitter * dt);
  const Vec2 h = heading(f);
  const float ca = std::cos(f.wanderAngle), sa = std::sin(f.wanderAngle);
  const Vec2 target{h.x * kWanderAhead + (h.x * ca - h.y * sa) * kWanderRadius,
                    h.y * kWanderAhead + (h.y * ca + h.x * sa) * kWanderRadius};
  return target.normalized() * (t.wanderWeight * t.maxForce);
}

Vec2 steerLevel(Fish& f, Rng& rng, float dt, const FxTuning& fx) {
  f.homeY = clampf(f.homeY + rng.uniform(-1.f, 1.f) * 8.f * dt + (f.homeBase - f.homeY) * 0.05f * dt, kHomeMin, kHomeMax);
  return {0.f, -f.vel.y * fx.levelPull * 2.f + (f.homeY - f.pos.y) * fx.depthPull * 0.5f};
}

Vec2 prioritize(Vec2 others, Vec2 edges, float boostMul, const Tuning& t, float* edgeFrac) {
  const float e = clampf(edges.length() / (t.edgeWeight * t.maxForce + 1e-6f), 0.f, 1.f);
  if (edgeFrac) *edgeFrac = e;
  return others.limited(t.maxForce * boostMul * (1.f - 0.6f * e)) + edges;
}

void swim(Fish& f, Vec2 force, Vec2 nav, float e, float dt, const Tuning& t, const FxTuning& fx) {
  const float ms = t.maxSpeed[static_cast<int>(f.layer)];
  if (f.airborne) {
    f.vel += force * dt;
  } else {
    const bool boosted = f.boostT > 0.f;
    const Vec2 s = force.limited(t.maxForce * (boosted ? 4.f : 1.f) * (1.f + 2.f * e));
    const Vec2 want = f.vel + s * dt;
    if (f.leapT > 0.f) {
      f.vel = {want.x, std::min(want.y, -kLeapVy * 0.8f)};  // keep swimming up until it breaks the surface
    } else {
      const float cap = ms * (1.f + 1.4f * clampf(f.boostT / 0.7f, 0.f, 1.f));
      const float curA = std::atan2(f.vel.y, f.vel.x);
      const float wantA = want.length() > 1e-6f ? std::atan2(want.y, want.x) : curA;
      const float maxTurn = fx.maxTurnRate * (boosted ? 2.2f : 1.f) * (1.f + e) * dt;
      const float diff = wrapAngle(wantA - curA);

      // Reversals come from NAVIGATION only (walls + wander), debounced, with a cooldown.
      const float navLen = nav.length();
      const bool reversing = navLen > 0.5f * t.maxForce &&
                             std::fabs(wrapAngle(std::atan2(nav.y, nav.x) - curA)) > fx.flipAngle * kDeg &&
                             sgn(nav.x) != sgn(f.vel.x) && std::fabs(nav.x) > 0.3f * navLen;
      f.flipCooldown = std::max(0.f, f.flipCooldown - dt);
      f.reverseT = reversing ? f.reverseT + dt : 0.f;
      const bool canFlip = f.turnP == 0.f && (f.flipCooldown == 0.f || e > 0.3f);
      if (canFlip && f.reverseT > 0.25f) startFlip(f);

      if (f.flipTurn) {  // hold heading and slow down while the turn animation plays
        const float spd = std::max(ms * fx.minSpeedFrac * 0.5f, f.vel.length() * (1.f - 3.f * dt));
        f.vel = Vec2{std::cos(curA), std::sin(curA)} * spd;
      } else {
        float ang = curA + clampf(diff, -maxTurn, maxTurn);
        const bool right = std::cos(curA) >= 0.f;
        if ((std::cos(ang) >= 0.f) != right) {  // would cross vertical → flip-turn instead
          if (canFlip) startFlip(f);
          ang = curA;
        }
        const float pm = fx.maxPitch * kDeg;
        const float pitch = clampf(right ? wrapAngle(ang) : wrapAngle(kPi - ang), -pm, pm);
        ang = right ? pitch : kPi - pitch;
        const float spd = clampf(want.length(), ms * fx.minSpeedFrac, cap);
        f.vel = Vec2{std::cos(ang), std::sin(ang)} * spd;
      }
    }
  }
  f.pos += f.vel * dt;
  f.boostT = std::max(0.f, f.boostT - dt);
  f.leapT = std::max(0.f, f.leapT - dt);
  const float speed = f.vel.length();
  // wrapped: an ever-growing float phase stops changing after ~5 days of 24/7 running
  f.wavePhase = std::fmod(f.wavePhase + kTau * fx.bodyWaveHz * (0.4f + 0.9f * clampf(speed / ms, 0.f, 2.5f)) * dt, kTau);
}

void advanceTurn(Fish& f, float dt, const Tuning& t, const FxTuning& fx) {
  if (f.turnP > 0.f) {
    f.turnP += dt / fx.turnTime;
    if (!f.turnSwapped && f.turnP >= 0.5f) {
      f.vel.x = -f.vel.x;
      f.facingRight = f.vel.x > 0.f;
      f.wanderAngle = -f.wanderAngle;  // mirror the heading-relative wander too
      f.turnSwapped = true;
    }
    if (f.turnP >= 1.f) {
      f.turnP = 0.f;
      f.flipTurn = false;
    }
  } else if (!f.airborne && std::fabs(f.vel.x) > t.facingThreshold && (f.vel.x > 0.f) != f.facingRight) {
    // Safety net: velocity disagrees with the glyph → turn properly (never swap in one frame).
    f.vel.x = -f.vel.x;
    f.turnP = 1e-6f;
    f.flipTurn = true;
    f.turnSwapped = false;
  }
  const float tiltMax = (f.airborne || f.leapT > 0.f ? 35.f : fx.maxTilt) * kDeg;
  const float target = clampf(std::atan2(f.vel.y, std::fabs(f.vel.x) + 1e-6f) * 0.8f, -tiltMax, tiltMax);
  f.tilt += (target - f.tilt) * std::min(1.f, dt * 4.f);
}

}  // namespace tank
