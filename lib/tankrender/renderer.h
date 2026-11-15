#pragma once
// Renders a Tank into an RGB565 Frame: the C++ port of drawDevice() in prototype/index.html
// (DESIGN §7 draw order, ADR 0008). Hardware-free; the firmware pushes the frame to the panel.

#include <cstdint>

#include "frame.h"
#include "tank.h"

namespace tank::render {

// `background` is the pre-built glow (buildBackground); nullptr renders on plain black.
// Resets f.glyphs / f.writes, then fills the whole frame.
void renderFrame(const Tank& tk, const uint16_t* background, Frame& f);

}  // namespace tank::render
