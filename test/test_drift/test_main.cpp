#include <unity.h>

#include <cstdlib>
#include <set>
#include <utility>

#include "drift.h"

using tank::render::driftOffset;
using tank::render::kDriftCycle;
using tank::render::kDriftPositions;
using tank::render::kDriftRadius;

void setUp() {}
void tearDown() {}

// The image must never jump: every step moves exactly one pixel along one axis, including the
// wrap from the end of a cycle back to its start.
void test_drift_moves_one_pixel_per_step() {
  for (uint32_t s = 0; s < 3 * kDriftCycle; ++s) {
    const auto a = driftOffset(s), b = driftOffset(s + 1);
    TEST_ASSERT_EQUAL_INT(1, std::abs(a.dx - b.dx) + std::abs(a.dy - b.dy));
  }
}

void test_drift_stays_within_radius_and_visits_every_position() {
  std::set<std::pair<int, int>> seen;
  for (uint32_t s = 0; s < kDriftCycle; ++s) {
    const auto o = driftOffset(s);
    TEST_ASSERT_TRUE(std::abs(o.dx) <= kDriftRadius && std::abs(o.dy) <= kDriftRadius);
    seen.insert({o.dx, o.dy});
  }
  TEST_ASSERT_EQUAL_INT(kDriftPositions, static_cast<int>(seen.size()));
}

void test_drift_is_periodic() {
  for (uint32_t s = 0; s < kDriftCycle; ++s) {
    TEST_ASSERT_EQUAL_INT(driftOffset(s).dx, driftOffset(s + kDriftCycle).dx);
    TEST_ASSERT_EQUAL_INT(driftOffset(s).dy, driftOffset(s + kDriftCycle).dy);
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_drift_moves_one_pixel_per_step);
  RUN_TEST(test_drift_stays_within_radius_and_visits_every_position);
  RUN_TEST(test_drift_is_periodic);
  return UNITY_END();
}
