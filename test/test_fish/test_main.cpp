#include <unity.h>

#include <cmath>

#include "fish.h"

using tank::Fish;
using tank::FxTuning;
using tank::Layer;
using tank::Tuning;
using tank::Vec2;

namespace {
constexpr float kDt = 1.f / 60.f;
constexpr float kDeg = 3.14159265f / 180.f;
const Tuning kT{};
const FxTuning kFx{};

Fish fishAt(Layer layer, Vec2 pos, Vec2 vel) {
  Fish f{};
  f.layer = layer;
  f.pos = pos;
  f.vel = vel;
  f.facingRight = vel.x >= 0;
  return f;
}

float angleBetween(Vec2 a, Vec2 b) {
  const float d = (a.x * b.x + a.y * b.y) / (a.length() * b.length());
  return std::acos(std::fmax(-1.f, std::fmin(1.f, d)));
}

void stepFish(Fish& f, Vec2 force, Vec2 nav, float edgeFrac) {
  tank::swim(f, force, nav, edgeFrac, kDt, kT, kFx);
  tank::advanceTurn(f, kDt, kT, kFx);
}
}  // namespace

void setUp() {}
void tearDown() {}

void test_body_length_and_advance_follow_font_scale() {
  for (int L = 0; L < tank::kLayerCount; ++L) {
    const auto layer = static_cast<Layer>(L);
    TEST_ASSERT_EQUAL_FLOAT(tank::kCellW * tank::kFontScale[L], tank::charAdvance(layer));
    TEST_ASSERT_EQUAL_FLOAT(tank::kGlyphLen[L] * tank::charAdvance(layer), tank::bodyLength(layer));
  }
}

void test_rigid_body_is_straight_row_behind_head() {
  const Fish f = fishAt(Layer::Near, {160, 120}, {20, 0});
  Vec2 pts[7];
  tank::rigidBody(f, 7, 12.f, 1.f, pts);
  for (int j = 0; j < 7; ++j) {
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 160.f - 12.f * j, pts[j].x);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 120.f, pts[j].y);
  }
}

void test_rigid_body_squeezes_about_middle() {
  const Fish f = fishAt(Layer::Near, {160, 120}, {20, 0});
  Vec2 pts[7];
  tank::rigidBody(f, 7, 12.f, 0.5f, pts);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 124.f, pts[3].x);  // middle unchanged
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 142.f, pts[0].x);  // head halfway to the middle
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 106.f, pts[6].x);  // tail halfway to the middle
}

void test_edges_zero_in_interior_and_sides_gated() {
  Fish f = fishAt(Layer::Mid, {tank::kScreen.w / 2.f, tank::kScreen.h / 2.f}, {20, 0});
  Vec2 e = tank::steerEdges(f, kT);
  TEST_ASSERT_EQUAL_FLOAT(0.f, e.x);
  TEST_ASSERT_EQUAL_FLOAT(0.f, e.y);

  f.pos = {5, tank::kScreen.h / 2.f};
  f.sideWalls = false;  // allowed to leave: no side wall
  TEST_ASSERT_EQUAL_FLOAT(0.f, tank::steerEdges(f, kT).x);
  f.sideWalls = true;
  TEST_ASSERT_TRUE(tank::steerEdges(f, kT).x > 0.f);
}

void test_edges_ignore_surface_while_leaping() {
  Fish f = fishAt(Layer::Mid, {tank::kScreen.w / 2.f, tank::kSurfaceY + kT.edgeMargin / 2.f}, {20, 0});
  TEST_ASSERT_TRUE(tank::steerEdges(f, kT).y > 0.f);  // pushed down, away from the surface
  f.leapT = 1.f;
  TEST_ASSERT_EQUAL_FLOAT(0.f, tank::steerEdges(f, kT).y);
}

void test_wander_never_points_backward() {
  Fish f = fishAt(Layer::Mid, {160, 120}, {20, 5});
  tank::Rng rng(3);
  for (int i = 0; i < 2000; ++i) {
    const Vec2 w = tank::steerWander(f, rng, kDt, kT);
    TEST_ASSERT_TRUE(angleBetween(w, f.vel) <= 30.1f * kDeg);
    TEST_ASSERT_FLOAT_WITHIN(1e-3f, kT.wanderWeight * kT.maxForce, w.length());
  }
}

void test_prioritize_scales_others_near_walls() {
  float e = -1.f;
  const Vec2 edges{kT.edgeWeight * kT.maxForce, 0.f};
  const Vec2 r = tank::prioritize({0.f, 1000.f}, edges, 1.f, kT, &e);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.f, e);
  TEST_ASSERT_TRUE(r.y <= 0.4f * kT.maxForce + 1e-3f);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, edges.x, r.x);

  const Vec2 calm = tank::prioritize({0.f, 1000.f}, {0.f, 0.f}, 1.f, kT, &e);
  TEST_ASSERT_EQUAL_FLOAT(0.f, e);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, kT.maxForce, calm.y);
}

void test_swim_turn_rate_is_limited() {
  Fish f = fishAt(Layer::Mid, {160, 120}, {20, 0});
  tank::swim(f, {0.f, -1000.f}, {0.f, 0.f}, 0.f, kDt, kT, kFx);
  const float turned = std::fabs(std::atan2(f.vel.y, f.vel.x));
  TEST_ASSERT_TRUE(turned <= kFx.maxTurnRate * kDt + 1e-5f);
}

void test_swim_pitch_is_limited() {
  Fish f = fishAt(Layer::Mid, {160, 120}, {20, 0});
  for (int i = 0; i < 200; ++i) {
    tank::swim(f, {0.f, -1000.f}, {0.f, 0.f}, 0.f, kDt, kT, kFx);
    f.pos = {160, 120};  // stay in open water
  }
  const float pitch = std::atan2(-f.vel.y, std::fabs(f.vel.x));
  TEST_ASSERT_TRUE(pitch <= kFx.maxPitch * kDeg + 1e-3f);
  TEST_ASSERT_TRUE(pitch > 0.f);
}

void test_swim_keeps_min_speed() {
  Fish f = fishAt(Layer::Near, {160, 120}, {1, 0});
  tank::swim(f, {0.f, 0.f}, {0.f, 0.f}, 0.f, kDt, kT, kFx);
  TEST_ASSERT_TRUE(f.vel.length() >= kFx.minSpeedFrac * kT.maxSpeed[2] - 1e-4f);
}

void test_reversal_starts_flip_after_debounce() {
  Fish f = fishAt(Layer::Mid, {160, 120}, {20, 0});
  const Vec2 back{-2.f * kT.maxForce, 0.f};
  for (int i = 0; i < 12; ++i) stepFish(f, back, back, 0.f);  // 0.2 s
  TEST_ASSERT_EQUAL_FLOAT(0.f, f.turnP);
  for (int i = 0; i < 6; ++i) stepFish(f, back, back, 0.f);   // by 0.3 s
  TEST_ASSERT_TRUE(f.turnP > 0.f || !f.facingRight);
}

void test_flip_reverses_at_midpoint() {
  Fish f = fishAt(Layer::Mid, {160, 120}, {20, 0});
  f.turnP = 1e-6f;
  f.flipTurn = true;
  int velFlips = 0, faceFlips = 0;
  bool prevRight = f.facingRight, prevVel = f.vel.x > 0;
  float turnAtFlip = -1.f;
  for (int i = 0; i < 40 && (f.turnP > 0.f || i == 0); ++i) {
    const float before = f.turnP;
    tank::advanceTurn(f, kDt, kT, kFx);
    if ((f.vel.x > 0) != prevVel) { ++velFlips; turnAtFlip = before; prevVel = f.vel.x > 0; }
    if (f.facingRight != prevRight) { ++faceFlips; prevRight = f.facingRight; }
  }
  TEST_ASSERT_EQUAL_INT(1, velFlips);
  TEST_ASSERT_EQUAL_INT(1, faceFlips);
  TEST_ASSERT_TRUE(turnAtFlip < 0.5f);                      // flipped on the step that crossed 0.5
  TEST_ASSERT_TRUE(turnAtFlip + kDt / kFx.turnTime >= 0.5f);
  TEST_ASSERT_EQUAL_FLOAT(0.f, f.turnP);
  TEST_ASSERT_FALSE(f.flipTurn);
  TEST_ASSERT_FALSE(f.facingRight);
}

void test_flip_cooldown_blocks_unless_wall() {
  Fish f = fishAt(Layer::Mid, {160, 120}, {20, 0});
  const Vec2 back{-2.f * kT.maxForce, 0.f};
  for (int i = 0; i < 60 && f.facingRight; ++i) stepFish(f, back, back, 0.f);
  TEST_ASSERT_FALSE(f.facingRight);
  for (int i = 0; i < 30 && f.turnP > 0.f; ++i) stepFish(f, {0, 0}, {0, 0}, 0.f);
  TEST_ASSERT_EQUAL_FLOAT(0.f, f.turnP);

  const Vec2 fwd{2.f * kT.maxForce, 0.f};  // now "back" relative to the new heading
  for (int i = 0; i < 30; ++i) stepFish(f, fwd, fwd, 0.f);  // 0.5 s, still in cooldown
  TEST_ASSERT_EQUAL_FLOAT(0.f, f.turnP);
  TEST_ASSERT_FALSE(f.facingRight);

  for (int i = 0; i < 20 && f.turnP == 0.f; ++i) stepFish(f, fwd, fwd, 0.5f);  // wall overrides
  TEST_ASSERT_TRUE(f.turnP > 0.f);
}

void test_safety_net_starts_flip_on_mismatch() {
  Fish f = fishAt(Layer::Mid, {160, 120}, {-10, 0});
  f.facingRight = true;  // disagrees with vx
  tank::advanceTurn(f, kDt, kT, kFx);
  TEST_ASSERT_TRUE(f.turnP > 0.f);
  TEST_ASSERT_TRUE(f.flipTurn);
  TEST_ASSERT_TRUE(f.facingRight);  // no instant swap: the turn animation does it
}

void test_tilt_limited_and_smoothed() {
  Fish f = fishAt(Layer::Mid, {160, 120}, {20, 20});
  tank::advanceTurn(f, kDt, kT, kFx);
  const float target = kFx.maxTilt * kDeg;  // a 45° dive clamps to maxTilt
  TEST_ASSERT_TRUE(f.tilt <= target * kDt * 4.f + 1e-5f);
  for (int i = 0; i < 300; ++i) tank::advanceTurn(f, kDt, kT, kFx);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, target, f.tilt);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_body_length_and_advance_follow_font_scale);
  RUN_TEST(test_rigid_body_is_straight_row_behind_head);
  RUN_TEST(test_rigid_body_squeezes_about_middle);
  RUN_TEST(test_edges_zero_in_interior_and_sides_gated);
  RUN_TEST(test_edges_ignore_surface_while_leaping);
  RUN_TEST(test_wander_never_points_backward);
  RUN_TEST(test_prioritize_scales_others_near_walls);
  RUN_TEST(test_swim_turn_rate_is_limited);
  RUN_TEST(test_swim_pitch_is_limited);
  RUN_TEST(test_swim_keeps_min_speed);
  RUN_TEST(test_reversal_starts_flip_after_debounce);
  RUN_TEST(test_flip_reverses_at_midpoint);
  RUN_TEST(test_flip_cooldown_blocks_unless_wall);
  RUN_TEST(test_safety_net_starts_flip_on_mismatch);
  RUN_TEST(test_tilt_limited_and_smoothed);
  return UNITY_END();
}
