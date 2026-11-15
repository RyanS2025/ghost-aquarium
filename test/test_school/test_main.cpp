#include <unity.h>

#include <cmath>

#include "school.h"

using tank::Fish;
using tank::FxTuning;
using tank::Layer;
using tank::Tuning;
using tank::Vec2;

namespace {
const FxTuning kFx{};

// A fish with its body points filled in, as Tank::step does before schooling runs.
Fish fish(Layer layer, Vec2 head, bool facingRight, Vec2 vel = {0, 0}) {
  Fish f{};
  f.layer = layer;
  f.pos = head;
  f.vel = vel;
  f.facingRight = facingRight;
  f.bodyN = static_cast<uint8_t>(tank::kGlyphLen[static_cast<int>(layer)]);
  tank::rigidBody(f, f.bodyN, tank::charAdvance(layer), 1.f, f.body);
  return f;
}
}  // namespace

void setUp() {}
void tearDown() {}

void test_school_lone_fish_is_zero() {
  Fish all[1] = {fish(Layer::Mid, {100, 120}, true, {20, 0})};
  const Vec2 r = tank::steerSchool(all[0], all, 1, Tuning{}, kFx);
  TEST_ASSERT_EQUAL_FLOAT(0.f, r.x);
  TEST_ASSERT_EQUAL_FLOAT(0.f, r.y);
}

void test_school_ignores_away_and_airborne() {
  Fish all[2] = {fish(Layer::Mid, {100, 120}, true), fish(Layer::Mid, {104, 120}, true)};
  all[1].away = true;
  Vec2 r = tank::steerSchool(all[0], all, 2, Tuning{}, kFx);
  TEST_ASSERT_EQUAL_FLOAT(0.f, r.x);
  all[1].away = false;
  all[1].airborne = true;
  r = tank::steerSchool(all[0], all, 2, Tuning{}, kFx);
  TEST_ASSERT_EQUAL_FLOAT(0.f, r.x);
}

// Neighbour's head is 78 px away (outside separation AND neighbour range) but its TAIL is
// ~8.5 px away, down-right of us. Only a body-aware rule notices it.
void test_school_separation_pushes_away_from_body() {
  Fish all[2] = {fish(Layer::Near, {100, 120}, true, {20, 0}), fish(Layer::Near, {178, 126}, true)};
  const Vec2 r = tank::steerSchool(all[0], all, 2, Tuning{}, kFx);
  TEST_ASSERT_TRUE(r.x < 0.f);
  TEST_ASSERT_TRUE(r.y < 0.f);
}

void test_school_cross_layer_neighbor_still_pushes() {
  Fish all[2] = {fish(Layer::Mid, {100, 120}, true), fish(Layer::Far, {108, 120}, false)};
  const Vec2 r = tank::steerSchool(all[0], all, 2, Tuning{}, kFx);
  TEST_ASSERT_TRUE(r.x < 0.f);  // away from the Far fish on our right
}

// Same-layer fish 8 px to the left, other-layer fish 8 px to the right: the same-layer push
// must win, so the net force points right.
void test_school_cross_layer_spacing_is_weaker_than_same_layer() {
  Fish all[3] = {fish(Layer::Mid, {100, 120}, true), fish(Layer::Mid, {92, 120}, true),
                 fish(Layer::Far, {108, 120}, false)};
  const Vec2 r = tank::steerSchool(all[0], all, 3, Tuning{}, kFx);
  TEST_ASSERT_TRUE(r.x > 0.f);
}

void test_school_alignment_matches_heading() {
  Tuning t;
  t.cohesionWeight = 0.f;
  t.separationWeight = 0.f;
  Fish all[2] = {fish(Layer::Mid, {100, 120}, true), fish(Layer::Mid, {100, 150}, true, {20, 0})};
  const Vec2 r = tank::steerSchool(all[0], all, 2, t, kFx);
  TEST_ASSERT_TRUE(r.x > 0.f);
}

void test_school_cohesion_pulls_toward_center() {
  Tuning t;
  t.alignmentWeight = 0.f;
  t.separationWeight = 0.f;
  Fish all[2] = {fish(Layer::Mid, {100, 120}, true), fish(Layer::Mid, {140, 120}, true)};
  const Vec2 r = tank::steerSchool(all[0], all, 2, t, kFx);
  TEST_ASSERT_TRUE(r.x > 0.f);
}

void test_school_far_neighbors_ignored() {
  Fish all[2] = {fish(Layer::Mid, {100, 120}, true), fish(Layer::Mid, {220, 120}, true)};
  const Vec2 r = tank::steerSchool(all[0], all, 2, Tuning{}, kFx);
  TEST_ASSERT_EQUAL_FLOAT(0.f, r.x);
  TEST_ASSERT_EQUAL_FLOAT(0.f, r.y);
}

// Review Focus #1: two fish exactly on top of each other must not produce NaN.
void test_school_coincident_fish_is_finite() {
  Fish all[2] = {fish(Layer::Mid, {100, 120}, true), fish(Layer::Mid, {100, 120}, true)};
  const Vec2 r = tank::steerSchool(all[0], all, 2, Tuning{}, kFx);
  TEST_ASSERT_TRUE(std::isfinite(r.x));
  TEST_ASSERT_TRUE(std::isfinite(r.y));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_school_lone_fish_is_zero);
  RUN_TEST(test_school_ignores_away_and_airborne);
  RUN_TEST(test_school_separation_pushes_away_from_body);
  RUN_TEST(test_school_cross_layer_neighbor_still_pushes);
  RUN_TEST(test_school_cross_layer_spacing_is_weaker_than_same_layer);
  RUN_TEST(test_school_alignment_matches_heading);
  RUN_TEST(test_school_cohesion_pulls_toward_center);
  RUN_TEST(test_school_far_neighbors_ignored);
  RUN_TEST(test_school_coincident_fish_is_finite);
  return UNITY_END();
}
