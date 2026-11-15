#include <unity.h>

#include "tuning.h"

void setUp() {}
void tearDown() {}

// Pins the field-order contract with prototype/index.html's "Copy as C++" export:
// this initializer is exactly what the prototype emits for the DESIGN §6 defaults.
// If someone reorders tank::Tuning, pasted tuned values would land in the wrong fields;
// this test fails first.
constexpr tank::Tuning kExported{
    /* maxSpeed         */ {18.0f, 28.0f, 40.0f},
    /* maxForce         */ 30.0f,
    /* neighborRadius   */ 40.0f,
    /* separationRadius */ 14.0f,
    /* cohesionWeight   */ 0.6f,
    /* alignmentWeight  */ 1.0f,
    /* separationWeight */ 1.6f,
    /* wanderWeight     */ 0.8f,
    /* wanderJitter     */ 2.0f,
    /* edgeMargin       */ 24.0f,
    /* edgeWeight       */ 2.5f,
    /* facingThreshold  */ 3.0f,
    /* fishPerLayer     */ {6, 5, 3},
    /* bubblesPerSec    */ 1.5f,
    /* bubbleRise       */ 25.0f,
    /* bubbleWobbleAmp  */ 2.0f,
    /* bubbleWobbleHz   */ 1.2f,
    /* seaweedSwayAmp   */ 6.0f,
    /* seaweedHz        */ {0.31f, 0.83f},
};

void test_exported_initializer_matches_defaults_field_by_field() {
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

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_exported_initializer_matches_defaults_field_by_field);
  return UNITY_END();
}
