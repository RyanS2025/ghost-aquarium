#include <unity.h>

#include "rng.h"

void setUp() {}
void tearDown() {}

void test_rng_same_seed_same_sequence() {
  tank::Rng a(42), b(42);
  for (int i = 0; i < 100; ++i) TEST_ASSERT_EQUAL_UINT32(a.next(), b.next());
}

// Review Focus #5: raw xorshift seeded with 0 returns 0 forever.
void test_rng_seed_zero_still_varies() {
  tank::Rng r(0);
  uint32_t a = r.next(), b = r.next();
  TEST_ASSERT_NOT_EQUAL(0u, a);
  TEST_ASSERT_NOT_EQUAL(a, b);
}

void test_rng_uniform_in_range() {
  tank::Rng r(7);
  for (int i = 0; i < 10000; ++i) {
    float x = r.uniform(-2.f, 5.f);
    TEST_ASSERT_TRUE(x >= -2.f);
    TEST_ASSERT_TRUE(x < 5.f);
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_rng_same_seed_same_sequence);
  RUN_TEST(test_rng_seed_zero_still_varies);
  RUN_TEST(test_rng_uniform_in_range);
  return UNITY_END();
}
