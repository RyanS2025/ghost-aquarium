#include <unity.h>

#include "tuning.h"

void setUp() {}
void tearDown() {}

// Pins the field-order contract with prototype/index.html's "Copy as C++" export.
// These initializers are in the prototype's export format, with the 128×128 defaults (ADR 0010;
// the prototype itself still opens with the 320×240 values, so re-scale before pasting).
// If someone reorders a struct, pasted tuned values would land in the wrong fields;
// these tests fail first.
constexpr tank::Tuning kExported{
    /* maxSpeed         */ {7.0f, 10.0f, 13.0f},
    /* maxForce         */ 18.0f,
    /* neighborRadius   */ 32.0f,
    /* separationRadius */ 14.0f,
    /* cohesionWeight   */ 0.35f,
    /* alignmentWeight  */ 1.0f,
    /* separationWeight */ 2.2f,
    /* wanderWeight     */ 0.6f,
    /* wanderJitter     */ 1.6f,
    /* edgeMargin       */ 18.0f,
    /* edgeWeight       */ 2.8f,
    /* facingThreshold  */ 1.0f,
    /* fishPerLayer     */ {2, 2, 1},
    /* bubblesPerSec    */ 0.5f,
    /* bubbleRise       */ 14.0f,
    /* bubbleWobbleAmp  */ 1.5f,
    /* bubbleWobbleHz   */ 1.2f,
    /* seaweedSwayAmp   */ 3.5f,
    /* seaweedHz        */ {0.31f, 0.83f},
};

constexpr tank::FxTuning kExportedFx{
    /* maxTurnRate    */ 1.6f,
    /* maxTilt        */ 8.0f,
    /* turnTime       */ 0.4f,
    /* flipAngle      */ 100.0f,
    /* maxPitch       */ 25.0f,
    /* returnDelayMin */ 2.0f,
    /* returnDelayMax */ 7.0f,
    /* maxAwayFrac    */ 0.35f,
    /* minSpeedFrac   */ 0.3f,
    /* bodyWaveAmp    */ 0.6f,
    /* bodyWaveHz     */ 1.4f,
    /* levelPull      */ 0.7f,
    /* depthPull      */ 0.8f,
    /* layerSpacing   */ 0.4f,
    /* vibrance       */ 1.0f,
    /* shimmer        */ 0.12f,
    /* bgGlow         */ 0.9f,
    /* rays           */ 0.7f,
    /* reef           */ 1,
    /* dartPerSec     */ 0.015f,
    /* leapPerSec     */ 0.08f,
    /* splashDroplets */ 8,
};

void test_core_initializer_matches_defaults_field_by_field() {
  const tank::Tuning d{};
  for (int i = 0; i < tank::kLayerCount; ++i) {
    TEST_ASSERT_EQUAL_FLOAT(d.maxSpeed[i], kExported.maxSpeed[i]);
    TEST_ASSERT_EQUAL_UINT8(d.fishPerLayer[i], kExported.fishPerLayer[i]);
  }
  TEST_ASSERT_EQUAL_FLOAT(d.maxForce, kExported.maxForce);
  TEST_ASSERT_EQUAL_FLOAT(d.neighborRadius, kExported.neighborRadius);
  TEST_ASSERT_EQUAL_FLOAT(d.separationRadius, kExported.separationRadius);
  TEST_ASSERT_EQUAL_FLOAT(d.cohesionWeight, kExported.cohesionWeight);
  TEST_ASSERT_EQUAL_FLOAT(d.alignmentWeight, kExported.alignmentWeight);
  TEST_ASSERT_EQUAL_FLOAT(d.separationWeight, kExported.separationWeight);
  TEST_ASSERT_EQUAL_FLOAT(d.wanderWeight, kExported.wanderWeight);
  TEST_ASSERT_EQUAL_FLOAT(d.wanderJitter, kExported.wanderJitter);
  TEST_ASSERT_EQUAL_FLOAT(d.edgeMargin, kExported.edgeMargin);
  TEST_ASSERT_EQUAL_FLOAT(d.edgeWeight, kExported.edgeWeight);
  TEST_ASSERT_EQUAL_FLOAT(d.facingThreshold, kExported.facingThreshold);
  TEST_ASSERT_EQUAL_FLOAT(d.bubblesPerSec, kExported.bubblesPerSec);
  TEST_ASSERT_EQUAL_FLOAT(d.bubbleRise, kExported.bubbleRise);
  TEST_ASSERT_EQUAL_FLOAT(d.bubbleWobbleAmp, kExported.bubbleWobbleAmp);
  TEST_ASSERT_EQUAL_FLOAT(d.bubbleWobbleHz, kExported.bubbleWobbleHz);
  TEST_ASSERT_EQUAL_FLOAT(d.seaweedSwayAmp, kExported.seaweedSwayAmp);
  TEST_ASSERT_EQUAL_FLOAT(d.seaweedHz[0], kExported.seaweedHz[0]);
  TEST_ASSERT_EQUAL_FLOAT(d.seaweedHz[1], kExported.seaweedHz[1]);
}

void test_fx_initializer_matches_defaults_field_by_field() {
  const tank::FxTuning d{};
  TEST_ASSERT_EQUAL_FLOAT(d.maxTurnRate, kExportedFx.maxTurnRate);
  TEST_ASSERT_EQUAL_FLOAT(d.maxTilt, kExportedFx.maxTilt);
  TEST_ASSERT_EQUAL_FLOAT(d.turnTime, kExportedFx.turnTime);
  TEST_ASSERT_EQUAL_FLOAT(d.flipAngle, kExportedFx.flipAngle);
  TEST_ASSERT_EQUAL_FLOAT(d.maxPitch, kExportedFx.maxPitch);
  TEST_ASSERT_EQUAL_FLOAT(d.returnDelayMin, kExportedFx.returnDelayMin);
  TEST_ASSERT_EQUAL_FLOAT(d.returnDelayMax, kExportedFx.returnDelayMax);
  TEST_ASSERT_EQUAL_FLOAT(d.maxAwayFrac, kExportedFx.maxAwayFrac);
  TEST_ASSERT_EQUAL_FLOAT(d.minSpeedFrac, kExportedFx.minSpeedFrac);
  TEST_ASSERT_EQUAL_FLOAT(d.bodyWaveAmp, kExportedFx.bodyWaveAmp);
  TEST_ASSERT_EQUAL_FLOAT(d.bodyWaveHz, kExportedFx.bodyWaveHz);
  TEST_ASSERT_EQUAL_FLOAT(d.levelPull, kExportedFx.levelPull);
  TEST_ASSERT_EQUAL_FLOAT(d.depthPull, kExportedFx.depthPull);
  TEST_ASSERT_EQUAL_FLOAT(d.layerSpacing, kExportedFx.layerSpacing);
  TEST_ASSERT_EQUAL_FLOAT(d.vibrance, kExportedFx.vibrance);
  TEST_ASSERT_EQUAL_FLOAT(d.shimmer, kExportedFx.shimmer);
  TEST_ASSERT_EQUAL_FLOAT(d.bgGlow, kExportedFx.bgGlow);
  TEST_ASSERT_EQUAL_FLOAT(d.rays, kExportedFx.rays);
  TEST_ASSERT_EQUAL_INT(d.reef, kExportedFx.reef);
  TEST_ASSERT_EQUAL_FLOAT(d.dartPerSec, kExportedFx.dartPerSec);
  TEST_ASSERT_EQUAL_FLOAT(d.leapPerSec, kExportedFx.leapPerSec);
  TEST_ASSERT_EQUAL_INT(d.splashDroplets, kExportedFx.splashDroplets);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_core_initializer_matches_defaults_field_by_field);
  RUN_TEST(test_fx_initializer_matches_defaults_field_by_field);
  return UNITY_END();
}
