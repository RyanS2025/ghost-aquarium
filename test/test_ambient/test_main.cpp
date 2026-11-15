#include <unity.h>

#include <cmath>

#include "ambient.h"

using tank::Pool;
using tank::Tuning;

void setUp() {}
void tearDown() {}

void test_pool_add_fails_when_full_and_remove_compacts() {
  Pool<int, 4> p;
  for (int i = 0; i < 4; ++i) TEST_ASSERT_TRUE(p.add(i));
  TEST_ASSERT_FALSE(p.add(99));  // full: dropped, never overflows
  TEST_ASSERT_EQUAL_INT(4, p.count);
  p.removeIf([](int v) { return v % 2 == 0; });
  TEST_ASSERT_EQUAL_INT(2, p.count);
  TEST_ASSERT_EQUAL_INT(1, p.items[0]);
  TEST_ASSERT_EQUAL_INT(3, p.items[1]);
}

void test_seaweed_root_fixed_and_sway_bounded() {
  const Tuning t;
  for (float s = 0.f; s < 60.f; s += 0.05f) {
    TEST_ASSERT_EQUAL_FLOAT(0.f, tank::seaweedSway(0.f, s, 1.3f, t));
    TEST_ASSERT_TRUE(std::fabs(tank::seaweedSway(1.f, s, 1.3f, t)) <= t.seaweedSwayAmp + 1e-4f);
  }
}

void test_surface_flat_without_ripples_and_bounded_with() {
  tank::RipplePool r;
  for (float x = 0.f; x < tank::kScreen.w; x += 7.f) {
    const float y = tank::surfaceAt(r, x, 3.7f);
    TEST_ASSERT_TRUE(std::fabs(y - tank::kSurfaceY) <= 1.8f + 1e-4f);
  }
  tank::addRipple(r, tank::kScreen.w / 2.f, 3.f);
  tank::addRipple(r, tank::kScreen.w / 4.f, 2.f);
  r.items[0].age = 1.f;  // ripples spread with age
  r.items[1].age = 0.5f;
  for (float x = 0.f; x < tank::kScreen.w; x += 7.f) {
    const float y = tank::surfaceAt(r, x, 1.f);
    TEST_ASSERT_TRUE(std::fabs(y - tank::kSurfaceY) <= 1.8f + 5.f + 1e-4f);
  }
}

void test_ripples_drop_oldest_when_full() {
  tank::RipplePool r;
  for (int i = 0; i < tank::kMaxRipples + 1; ++i) tank::addRipple(r, static_cast<float>(i), 1.f);
  TEST_ASSERT_EQUAL_INT(tank::kMaxRipples, r.count);
  TEST_ASSERT_EQUAL_FLOAT(1.f, r.items[0].x);  // ripple 0 was dropped
}

// The renderer computes the whole surface in one pass (recurrences, per-ripple constants);
// it must match surfaceAt point for point.
void test_surface_profile_matches_surface_at() {
  tank::RipplePool r;
  tank::addRipple(r, 40.f, 3.f);
  tank::addRipple(r, 100.f, 2.f);
  tank::addRipple(r, 15.f, 1.f);
  r.items[0].age = 1.2f;
  r.items[1].age = 0.4f;
  r.items[2].age = 3.1f;
  constexpr int n = 22;  // x = 3 … 129: one point past the 128-px screen
  float ys[n];
  const float t = 1234.567f;
  tank::surfaceProfile(r, t, 3.f, 6.f, n, ys);
  for (int k = 0; k < n; ++k) TEST_ASSERT_FLOAT_WITHIN(2e-3f, tank::surfaceAt(r, 3.f + 6.f * k, t), ys[k]);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_surface_profile_matches_surface_at);
  RUN_TEST(test_pool_add_fails_when_full_and_remove_compacts);
  RUN_TEST(test_seaweed_root_fixed_and_sway_bounded);
  RUN_TEST(test_surface_flat_without_ripples_and_bounded_with);
  RUN_TEST(test_ripples_drop_oldest_when_full);
  return UNITY_END();
}
