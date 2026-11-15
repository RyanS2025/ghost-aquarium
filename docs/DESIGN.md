# Ghost Aquarium: Design Spec

This is the source of truth for what the tank must do and look like. The [ADRs](adr/) record why each decision was made. The [implementation plan](plans/2026-11-14-phase0-sim-and-renderer.md) breaks the work into tasks. The look was settled by iterating in the [browser prototype](../prototype/index.html), whose default **Device preview** is the reference renderer ([ADR 0007](adr/0007-browser-prototype-first.md), [ADR 0008](adr/0008-device-render-pipeline-and-budgets.md)).

## 1. Goal

A desk piece that runs 24/7. ASCII fish, coral, rocks and bubbles seem to float inside an empty clear box. Motion should feel **calm, alive and legible**: fish glide, ease in and out of speed, rise and sink gently, leave and come back, and occasionally leap out of the water. Nothing tears, flickers or turns into a jumble of characters.

## 2. Non-goals (v1)

- Drawn or vector fish. The style is ASCII; a drawn-fish experiment lives only in the prototype's smooth mode.
- Sound, battery power, touch input. Interaction (feed button, phone page) arrives in v2.

## 3. How the illusion works

The screen lies face-up inside the base. A thin clear sheet at 45° reflects it toward the viewer, so the image appears to float behind the sheet, **mirrored left to right**. The display controller flips it back in hardware ([ADR 0003](adr/0003-rendering-lovyangfx-sprite.md)).

Black pixels are invisible in the reflection, and anything lit floats. Therefore:

- The water glow fades to **pure black at all four screen edges**, so the outline of the screen never shows on the reflector.
- No effect may leave non-black residue (for example, alpha-faded trails). Every frame is rebuilt from the background.
- The box interior is matte black, and the sheet is as thin as practical (about 1 mm) to keep the second reflection small.

## 4. Scene

All on a 320×240 screen. The water surface is at y = 20.

| Element | Content |
|---|---|
| **Fish** | 12 fish in 3 depth layers (5 Far, 4 Mid, 3 Near). Each is a rigid ASCII row with its own color palette. |
| **Water surface** | A row of `~` with gentle waves, ripples from splashes and popping bubbles, and occasional white `'` glints. |
| **Light rays** | 4 soft beams from the surface that sway and pulse slowly. They are added on top of the background (additive), not blended. |
| **Water glow** | Teal under the surface, then indigo, then violet in the depths, faded to black at every edge. Pre-rendered once and dithered. |
| **Seabed** | 4 ASCII rocks, 4 seaweed stalks and 3 coral pieces, packed left to right with even gaps, never overlapping. Sand `, . \`` and colored pebbles `o` along the bottom. |
| **Plankton** | 32 single-pixel specks that drift and twinkle in cyan and reef colors. |
| **Bubbles** | Ambient bubbles `o` rise and wobble, then pop at the surface with a small ripple. |
| **Splashes** | Droplets `' .` arc up and fall back; sparkles `*` and bubbles appear when a fish leaps out of or back into the water. |

### Fish glyphs (head → tail as drawn)

| Layer | Facing right | Facing left | Font scale | Character advance |
|---|---|---|---|---|
| Far | `><>` | `<><` | 1× | 6 px |
| Mid | `><('>` | `<')><` | 1.5× | 9 px |
| Near | `><((('>` | `<')))><` | 2× | 12 px |

### Fish palettes (head color → tail color)

`#00f0ff→#ff2d55` neon tetra · `#ff2bd6→#7a3cff` betta · `#ffd400→#ff5a00` goldfish · `#ff8c00→#fff1d6` clownfish · `#b6ff00→#00d5a0` lime · `#ff5e7a→#ffc46b` coral · `#5ad1ff→#b07bff` sky

### Seabed pieces

- **Rocks:** a jittered half-ellipse outline filled with a character grid. The top edge uses `/` `\` `-` depending on slope, the sides use `(` `)`, the inside is about 70% `#` with some `%` and `:`, and some flat top cells grow `"` moss. One or two rocks carry a `*` starfish. Sizes are 60×28, 40×20, 24×12 and 14×7 px (±15%).
- **Coral templates:** branch coral, sea fan, anemone (glows), brain coral, tube sponges. These are the five templates in `prototype/index.html` (`CORAL`), 5 characters wide.
- **Seaweed:** alternating `(` and `)` every 8 px, rooted at the bottom, swaying more toward the tip.

## 5. Motion rules ([ADR 0009](adr/0009-motion-model.md))

1. **Fixed timestep:** the simulation advances in 1/60 s steps, with at most 5 steps per loop and any extra time dropped. It is deterministic for a given seed ([ADR 0005](adr/0005-motion-subpixel-steering.md)).
2. **Float positions, rigid bodies.** A fish's body is a straight row of characters behind its head at the layer's advance, tilted by a smoothed visual tilt (at most `maxTilt`). It never bends.
3. **Steering forces, summed per step:**
   - schooling (same layer)
   - edges (top, bottom, and side walls only when the fish can't leave)
   - wander
   - level swimming and a pull toward the fish's own preferred depth

   All forces are computed from one snapshot of the tank, then all fish move, so the result doesn't depend on fish order.
4. **Wall priority.** Near a wall the other forces are scaled by `(1 − 0.6e)`, the force budget is multiplied by `(1 + 2e)`, and the turn rate by `(1 + e)`, where e (0–1) is how strongly the wall is pushing.
5. **Heading limits.** The heading rotates at most `maxTurnRate` rad/s and the pitch stays within `maxPitch` of horizontal. Speed stays between `minSpeedFrac × maxSpeed` and the layer's top speed.
6. **Reversal is always a flip-turn.** It triggers when the **navigation** force (walls + wander, never schooling) points more than `flipAngle` away from the heading for 0.25 s, with a 3 s cooldown (walls override the cooldown). It also triggers when a course change would cross vertical. During the `turnTime` animation the body shrinks to 12% around its middle, then at the halfway point vx reverses and the glyph swaps.
7. **Wander** (Reynolds): the fish steers toward a point on a circle of radius 1 placed 2 units ahead. That point drifts by `wanderJitter`, so wander can never point backward.
8. **Preferred depth.** Each fish gets its own band within its layer (spread evenly). Its preferred depth (homeY) drifts at ±8 px/s and is pulled back toward that band.
9. **Coming and going.** Exits are **reserved**: a fish that enters the side margin heading outward takes a slot if fewer than `maxAwayFrac` of the fish are away or committed, and side walls are switched off for it. Fish without a slot see the side wall from the full margin, so they always have room to turn. The cap is exact. A fish that has swum completely out of view is away for `uniform(returnDelayMin, returnDelayMax)` s. It then re-enters from a random side, at the depth (best of 6 tries) farthest from its layer-mates, with a 50% chance of a new palette.
10. **Darts:** a sprint at 2.3× speed for 0.7 s, only when the water 1 s ahead is clear, leaving a bubble spray.
11. **Leaps:** only Mid and Near fish within 70 px of the surface, away from the side walls (70 px), already heading toward the middle. The fish exits at 100 px/s, falls under 170 px/s² gravity, and splashes on both exit and re-entry.
12. **Facing:** the glyph always matches the sign of vx outside a flip. If they ever disagree, a flip-turn starts (it never swaps in one frame).

## 6. Tuning parameters

The defaults below are the values from the final prototype. Field **order** matters: the prototype's "Copy as C++" exports positional initializers in exactly this order. `test_tuning` pins the order.

### `tank::Tuning` (core)

| Field | Default | Unit |
|---|---|---|
| `maxSpeed[3]` | 16, 26, 38 | px/s (Far, Mid, Near) |
| `maxForce` | 40 | px/s² |
| `neighborRadius` | 60 | px |
| `separationRadius` | 26 | px (× 0.7 / 1.0 / 1.35 by layer) |
| `cohesionWeight` | 0.35 | |
| `alignmentWeight` | 1.0 | |
| `separationWeight` | 2.2 | |
| `wanderWeight` | 0.6 | × maxForce |
| `wanderJitter` | 1.6 | rad/s |
| `edgeMargin` | 36 | px |
| `edgeWeight` | 2.8 | × maxForce |
| `facingThreshold` | 2 | px/s |
| `fishPerLayer[3]` | 5, 4, 3 | fish |
| `bubblesPerSec` | 1.0 | 1/s |
| `bubbleRise` | 25 | px/s (× 0.8–1.2) |
| `bubbleWobbleAmp` | 2 | px |
| `bubbleWobbleHz` | 1.2 | Hz |
| `seaweedSwayAmp` | 7 | px at the tip |
| `seaweedHz[2]` | 0.31, 0.83 | Hz |

### `tank::FxTuning` (motion and look)

| Field | Default | Unit |
|---|---|---|
| `maxTurnRate` | 1.6 | rad/s |
| `maxTilt` | 8 | degrees |
| `turnTime` | 0.4 | s |
| `flipAngle` | 100 | degrees |
| `maxPitch` | 25 | degrees |
| `returnDelayMin` | 2 | s |
| `returnDelayMax` | 7 | s |
| `maxAwayFrac` | 0.35 | |
| `minSpeedFrac` | 0.3 | |
| `bodyWaveAmp` | 0.8 | px (tail wiggle) |
| `bodyWaveHz` | 1.4 | Hz (× 0.4 + 0.9·speed/max) |
| `levelPull` | 0.7 | |
| `depthPull` | 0.8 | |
| `layerSpacing` | 0.4 | cross-layer separation weight |
| `vibrance` | 1.0 | saturation multiplier |
| `shimmer` | 0.12 | lightness wobble |
| `bgGlow` | 0.9 | |
| `rays` | 0.7 | |
| `reef` | 1 | 0 or 1 |
| `dartPerSec` | 0.015 | per fish |
| `leapPerSec` | 0.08 | per tank |
| `splashDroplets` | 16 | |

## 7. Visual rules

- **Depth grading** for each fish character, applied in HSL: Far lightness ×0.55, saturation ×0.80, hue 30% of the way toward 215°. Mid ×0.80, ×0.92, 10%. Near unchanged.
- **Fish color:** head palette color → tail palette color across the row, plus shimmer `0.12·sin(3.1t − 0.85j + 1.7·id)`.
- **Draw order:** background → rays → plankton → rocks → seaweed → coral → sand → fish (Far → Near) → bubbles → particles → surface.
- **Font:** 5×7 glyphs in a 6×8 cell (LovyanGFX built-in style), placed at whole pixels, scaled 1× / 1.5× / 2× with nearest-neighbor.
- **Color:** RGB565 only. The background is dithered with a 4×4 Bayer matrix when quantized.

## 8. Performance budget ([ADR 0008](adr/0008-device-render-pipeline-and-budgets.md))

| Item | Budget / measured |
|---|---|
| Simulation | 60 Hz fixed |
| Render | 30 fps target. A full 320×240×16-bit push at 40 MHz SPI takes ≈31 ms |
| Per-frame drawing | ≈290 glyph blits (≈18k pixel writes) when calm, ≤365 when busy (measured in the device preview) |
| Pools | particles 96, bubbles 24, ripples 12, plankton 32, fish 12 (fixed arrays; when a pool is full, new items are dropped) |
| Memory | 150 KB frame in internal SRAM, 150 KB dithered background in PSRAM |
| Uptime | Task watchdog (5 s), no heap allocation after setup |

## 9. Hardware

| Part | Choice | Decision |
|---|---|---|
| MCU | ESP32-S3-DevKitC-1 **N8R8** | [ADR 0004](adr/0004-toolchain-platformio-wokwi.md) |
| Display | 2.8" 320×240 ILI9341 SPI TFT | [ADR 0002](adr/0002-display-ili9341-28.md) |
| Reflector | About 1 mm clear PETG or acrylic, optionally with beam-splitter film | §3 |
| Enclosure | 3D-printed base, matte black interior | [ROADMAP](ROADMAP.md) |

### Pin map (ESP32-S3)

| Signal | GPIO | Note |
|---|---|---|
| SPI SCLK | 12 | |
| SPI MOSI | 11 | |
| SPI MISO | 13 | Reserved (SD card later) |
| TFT CS | 10 | |
| TFT DC | 9 | |
| TFT RST | 14 | |
| TFT backlight | 21 | PWM |
| Feed button (v2) | 4 | Internal pull-up |

GPIO 35–37 are used by the octal PSRAM, and GPIO 0, 3, 45 and 46 are strapping pins. None of them are used.

## 10. Code constraints

- C++17, no exceptions and no RTTI in `lib/`.
- **`lib/tanksim`** (simulation) and **`lib/tankrender`** (framebuffer renderer) include only the standard library. Both build and are tested on a laptop ([ADR 0006](adr/0006-host-testable-sim-library.md)). Only `src/` touches Arduino, ESP-IDF or LovyanGFX.
- No heap allocation in `Tank::step()` or the renderer after setup: fixed-capacity pools only.
- All simulation randomness comes from `tank::Rng`.

## 11. Future (v2, specced separately)

- **Feed button:** pellets drift down and nearby fish steer toward them. A tap scatters fish; that logic exists in the prototype as `burst`.
- **Living tank:** hunger and growth, with state saved to NVS.
- **Phone control:** a web page on the local WiFi (feed, name fish, mirror toggle).
- **Real-world link:** Open-Meteo sunset time and rain for a configurable location.
