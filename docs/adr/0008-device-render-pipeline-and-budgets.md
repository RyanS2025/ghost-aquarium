# 0008. Device render pipeline and per-frame budgets

- **Status:** Accepted. Frame-time numbers are estimates until Phase 1 hardware measurement.
- **Date:** 2026-11-14
- **Refines:** [ADR 0003](0003-rendering-lovyangfx-sprite.md)

## Context

The look was designed in a browser that can do things the device can't: anti-aliased vector fonts, 24-bit color, alpha blending and 120 fps. Tuning by eye there produces a look the ESP32-S3 + ILI9341 can't show. The hardware is:

- **Panel:** 320×240, RGB565 (16-bit) only, fed over SPI. A full frame is 153,600 bytes, about **31 ms at 40 MHz**, so roughly **30 fps** for full-frame updates.
- **MCU:** ESP32-S3 at 240 MHz with a single-precision FPU, 512 KB internal SRAM (DMA-capable) and 8 MB octal PSRAM (larger, slower).
- **Text:** LovyanGFX's built-in 6×8 bitmap font (5×7 glyphs), scaled with `setTextSize`. No anti-aliasing.

## Decision

1. **The prototype's default "Device preview" is the reference renderer.** `prototype/index.html` draws into a `Uint16Array(320*240)` RGB565 framebuffer using only operations the firmware will use:
   - copy a pre-rendered background
   - blit 5×7 glyphs at whole-pixel positions
   - plot single pixels
   - saturating RGB565 adds for the light rays

   It caps rendering at 30 fps while the simulation keeps stepping at 60 Hz. The C++ renderer (`src/render/`) ports this function (`drawDevice`) directly.
2. **The background is baked once, with ordered dithering.** The water glow (black at every edge) is computed at startup and quantized to RGB565 with a 4×4 Bayer matrix. RGB565 has only 32 levels of red and blue, so dark gradients band without dithering. Dithering is free per frame, and at 0.18 mm per pixel the pattern isn't visible. Each frame starts with one copy of this 150 KB image.
3. **Fixed pools, sized from measurement.** All are allocated at boot, with nothing allocated per frame (DESIGN §10):

   | Pool | Cap | Measured peak (tap every 0.5 s + leaps) |
   |---|---|---|
   | Fish | 12 | — |
   | Particles | 96 | 50 |
   | Ambient bubbles | 24 | 9 |
   | Surface ripples | 12 | 5 |
   | Plankton | 32 | — |

4. **Measured per-frame drawing workload (device preview):** about **290 glyph blits (≈18k pixel writes)** when calm and **≤365 glyphs** when busy, plus the 150 KB background copy and ray adds. That's small for a 240 MHz core. The rough estimate is a few milliseconds of CPU work, compared with the 31 ms SPI push.
5. **Font scales per depth layer: 1×, 1.5×, 2×** (6/9/12 px advance). The fractional 1.5× repeats some pixel rows unevenly, which is exactly what `setTextSize(1.5)` does and is what the preview shows.
6. **Color math.** v1 computes HSL→RGB565 per glyph, like the prototype: simple, and cheap at ~300 glyphs. If Phase 1 profiling shows render time matters, switch to RGB565 lookup tables built at boot (per palette × layer × character position × 16 shimmer phases, a few KB). The framebuffer holds **native** RGB565, so the firmware calls `lcd.setSwapBytes(true)` before `pushImage`. LovyanGFX reads a plain `uint16_t*` as byte-swapped otherwise, which garbles every color (caught in the final review).

7. **Frame pipeline: start simple, with a planned upgrade.**
   - **v1:** one full-frame 150 KB sprite in **internal SRAM** (DMA-capable, faster than PSRAM), with the background copy in PSRAM. Rendering and the DMA push run one after the other, so expect `1000 / (render + 31) ms` → **~26–28 fps** if rendering takes ~5–8 ms.
   - **If Phase 1 measures under 30 fps:** switch to **banded double-buffering**. Two 40-row band buffers (2 × 25 KB) let the CPU render band *k+1* while DMA pushes band *k*, so the frame rate is limited only by SPI (~32 fps). `renderFrame` currently draws only full 320×240 frames, and refuses any other size rather than overrun it. Banding needs a band origin and clipping added to the renderer first. With banding, the renderer would write byte-swapped pixels so the push can use DMA without CPU conversion.
   - **Optional:** try 60–80 MHz SPI on the real module. Many ILI9341 boards handle it, but it isn't guaranteed. Keep 40 MHz as the default.
8. **Legibility rules on the pixel grid:** fish tilt is capped at 8° by default (15° made the big fish into visible staircases), and the tail wiggle is under 1 px.

## Alternatives considered

| Option | Why not |
|---|---|
| Keep tuning in the smooth (anti-aliased, 24-bit) preview | What you tune isn't what ships: different glyph shapes, banding, and a 4× higher frame rate |
| Drawn (vector) fish | Bezier and gradient rasterizing per fish per frame is far more CPU than glyph blits. It's kept only as a smooth-mode experiment, and would need pre-rendered sprite frames + `pushRotateZoom` on the device |
| Per-frame alpha-blended trails / full-frame fade | Never reaches true black in 8-bit or 565 (rounding leaves haze on the reflector), and it costs a full-frame read-modify-write |
| Double full-frame buffers (2 × 150 KB SRAM) | Too much internal RAM once WiFi (v2) is running. Banding gets the same overlap for 50 KB |

## Consequences

- Visual decisions are made against the real pixel grid, colors and frame rate.
- The renderer has a JS reference implementation to port and check the C++ against. The C++ RNG draw order differs from the JS (some JS draws only feed the drawn-fish experiment), so the same seed builds a different world. The comparison is visual: a C++ snapshot (`GHOST_SNAPSHOT`) against the device preview.
- The pool caps are enforced in `tanksim`. Exceeding one drops the new item (it's never an error).
- Phase 1 must record real numbers: render ms, push ms, fps, and free SRAM/PSRAM. These go into DESIGN §8 and confirm or replace the estimates above.

## Phase 0 measurements (Wokwi, 2026-11-14)

Per-stage times from the firmware's `stages_us` log line, in Wokwi time. The simulator runs slower than real silicon, so it's calibrated against the push: 153,600 bytes at 40 MHz SPI is ≈31 ms on hardware, and Wokwi reported 149 ms, a factor of ≈4.8. Real-hardware numbers are estimates until Phase 1.

| Change | Render (Wokwi) | Biggest stage after it |
|---|---|---|
| First firmware | 178 ms | rays 83.5 ms (47%) |
| Rocks and sand baked into the background once; rays in a PSRAM backdrop, one of 8 bands refreshed per frame | 92 ms | ambient 24 ms |
| `fmod` → `floor` wrap in the color math; `pow(u, 1.5)` → `u·√u` | 86 ms | ambient 24 ms |
| Water surface in one pass (angle-addition recurrence, per-ripple constants, 120 px ripple reach) and an integer glint hash | **70 ms** | fish 17.5 ms |

With the ≈4.8× calibration that's about 15 ms render + 31 ms push, so **≈22 fps** while rendering and pushing run one after the other. The push is now twice the render time, so the next step toward 30 fps is overlapping them (§7: DMA with banding or a second buffer), not more CPU work. Confirm on hardware first.
