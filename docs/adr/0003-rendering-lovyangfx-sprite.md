# 0003. Render with LovyanGFX: full-frame sprite, DMA push, mirroring in hardware

- **Status:** Accepted (amended by [0010](0010-display-ssd1351-oled.md): the panel is now a 128×128 SSD1351, mirrored with rotation 6; the 320×240 bandwidth figures below apply to the simulator only)
- **Date:** 2026-11-14

## Context

"Fluid" rules out visible flicker or tearing. The usual Arduino approach (Adafruit_GFX, erase each fish and redraw it straight to the screen) causes flicker: the panel shows the erased frame before the redraw lands, and overlapping fish erase each other.

The bandwidth budget is fixed by the bus. One full 320×240 frame at 16 bits per pixel is **153,600 bytes**, which takes **about 31 ms at 40 MHz SPI**. That caps full redraws at about 32 fps.

The Pepper's Ghost reflection also mirrors the image left to right, so it has to be flipped.

## Decision

- Use **LovyanGFX** as the graphics library.
- Draw every frame into a **full-screen `LGFX_Sprite`** (an off-screen framebuffer). Allocate it in **PSRAM**. If PSRAM isn't available (for example in a simulator that doesn't model it), fall back to internal SRAM, which the S3 has enough of (512 KB) for one 150 KB buffer.
- Push the finished frame with **`pushSprite` over DMA**, so the CPU can start simulating the next frame while the bytes go out.
- Mirror the image **in the display controller** by choosing a rotation whose MADCTL bits mirror left-right: landscape is rotation 1 (MV|MX), and its left-right mirror is **rotation 7** (adds MY). Rotation 5 would flip top-to-bottom instead (checked against LovyanGFX `Panel_LCD::getMadCtl` in the final review). This costs no CPU.

## Alternatives considered

| Option | Why not |
|---|---|
| Adafruit_GFX + Adafruit_ILI9341 | No sprite or DMA pipeline. Drawing straight to the screen flickers. Slower SPI path. |
| TFT_eSPI | Also has sprites and DMA, and is widely used. Its configuration lives in a `User_Setup.h` edited inside the library folder, which is awkward to keep in a repo and in CI. LovyanGFX takes its configuration in code, in our own repo. |
| Dirty-rectangle redraw only | Uses less bandwidth, but overlapping moving fish make the bookkeeping complicated and error-prone. Kept as an optimization if profiling shows the full-frame push is the bottleneck. |
| Mirroring in software | Works, but costs CPU every frame for something the panel does for free. |

## Consequences

- One framebuffer uses about 150 KB. Double buffering (draw one while DMA sends the other) would use about 300 KB in PSRAM. Add it only if profiling shows the CPU waiting on DMA.
- The frame rate is limited by SPI at about 30 fps. The simulation runs on its own 60 Hz clock ([ADR 0005](0005-motion-subpixel-steering.md)), so a lower render rate doesn't change how the fish move.
- Check the mirrored-rotation behavior on the real panel in Phase 1, since the simulator may not model MADCTL mirroring exactly.
