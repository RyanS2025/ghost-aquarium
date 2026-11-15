#pragma once
// Renders a Tank into an RGB565 Frame: the C++ port of drawDevice() in prototype/index.html
// (DESIGN §7 draw order, ADR 0008). Hardware-free; the firmware pushes the frame to the panel.

#include <cstddef>
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
// Resets f.glyphs / f.writes, then fills the whole frame. (= buildDisplayList + rasterize.)
void renderFrame(const Tank& tk, const uint16_t* background, Frame& f, uint32_t flags = kLive);

// ---- banded rendering (ADR 0008 §7): build once, rasterise per band, DMA each band ----
// One glyph (ch != 0) or one pixel (ch == 0), already positioned and coloured.
struct DrawOp {
  int16_t x, y;      // glyph cell top-left / pixel, screen coordinates
  uint16_t color;    // RGB565
  char ch;
  uint8_t scale2;    // font scale × 2 (2, 3, 4 → 1×, 1.5×, 2×)
};
constexpr int kMaxOps = 1024;  // ~300 glyphs calm, ≤ 450 busy (measured): plenty of headroom

// Everything to draw this frame, in draw order. All colour/animation math happens while
// building it, once, so rasterising N bands doesn't repeat that work N times. ~8 KB: keep static.
struct DisplayList {
  DrawOp ops[kMaxOps];
  int n = 0;
  uint32_t dropped = 0;  // ops refused because the list was full (never expected)
  Frame::ClockFn clock = nullptr;
  uint32_t stageUs[Frame::kStageCount] = {};

  void blit(char ch, float cx, float cy, float s, uint16_t color);  // same rounding as Frame::blit
  void plot(float x, float y, uint16_t color);
};

void buildDisplayList(const Tank& tk, DisplayList& list, uint32_t flags = kLive);

// Draws rows [f.originY, f.originY + f.h) of the screen into f: background rows, live rays
// (unless baked), then every op, clipped. Stitched bands == renderFrame, byte for byte.
void rasterize(const Tank& tk, const DisplayList& list, const uint16_t* background, Frame& f, uint32_t flags = kLive);

// RGB565 → panel byte order in place, so DMA can send the band without CPU conversion.
void swapBytes(uint16_t* px, std::size_t n);

// Draws the static seabed (rocks, sand) into a 320×240 background buffer, once.
void bakeStaticScenery(const Tank& tk, uint16_t* background);

// Rebuilds rows of `band` (0..kRayBands-1) in `backdrop` as `base` + the light rays at tk.animTime().
void refreshBackdrop(const Tank& tk, const uint16_t* base, uint16_t* backdrop, int band);

}  // namespace tank::render
