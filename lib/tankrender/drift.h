#pragma once
// Burn-in protection for the OLED (ADR 0010): the whole image wanders over a 5×5 px square, one
// pixel per step along a snake path, then back again (ping-pong), so it never jumps. The firmware
// advances one step every few minutes and pushes the frame at its home position + this offset.

#include <cstdint>

namespace tank::render {

constexpr int kDriftRadius = 2;                          // px in each direction
constexpr int kDriftSide = 2 * kDriftRadius + 1;         // 5
constexpr int kDriftPositions = kDriftSide * kDriftSide; // 25
constexpr uint32_t kDriftCycle = 2 * (kDriftPositions - 1);  // 48 steps out and back

struct DriftOffset {
  int dx, dy;
};

constexpr DriftOffset driftOffset(uint32_t step) {
  const uint32_t k = step % kDriftCycle;
  const int i = static_cast<int>(k < kDriftPositions ? k : kDriftCycle - k);  // ping-pong
  const int row = i / kDriftSide, col = row % 2 ? kDriftSide - 1 - i % kDriftSide : i % kDriftSide;  // snake
  return {col - kDriftRadius, row - kDriftRadius};
}

}  // namespace tank::render
