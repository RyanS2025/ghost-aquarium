#pragma once
// Renders a Tank into an RGB565 Frame: the C++ port of drawDevice() in prototype/index.html
// (DESIGN §7 draw order, ADR 0008). Hardware-free; the firmware pushes the frame to the panel.

#include <cstdint>

#include "frame.h"
#include "tank.h"

namespace tank::render {

// Fast path (measured in Wokwi: rays were 47% of render time, rocks + sand 10%):
//  1. bakeStaticScenery() draws the rocks and sand into the background ONCE (they never move);
//  2. refreshBackdrop() rebuilds one of kRayBands row bands of "background + rays" per frame
//     (rays sway over ~30 s, so ~4 Hz updates move them < 0.5 px: invisible);
//  3. renderFrame(..., backdrop, f, kBackdropHasRaysAndScenery) skips those stages.
// Visual difference vs. the live path: rays now also light the rocks (drawn under them).
constexpr int kRayBands = 8;
enum RenderFlags : uint32_t { kLive = 0, kBackdropHasRaysAndScenery = 1 };

// `background` is the pre-built glow (buildBackground) or the backdrop; nullptr renders on black.
// Resets f.glyphs / f.writes, then fills the whole frame.
void renderFrame(const Tank& tk, const uint16_t* background, Frame& f, uint32_t flags = kLive);

// Draws the static seabed (rocks, sand) into a 320×240 background buffer, once.
void bakeStaticScenery(const Tank& tk, uint16_t* background);

// Rebuilds rows of `band` (0..kRayBands-1) in `backdrop` as `base` + the light rays at tk.animTime().
void refreshBackdrop(const Tank& tk, const uint16_t* base, uint16_t* backdrop, int band);

}  // namespace tank::render
