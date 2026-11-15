#include <unity.h>

#include "vec2.h"

void setUp() {}
void tearDown() {}

// Review Focus #1: a zero vector must normalize to zero, never NaN.
void test_normalized_zero_is_zero() {
  auto n = tank::Vec2{0, 0}.normalized();
  TEST_ASSERT_EQUAL_FLOAT(0.f, n.x);
  TEST_ASSERT_EQUAL_FLOAT(0.f, n.y);
}

void test_normalized_has_unit_length() {
  // Braced init has a comma, so build it outside the macro (the preprocessor would split it).
  const tank::Vec2 v{3, 4};
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.f, v.normalized().length());
}

void test_limited_caps_length_keeps_direction() {
  auto l = tank::Vec2{30, 40}.limited(10.f);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 6.f, l.x);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 8.f, l.y);
}

void test_limited_leaves_short_vectors_alone() {
  auto l = tank::Vec2{1, 1}.limited(10.f);
  TEST_ASSERT_EQUAL_FLOAT(1.f, l.x);
  TEST_ASSERT_EQUAL_FLOAT(1.f, l.y);
}

void test_arithmetic_and_dot() {
  tank::Vec2 a{1, 2}, b{3, -4};
  auto s = a + b, d = a - b, m = a * 2.f, q = b / 2.f;
  TEST_ASSERT_EQUAL_FLOAT(4.f, s.x);  TEST_ASSERT_EQUAL_FLOAT(-2.f, s.y);
  TEST_ASSERT_EQUAL_FLOAT(-2.f, d.x); TEST_ASSERT_EQUAL_FLOAT(6.f, d.y);
  TEST_ASSERT_EQUAL_FLOAT(2.f, m.x);  TEST_ASSERT_EQUAL_FLOAT(4.f, m.y);
  TEST_ASSERT_EQUAL_FLOAT(1.5f, q.x); TEST_ASSERT_EQUAL_FLOAT(-2.f, q.y);
  TEST_ASSERT_EQUAL_FLOAT(-5.f, a.dot(b));
  a += b; TEST_ASSERT_EQUAL_FLOAT(4.f, a.x);
  a -= b; TEST_ASSERT_EQUAL_FLOAT(1.f, a.x);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_normalized_zero_is_zero);
  RUN_TEST(test_normalized_has_unit_length);
  RUN_TEST(test_limited_caps_length_keeps_direction);
  RUN_TEST(test_limited_leaves_short_vectors_alone);
  RUN_TEST(test_arithmetic_and_dot);
  return UNITY_END();
}
