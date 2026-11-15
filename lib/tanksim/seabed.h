#pragma once
// Seabed: ASCII rocks, seaweed, coral, sand, light rays (DESIGN §4). Laid out once per seed.

#include <cstdint>

#include "rng.h"
#include "tuning.h"

namespace tank {

constexpr int kRocks = 4, kSeaweeds = 4, kCorals = 3, kRays = 4;
constexpr int kMaxRockCells = 64, kMaxSand = 56;

struct RockCell {
  float x, y;  // relative to the rock's base centre; y is up-negative
  char ch;
  bool top, moss;
};

struct Rock {
  float x = 0, w = 0, h = 0;
  uint8_t tone = 0;        // index into kRockTones
  float lMul = 1, hueShift = 0;
  int8_t star = -1;        // reef color index of a starfish, or -1
  RockCell cells[kMaxRockCells]{};
  int cellN = 0;
};

struct Seaweed {
  float x, height, phase, tint;
};

struct CoralTemplate {
  const char* rows[4];
  int rowN;
  bool flex, glow;
};
constexpr int kCoralTemplates = 5;
extern const CoralTemplate kCoral[kCoralTemplates];

struct Coral {
  float x;
  uint8_t tpl, color;
  float phase;
};

struct Sand {
  float x, y;
  char ch;
  int8_t pebble;  // reef color index, or -1 for plain sand
};

struct Ray {
  float x, w, slant, phase;
};

struct Seabed {
  Rock rocks[kRocks];          // sorted biggest first
  Seaweed seaweed[kSeaweeds];
  Coral coral[kCorals];
  Sand sand[kMaxSand];
  int sandN = 0;
  Ray rays[kRays];
};

// Packs rocks, seaweed and coral left→right with even gaps (never overlapping), in a
// shuffled order, then sand and rays. Port of the prototype's makeTank seabed section.
void buildSeabed(Rng& rng, Seabed& out);

// One ASCII boulder of size w×h centred at x (character grid from a jittered outline).
void buildRock(Rng& rng, float x, float w, float h, Rock& out);

}  // namespace tank
