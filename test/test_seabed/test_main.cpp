#include <unity.h>

#include <cmath>
#include <cstring>

#include "seabed.h"

using tank::Seabed;

void setUp() {}
void tearDown() {}

// Overlapping ASCII rocks/coral/seaweed read as a jumble; packing must make overlap impossible.
void test_seabed_pieces_never_overlap_for_50_seeds() {
  for (uint32_t seed = 1; seed <= 50; ++seed) {
    tank::Rng rng(seed);
    Seabed s;
    tank::buildSeabed(rng, s);
    struct Piece { float x, half; } p[tank::kRocks + tank::kSeaweeds + tank::kCorals];
    int n = 0;
    for (const auto& r : s.rocks) p[n++] = {r.x, r.w / 2.f};
    for (const auto& w : s.seaweed) p[n++] = {w.x, 4.f};
    for (const auto& c : s.coral) p[n++] = {c.x, 15.f};
    for (int i = 0; i < n; ++i)
      for (int j = i + 1; j < n; ++j) TEST_ASSERT_TRUE(std::fabs(p[i].x - p[j].x) - p[i].half - p[j].half >= -1e-3f);
    for (int i = 0; i < n; ++i) {
      TEST_ASSERT_TRUE(p[i].x - p[i].half >= 0.f);
      TEST_ASSERT_TRUE(p[i].x + p[i].half <= tank::kScreen.w);
    }
  }
}

void test_rock_cells_follow_outline() {
  tank::Rng rng(7);
  Seabed s;
  tank::buildSeabed(rng, s);
  for (const auto& r : s.rocks) {
    TEST_ASSERT_TRUE(r.cellN > 0 && r.cellN <= tank::kMaxRockCells);
    for (int i = 0; i < r.cellN; ++i) {
      const auto& c = r.cells[i];
      if (c.top) TEST_ASSERT_NOT_NULL(std::strchr("/\\-", c.ch));
      if (c.moss) TEST_ASSERT_TRUE(c.top);
      TEST_ASSERT_TRUE(c.y < 0.f && c.y >= -(r.h + tank::kCellH));
      TEST_ASSERT_TRUE(std::fabs(c.x) <= r.w / 2.f);
    }
  }
  TEST_ASSERT_TRUE(s.rocks[0].star >= 0);  // the biggest rock always has a starfish
  TEST_ASSERT_TRUE(s.rocks[0].h >= s.rocks[1].h);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_seabed_pieces_never_overlap_for_50_seeds);
  RUN_TEST(test_rock_cells_follow_outline);
  return UNITY_END();
}
