# 0010. Use a 1.5" SSD1351 color OLED for v1

- **Status:** Accepted. Supersedes [0002](0002-display-ili9341-28.md).
- **Date:** 2026-11-15

## Context

[ADR 0002](0002-display-ili9341-28.md) picked a 2.8" ILI9341 LCD for v1 and kept a color OLED as an optional upgrade. Its known weakness was black level: an LCD backlight leaks light, so the whole screen shows as a faint gray rectangle in the reflection.

The point of the piece is that the fish float in an empty clear box. A gray rectangle behind them gives the trick away. Reference builds of ASCII ghost tanks show the OLED look: lit glyphs on clear acrylic with no visible screen outline. The owner chose that look for v1.

Three facts shape the choice:

1. **OLED black is off.** Unlit pixels emit nothing, so only the fish, plants and bubbles appear on the reflector. The black-edge fades in [DESIGN §3](../DESIGN.md#3-how-the-illusion-works) stop being a workaround and become a bonus.
2. **The Wokwi simulator has no SSD1351 part.** Its supported displays are the ILI9341, SSD1306 and SH1107 ([Wokwi supported hardware](https://docs.wokwi.com/getting-started/supported-hardware), checked 2026-11-15).
3. **LovyanGFX 1.3.0 (already pinned) ships `Panel_SSD1351`.** Its rotation table includes column-remapped rotations, so left-right mirroring stays free in the panel ([ADR 0003](0003-rendering-lovyangfx-sprite.md)).

## Decision

Use a **1.5" 128×128 SSD1351 16-bit color OLED over 4-wire SPI** for v1, for example [Adafruit #1431](https://www.adafruit.com/product/1431) or a Waveshare-style module.

- **The tank becomes a 128×128 world.** The simulation and renderer take their screen size from one place, and the scene is re-tuned for it. The scene has fewer fish and a shorter seabed, and keeps the same ASCII style and 6×8 glyph cell. That gives a 21 × 16 character grid.
- **The simulator keeps the ILI9341 part.** `make sim` draws the 128×128 tank 1:1 in the middle of the simulated 320×240 screen and blacks out the rest. The firmware selects the panel at compile time, so the same code runs in Wokwi and on the OLED.
- **Mirroring uses LovyanGFX rotation 6** (rotation 0 with the column order reversed). If the real module shows the tank upside down, use rotation 4 for upright and rotation 2 for mirrored. Confirm this in Phase 1, as ADR 0003 already requires for the LCD.
- **Burn-in protection is required:** a slow whole-image pixel drift, no static UI, and dimming at night.

## Alternatives considered

| Option | Why not |
|---|---|
| 2.8" ILI9341 LCD ([ADR 0002](0002-display-ili9341-28.md)) | Bigger ghost and a direct simulator match, but the backlight glow shows the screen's outline on the reflector. That outline is what the owner wants gone. |
| Larger color AMOLED (ESP32-S3 boards with a built-in 1.9–2.4" AMOLED) | Bigger ghost with true black, but a QSPI panel, a new board and a new driver path. Revisit for v2 if 1.5" feels small. |
| 0.96–1.3" monochrome OLED (SSD1306 / SH1107) | Wokwi simulates it, but it's single-color. The look depends on per-fish color palettes. |

## Consequences

- **The ghost is small:** the visible area is about 27 × 27 mm, roughly the size of a large postage stamp. The base, hood and reflector shrink to match. Re-run `make cad` with the module's measured size.
- **Bandwidth stops being the limit.** A full frame is 32,768 bytes, about 13 ms at the SSD1351's 20 MHz SPI limit, so 30 fps fits with room to spare. The 32 KB frame also fits in internal SRAM, so banded DMA ([ADR 0008](0008-device-render-pipeline-and-budgets.md)) becomes optional. The banded path stays in the code because it is tested.
- **Re-tuning work:** constants such as the floor height, swim range, leap range, fish count, pool sizes and the prototype's canvas assume 320×240. Each moves to the screen-size header or is re-tuned, and the soak tests run again at 128×128.
- **The simulator is no longer an exact hardware match.** Wokwi checks the logic, the memory and the 128×128 image. It doesn't check SSD1351 initialization, color order or rotation. Those are verified on the real module in Phase 1.
- **Brightness and lifetime:** OLED pixels age with use. The night-dim schedule (Phase 1) becomes part of the burn-in protection, not only a nice-to-have.
- **Cost:** about $17–40 for the display, depending on the seller (prices in [BUY.md](../BUY.md)).
