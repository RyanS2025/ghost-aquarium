#include <unity.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "palette.h"
#include "stepper.h"
#include "tank.h"

using tank::FxTuning;
using tank::Tank;
using tank::Tuning;

namespace {
constexpr float kDt = 1.f / 60.f;
const Tuning kT{};
const FxTuning kFx{};

int onScreen(const Tank& tk) {
  int n = 0;
  for (int i = 0; i < tk.fishCount(); ++i) n += tk.fish()[i].away ? 0 : 1;
  return n;
}
}  // namespace

void setUp() {}
void tearDown() {}

void test_tank_spawns_per_layer_counts_and_stratified_depths() {
  Tank tk(kT, kFx, 42);
  TEST_ASSERT_EQUAL_INT(kT.fishPerLayer[0] + kT.fishPerLayer[1] + kT.fishPerLayer[2], tk.fishCount());
  for (int L = 0; L < tank::kLayerCount; ++L) {
    std::vector<float> homes;
    for (int i = 0; i < tk.fishCount(); ++i)
      if (static_cast<int>(tk.fish()[i].layer) == L) homes.push_back(tk.fish()[i].homeBase);
    TEST_ASSERT_EQUAL_INT(kT.fishPerLayer[L], static_cast<int>(homes.size()));
    std::sort(homes.begin(), homes.end());
    const float band = (tank::kHomeMax - tank::kHomeMin) / homes.size();
    for (size_t k = 1; k < homes.size(); ++k) TEST_ASSERT_TRUE(homes[k] - homes[k - 1] >= band - 8.1f);  // ±4 px jitter each
  }
}

void test_stepper_runs_whole_steps() {
  tank::FixedStepper s(1.f / 60.f, 5);
  TEST_ASSERT_EQUAL_INT(2, s.advance(0.034f));
}

// Review Focus #2
void test_stepper_caps_steps_and_drops_excess() {
  tank::FixedStepper s(1.f / 60.f, 5);
  TEST_ASSERT_EQUAL_INT(5, s.advance(1.0f));
  TEST_ASSERT_EQUAL_INT(0, s.advance(0.0f));
}

void test_same_seed_is_deterministic() {
  Tank a(kT, kFx, 42), b(kT, kFx, 42);
  for (int i = 0; i < 600; ++i) { a.step(kDt); b.step(kDt); }
  for (int i = 0; i < a.fishCount(); ++i) {
    TEST_ASSERT_EQUAL_FLOAT(a.fish()[i].pos.x, b.fish()[i].pos.x);
    TEST_ASSERT_EQUAL_FLOAT(a.fish()[i].pos.y, b.fish()[i].pos.y);
  }
  TEST_ASSERT_EQUAL_INT(a.particles().count, b.particles().count);
}

void test_fish_leave_and_return() {
  Tank tk(kT, kFx, 42);
  std::vector<float> awaySince(tk.fishCount(), -1.f);
  int returns = 0;
  float longest = 0.f;
  for (int i = 0; i < 60 * 60 * 3; ++i) {
    tk.step(kDt);
    for (int k = 0; k < tk.fishCount(); ++k) {
      const bool away = tk.fish()[k].away;
      if (away && awaySince[k] < 0.f) awaySince[k] = static_cast<float>(tk.time());
      if (!away && awaySince[k] >= 0.f) { ++returns; longest = std::max(longest, static_cast<float>(tk.time()) - awaySince[k]); awaySince[k] = -1.f; }
    }
  }
  TEST_ASSERT_TRUE(returns >= 1);
  TEST_ASSERT_TRUE(longest <= kFx.returnDelayMax + 0.1f);
}

void test_away_cap_holds() {
  Tank tk(kT, kFx, 9);
  const int cap = static_cast<int>(kFx.maxAwayFrac * tk.fishCount());
  for (int i = 0; i < 60 * 60 * 5; ++i) {
    tk.step(kDt);
    TEST_ASSERT_TRUE(tk.awayCount() <= cap);  // exact: exits are reserved in advance
  }
}

void test_leap_goes_airborne_and_splashes() {
  Tank tk(kT, kFx, 42);
  bool leapt = false;
  for (int i = 0; i < 60 * 120 && !leapt; ++i) { tk.step(kDt); leapt = tk.tryLeap(); }
  TEST_ASSERT_TRUE(leapt);
  bool airborne = false;
  int particles = 0;
  for (int i = 0; i < 60 && !airborne; ++i) {
    tk.step(kDt);
    for (int k = 0; k < tk.fishCount(); ++k) airborne |= tk.fish()[k].airborne;
    particles = tk.particles().count;
  }
  TEST_ASSERT_TRUE(airborne);
  TEST_ASSERT_TRUE(particles > 0);
}

// Review Focus #5
void test_pools_drop_when_full() {
  Tank tk(kT, kFx, 42);
  for (int i = 0; i < 20; ++i) tk.splash(tank::kScreen.w / 2.f, 200.f, tank::kWhite);
  TEST_ASSERT_EQUAL_INT(tank::kMaxParticles, tk.particles().count);
  TEST_ASSERT_TRUE(tk.ripples().count <= tank::kMaxRipples);
  for (int i = 0; i < 600; ++i) tk.step(kDt);  // and they drain normally
  TEST_ASSERT_TRUE(tk.particles().count < tank::kMaxParticles);
}

// Review Focus #3: outside a flip-turn, the glyph always faces the direction of travel.
void test_soak_facing_always_matches() {
  Tank tk(kT, kFx, 1234);
  for (int i = 0; i < 100000; ++i) {
    tk.step(kDt);
    for (int k = 0; k < tk.fishCount(); ++k) {
      const auto& f = tk.fish()[k];
      if (f.away || f.airborne || f.turnP > 0.f || std::fabs(f.vel.x) <= 0.5f) continue;
      TEST_ASSERT_EQUAL(f.vel.x > 0.f, f.facingRight);
    }
  }
}

// Review Focus #4: 24/7 behaviour over ~28 simulated minutes.
void test_soak_100k_steps_finite_bounded_and_populated() {
  Tank tk(kT, kFx, 42);
  std::vector<float> awaySince(tk.fishCount(), -1.f);
  int minOn = tk.fishCount();
  for (int i = 0; i < 100000; ++i) {
    tk.step(kDt);
    minOn = std::min(minOn, onScreen(tk));
    for (int k = 0; k < tk.fishCount(); ++k) {
      const auto& f = tk.fish()[k];
      TEST_ASSERT_TRUE(std::isfinite(f.pos.x) && std::isfinite(f.pos.y));
      TEST_ASSERT_TRUE(std::isfinite(f.vel.x) && std::isfinite(f.vel.y));
      if (!f.away) TEST_ASSERT_TRUE(f.pos.y >= -60.f && f.pos.y <= tank::kScreen.h);
      if (f.away && awaySince[k] < 0.f) awaySince[k] = static_cast<float>(tk.time());
      if (!f.away) awaySince[k] = -1.f;
      if (f.away) TEST_ASSERT_TRUE(tk.time() - awaySince[k] <= kFx.returnDelayMax + 0.1f);
    }
  }
  TEST_ASSERT_TRUE(minOn >= tk.fishCount() - static_cast<int>(kFx.maxAwayFrac * tk.fishCount()) - 1);
}

// Review (Critical): a float clock accumulating 1/60 s stalls at 524288 s (~6 days of 24/7):
// ripples never expired and every animation froze. Time must keep advancing exactly.
void test_clock_keeps_running_after_six_days() {
  Tank tk(kT, kFx, 42);
  tk.setClockForTest(6.0 * 86400.0);
  const double t0 = tk.time();
  const float a0 = tk.animTime();
  for (int i = 0; i < 600; ++i) tk.step(kDt);
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 10.f, static_cast<float>(tk.time() - t0));
  TEST_ASSERT_FLOAT_WITHIN(0.02f, 10.f, tk.animTime() - a0);
}

void test_ripples_expire_after_six_days() {
  Tank tk(kT, kFx, 42);
  tk.setClockForTest(6.0 * 86400.0);
  tk.splash(tank::kScreen.w / 2.f, 120.f, tank::kWhite);
  for (int i = 0; i < 60 * 4; ++i) tk.step(kDt);
  for (const auto& r : tk.ripples()) TEST_ASSERT_TRUE(r.age < 3.5f);
  TEST_ASSERT_TRUE(tk.ripples().count < tank::kMaxRipples);
}

void test_phases_stay_bounded() {
  Tank tk(kT, kFx, 5);
  for (int i = 0; i < 100000; ++i) tk.step(kDt);
  for (int k = 0; k < tk.fishCount(); ++k) {
    const auto& f = tk.fish()[k];
    TEST_ASSERT_TRUE(f.wavePhase >= 0.f && f.wavePhase < 6.2832f);
    TEST_ASSERT_TRUE(std::fabs(f.wanderAngle) <= 3.1416f);
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_clock_keeps_running_after_six_days);
  RUN_TEST(test_ripples_expire_after_six_days);
  RUN_TEST(test_phases_stay_bounded);
  RUN_TEST(test_tank_spawns_per_layer_counts_and_stratified_depths);
  RUN_TEST(test_stepper_runs_whole_steps);
  RUN_TEST(test_stepper_caps_steps_and_drops_excess);
  RUN_TEST(test_same_seed_is_deterministic);
  RUN_TEST(test_fish_leave_and_return);
  RUN_TEST(test_away_cap_holds);
  RUN_TEST(test_leap_goes_airborne_and_splashes);
  RUN_TEST(test_pools_drop_when_full);
  RUN_TEST(test_soak_facing_always_matches);
  RUN_TEST(test_soak_100k_steps_finite_bounded_and_populated);
  return UNITY_END();
}
