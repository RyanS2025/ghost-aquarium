#pragma once
// The water glow, built once at startup (ADR 0008): teal under the surface → indigo → violet,
// faded to PURE BLACK at all four edges (the screen outline never shows on the reflector), and
// quantised to RGB565 with 4×4 Bayer dithering (no banding, zero per-frame cost).

#include <cstdint>

#include "tuning.h"

namespace tank::render {

// `out` must hold 320×240 pixels.
void buildBackground(uint16_t* out, const FxTuning& fx);

}  // namespace tank::render
