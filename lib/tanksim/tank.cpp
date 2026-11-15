#include "tank.h"

#include <algorithm>
#include <cmath>

#include "palette.h"
#include "school.h"

namespace tank {

namespace {
constexpr float kPi = 3.14159265358979f;
constexpr float kTau = 2.f * kPi;
constexpr float kLeapXMin = 70.f, kLeapXMax = 320.f - 70.f;

float clampf(float x, float lo, float hi) { return std::min(hi, std::max(lo, x)); }
float ms(const Tuning& t, const Fish& f) { return t.maxSpeed[static_cast<int>(f.layer)]; }
}  // namespace

Tank::Tank(const Tuning& t, const FxTuning& fx, uint32_t seed) : t_(t), fx_(fx), rng_(seed) {
  // Fish: each layer's fish get evenly spread preferred depths (stratified), so they use the
  // whole tank instead of piling into one band.
  for (int L = 0; L < kLayerCount; ++L) {
    const int cnt = std::min<int>(t_.fishPerLayer[L], kMaxFish - fishN_);
    int slots[kMaxFish];
    for (int i = 0; i < cnt; ++i) slots[i] = i;
    for (int i = cnt - 1; i > 0; --i) std::swap(slots[i], slots[randomIndex(rng_, i + 1)]);
    for (int i = 0; i < cnt; ++i) {
      Fish f{};
      f.layer = static_cast<Layer>(L);
      f.id = static_cast<uint8_t>(fishN_);
      f.homeBase = kHomeMin + (slots[i] + 0.5f) / std::max(1, cnt) * (kHomeMax - kHomeMin) + rng_.uniform(-8.f, 8.f);
      const float a = rng_.uniform(-0.4f, 0.4f) + (rng_.uniform(0.f, 1.f) < 0.5f ? 0.f : kPi);
      const float sp = t_.maxSpeed[L] * 0.6f;
      f.palette = static_cast<uint8_t>(randomIndex(rng_, kPaletteCount));
      f.pos = {rng_.uniform(t_.edgeMargin, kScreen.w - t_.edgeMargin), clampf(f.homeBase, kHomeMin, kHomeMax)};
      f.vel = {std::cos(a) * sp, std::sin(a) * sp};
      f.wanderAngle = 0.f;
      f.facingRight = std::cos(a) > 0.f;
      f.wavePhase = rng_.uniform(0.f, kTau);
      f.homeY = f.homeBase;
      updateBody(f);
      fish_[fishN_++] = f;
    }
  }
  buildSeabed(rng_, seabed_);
  for (Plankton& p : plankton_) {
    p.pos = {rng_.uniform(0.f, kScreen.w), rng_.uniform(kSurfaceY, kScreen.h)};
    p.vel = {rng_.uniform(-3.f, 3.f), rng_.uniform(-2.f, 2.f)};
    p.phase = rng_.uniform(0.f, kTau);
    p.rgb = rng_.uniform(0.f, 1.f) < 0.5f ? kPlanktonCyan : kReefColors[randomIndex(rng_, kReefColorCount)];
  }
}

int Tank::awayCount() const {
  int n = 0;
  for (int i = 0; i < fishN_; ++i) n += fish_[i].away ? 1 : 0;
  return n;
}

void Tank::updateBody(Fish& f) {
  const int L = static_cast<int>(f.layer);
  f.bodyN = static_cast<uint8_t>(kGlyphLen[L]);
  rigidBody(f, f.bodyN, charAdvance(f.layer), 1.f, f.body);
}

void Tank::splash(float x, float strength, uint32_t rgb) {
  const float s = clampf(strength / 60.f, 0.4f, 1.6f);
  const int drops = static_cast<int>(std::lround(fx_.splashDroplets * s));
  for (int i = 0; i < drops; ++i) {
    Particle p{};
    p.kind = ParticleKind::Drop;
    p.pos = {x + rng_.uniform(-5.f, 5.f), kSurfaceY - 1.f};
    p.vel = {rng_.uniform(-38.f, 38.f) * s, -rng_.uniform(35.f, 85.f) * s};
    p.life = rng_.uniform(0.7f, 1.3f);
    p.maxLife = 1.3f;
    p.rgb = rng_.uniform(0.f, 1.f) < 0.35f ? rgb : kScene.bubble;
    p.ch = rng_.uniform(0.f, 1.f) < 0.5f ? '\'' : '.';
    emit(p);
  }
  for (int i = 0, n = static_cast<int>(std::lround(6.f * s)); i < n; ++i) {
    Particle p{};
    p.kind = ParticleKind::Spark;
    p.pos = {x + rng_.uniform(-8.f, 8.f), kSurfaceY + rng_.uniform(-2.f, 2.f)};
    p.vel = {rng_.uniform(-25.f, 25.f), rng_.uniform(-10.f, 10.f)};
    p.life = rng_.uniform(0.25f, 0.5f);
    p.maxLife = 0.5f;
    p.rgb = kWhite;
    p.ch = '*';
    emit(p);
  }
  for (int i = 0, n = static_cast<int>(std::lround(5.f * s)); i < n; ++i) {
    Particle p{};
    p.kind = ParticleKind::Bubble;
    p.pos = {x + rng_.uniform(-6.f, 6.f), kSurfaceY + rng_.uniform(4.f, 14.f)};
    p.vel = {rng_.uniform(-6.f, 6.f), rng_.uniform(5.f, 25.f)};
    p.life = rng_.uniform(0.8f, 1.6f);
    p.maxLife = 1.6f;
    p.rgb = kScene.bubble;
    p.ch = rng_.uniform(0.f, 1.f) < 0.5f ? 'o' : '.';
    emit(p);
  }
  addRipple(ripples_, x, time_, 3.2f * s);
}

bool Tank::tryLeap() {
  int cands[kMaxFish], n = 0;
  for (int i = 0; i < fishN_; ++i) {
    const Fish& f = fish_[i];
    if (f.layer != Layer::Far && !f.away && !f.crossing && !f.airborne && f.leapT <= 0.f && f.turnP == 0.f &&
        f.pos.y < kSurfaceY + 70.f && f.pos.x > kLeapXMin && f.pos.x < kLeapXMax &&
        (f.pos.x < kScreen.w / 2.f) == (f.vel.x > 0.f))
      cands[n++] = i;
  }
  if (n == 0) return false;
  Fish& f = fish_[cands[randomIndex(rng_, n)]];
  const float dir = f.vel.x >= 0.f ? 1.f : -1.f;  // keep its direction: no glyph swap mid-leap
  f.leapT = 2.2f;
  f.boostT = 1.2f;
  f.vel = {dir * ms(t_, f) * 1.1f, -kLeapVy};
  return true;
}

// A fish that swam out of view comes back from a random side, at the entry depth with the
// most room from its layer-mates (best of 6), sometimes in a new color.
void Tank::reenter(Fish& f) {
  const float bodyLen = bodyLength(f.layer);
  const bool fromLeft = rng_.uniform(0.f, 1.f) < 0.5f;
  const float edgeX = fromLeft ? 0.f : kScreen.w;
  float bestY = kHomeMin, bestD = -1.f;
  for (int k = 0; k < 6; ++k) {
    const float y = rng_.uniform(kHomeMin, kHomeMax);
    float d = 1e30f;
    for (int i = 0; i < fishN_; ++i) {
      const Fish& o = fish_[i];
      if (&o != &f && !o.away && o.layer == f.layer) d = std::min(d, std::hypot(edgeX - o.pos.x, y - o.pos.y));
    }
    if (d > bestD) { bestD = d; bestY = y; }
  }
  const float sp = ms(t_, f) * rng_.uniform(0.55f, 0.8f);
  f.pos = {fromLeft ? -bodyLen + 2.f : kScreen.w + bodyLen - 2.f, bestY};
  f.vel = {fromLeft ? sp : -sp, 0.f};
  f.facingRight = fromLeft;
  f.tilt = 0.f; f.turnP = 0.f; f.flipTurn = false; f.turnSwapped = false;
  f.boostT = 0.f; f.leapT = 0.f; f.airborne = false;
  f.homeBase = bestY; f.homeY = bestY; f.wanderAngle = 0.f; f.reverseT = 0.f; f.flipCooldown = 2.f;
  f.away = false; f.crossing = true; f.leaving = false;
  if (rng_.uniform(0.f, 1.f) < 0.5f) f.palette = static_cast<uint8_t>(randomIndex(rng_, kPaletteCount));
  updateBody(f);
}

void Tank::step(float dt) {
  // Coming & going (ADR 0009): a fish may leave only through a RESERVED exit slot. It reserves
  // one (f.leaving) when it enters the side margin heading outward and fewer than maxAwayFrac
  // of the fish are away or committed. Fish without a slot see the side wall from the full
  // margin, so they have room to turn. (Switching walls on for everyone when the cap filled
  // let fish a few px from the edge drift out anyway: measured 6 away against a cap of 4.)
  const int cap = static_cast<int>(fx_.maxAwayFrac * fishN_);
  int committed = 0;
  for (int i = 0; i < fishN_; ++i) committed += (fish_[i].away || fish_[i].leaving) ? 1 : 0;
  for (int i = 0; i < fishN_; ++i) {
    Fish& f = fish_[i];
    if (f.away || f.airborne) continue;
    const bool nearLeft = f.pos.x < t_.edgeMargin, nearRight = f.pos.x > kScreen.w - t_.edgeMargin;
    const bool headingOut = (nearLeft && f.vel.x < 0.f) || (nearRight && f.vel.x > 0.f);
    if (!f.leaving && !f.crossing && headingOut && committed < cap) { f.leaving = true; ++committed; }
    if (f.leaving && !f.crossing && !nearLeft && !nearRight) f.leaving = false;  // turned back inside
    f.sideWalls = !f.leaving && !f.crossing;
  }

  // 1) all forces from one snapshot (order-independent)
  Vec2 force[kMaxFish], nav[kMaxFish];
  float edgeFrac[kMaxFish];
  for (int i = 0; i < fishN_; ++i) {
    Fish& f = fish_[i];
    force[i] = {}; nav[i] = {}; edgeFrac[i] = 0.f;
    if (f.away) continue;
    if (f.airborne) { force[i] = {0.f, kGravity}; continue; }
    const Vec2 a = steerSchool(f, fish_, fishN_, t_, fx_);
    const Vec2 b = steerEdges(f, t_);
    const Vec2 c = steerWander(f, rng_, dt, t_);
    const Vec2 level = steerLevel(f, rng_, dt, fx_);
    nav[i] = b + c;  // where it wants to go: walls + wander, never neighbours
    if (f.leapT > 0.f) { force[i] = {c.x * 0.3f, 0.f}; continue; }
    force[i] = prioritize(a + c + level, b, f.boostT > 0.f ? 4.f : 1.f, t_, &edgeFrac[i]);
  }

  // 2) integrate + events
  for (int i = 0; i < fishN_; ++i) {
    Fish& f = fish_[i];
    if (f.away) {
      f.returnT -= dt;
      if (f.returnT <= 0.f) reenter(f);
      continue;
    }
    swim(f, force[i], nav[i], edgeFrac[i], dt, t_, fx_);
    advanceTurn(f, dt, t_, fx_);
    updateBody(f);

    // crossing a side edge: away once fully out of view; entering fish ignore side walls
    const float bodyLen = bodyLength(f.layer);
    if (!f.crossing && !f.airborne && (f.pos.x < 0.f || f.pos.x > kScreen.w)) { f.crossing = true; f.leaving = true; }  // (normally already reserved)
    if (f.crossing && f.pos.x > bodyLen && f.pos.x < kScreen.w - bodyLen) { f.crossing = false; f.leaving = false; }
    if (f.crossing && (f.pos.x < -bodyLen - 4.f || f.pos.x > kScreen.w + bodyLen + 4.f)) {
      f.away = true;
      f.returnT = rng_.uniform(fx_.returnDelayMin, std::max(fx_.returnDelayMin, fx_.returnDelayMax));
      continue;
    }

    // surface crossings → splashes
    const uint32_t head = kFishPalette[f.palette % kPaletteCount][0];
    if (!f.airborne && f.pos.y < kSurfaceY && f.vel.y < 0.f) {
      f.airborne = true;
      splash(f.pos.x, std::fabs(f.vel.y), head);
    } else if (f.airborne && f.pos.y >= kSurfaceY && f.vel.y > 0.f) {
      f.airborne = false; f.leapT = 0.f;
      f.vel.y *= 0.35f; f.vel.x *= 0.8f; f.boostT = 0.4f;
      splash(f.pos.x, f.vel.y * 2.6f + 40.f, head);
    }

    // darts: a straight-ahead sprint, only into clear water, leaving a bubble spray
    const float m = ms(t_, f);
    const Vec2 u = f.vel.length() > 1e-3f ? f.vel.normalized() : Vec2{f.facingRight ? 1.f : -1.f, 0.f};
    const Vec2 ahead = f.pos + u * (m * 2.3f);
    const bool clearAhead = ahead.x > t_.edgeMargin && ahead.x < kScreen.w - t_.edgeMargin &&
                            ahead.y > kSurfaceY + t_.edgeMargin && ahead.y < kFloorY - t_.edgeMargin;
    if (clearAhead && !f.airborne && f.leapT <= 0.f && f.turnP == 0.f && rng_.uniform(0.f, 1.f) < fx_.dartPerSec * dt) {
      f.vel = u * (m * 2.3f);
      f.boostT = 0.7f;
      for (int k = 0; k < 7; ++k) {
        Particle p{};
        p.kind = ParticleKind::Bubble;
        p.pos = {f.pos.x - u.x * 10.f, f.pos.y};
        p.vel = {-u.x * rng_.uniform(10.f, 30.f), rng_.uniform(-12.f, 4.f)};
        p.life = rng_.uniform(0.8f, 1.6f);
        p.maxLife = 1.6f;
        p.rgb = kScene.bubble;
        p.ch = rng_.uniform(0.f, 1.f) < 0.6f ? '.' : 'o';
        emit(p);
      }
    }
  }
  if (rng_.uniform(0.f, 1.f) < fx_.leapPerSec * dt) tryLeap();

  // 3) ambient bubbles: rise, wobble (drawn), pop at the surface with a tiny ripple
  bubbleAcc_ += t_.bubblesPerSec * dt;
  while (bubbleAcc_ >= 1.f) {
    bubbleAcc_ -= 1.f;
    if (bubbles_.count < BubblePool::kCapacity) {
      const float x = rng_.uniform(8.f, kScreen.w - 8.f), phase = rng_.uniform(0.f, kTau);
      bubbles_.add({{x, kScreen.h + 2.f}, phase, t_.bubbleRise * rng_.uniform(0.8f, 1.2f)});
    }
  }
  for (Bubble& b : bubbles_) b.pos.y -= b.rise * dt;
  for (int i = bubbles_.count - 1; i >= 0; --i) {
    if (bubbles_.items[i].pos.y < kSurfaceY + 1.f) {
      addRipple(ripples_, bubbles_.items[i].pos.x, time_, 0.5f);
      bubbles_.removeAt(i);
    }
  }

  // 4) particles
  for (Particle& p : particles_) {
    if (p.kind == ParticleKind::Drop) {
      p.vel.y += kGravity * dt;
    } else if (p.kind == ParticleKind::Bubble) {
      p.vel.y -= 40.f * dt;
      p.vel.x *= 1.f - 1.5f * dt;
    } else {
      p.vel = p.vel * (1.f - 2.5f * dt);
    }
    p.pos += p.vel * dt;
    p.life -= dt;
    if (p.kind == ParticleKind::Drop && p.vel.y > 0.f && p.pos.y > kSurfaceY + 1.f) {
      p.life = 0.f;
      addRipple(ripples_, p.pos.x, time_, 0.35f);
    }
    if (p.kind == ParticleKind::Bubble && p.pos.y < kSurfaceY + 1.f) p.life = 0.f;
  }
  particles_.removeIf([](const Particle& p) {
    return p.life <= 0.f || p.pos.x < -10.f || p.pos.x > kScreen.w + 10.f || p.pos.y > kScreen.h + 10.f;
  });
  const float now = time_;
  ripples_.removeIf([now](const Ripple& r) { return now - r.t0 >= 3.5f; });

  // 5) plankton drift (wraps around)
  const float waterH = kScreen.h - kSurfaceY;
  for (Plankton& p : plankton_) {
    p.pos.x = std::fmod(p.pos.x + p.vel.x * dt + kScreen.w, kScreen.w);
    p.pos.y = kSurfaceY + std::fmod(p.pos.y - kSurfaceY + p.vel.y * dt + waterH, waterH);
  }
  time_ += dt;
}

}  // namespace tank
