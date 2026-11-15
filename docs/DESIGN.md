# Ghost Aquarium: Design Spec

This is the source of truth for what the tank must do and look like. The [ADRs](adr/) record why each technical choice was made. The [implementation plan](plans/2026-11-14-phase0-sim-and-renderer.md) breaks the work into tasks.

## 1. Goal

A desk piece that runs 24/7. ASCII fish, seaweed and bubbles seem to float inside an empty clear box. Motion should feel **fluid, alive and calm**: fish glide, school, drift apart and turn smoothly. Nothing jumps, flickers or tears.

## 2. Non-goals (v1)

- Realistic fish graphics. The style is ASCII by design.
- Sound.
- Battery power. The tank runs on USB.
- Touch input. Interaction arrives in v2 (feed button, phone page).

## 3. How the illusion works

The screen lies face-up inside the base. A thin clear sheet sits above it at 45° and reflects the screen toward the viewer. The reflected image appears to float behind the sheet. Because it is a reflection, it is **mirrored left to right**. The display controller corrects this in hardware (see [ADR 0003](adr/0003-rendering-lovyangfx-sprite.md)).

Black pixels are invisible in the reflection. Everything that isn't black looks like it's floating. So:

- The background is pure black (`0x0000`). Only fish, seaweed and bubbles are lit.
- The inside walls of the box are matte black, so the room doesn't show in the reflection.
- The thinnest practical sheet (about 1mm PETG or acrylic) keeps the second, offset reflection from the back surface to a minimum.

## 4. Scene

| Element | Behavior |
|---|---|
| **Fish** | 14 fish in 3 depth layers. Each layer is its own school. Fish wander, school, avoid the edges and turn smoothly. |
| **Depth layers** | Far: small, dim, slow, blue-tinted. Mid: medium. Near: large, bright, fast. Fish only school with fish in their own layer. |
| **Seaweed** | 4–6 stalks rooted at the bottom. The sway grows toward the tip and uses two overlapping sine waves, so it never looks like it's on a loop. |
| **Bubbles** | Spawn at random along the bottom, rise steadily, wobble side to side, and disappear at the top. |

### Species (ASCII glyphs)

| Species | Facing right | Facing left | Layer |
|---|---|---|---|
| Minnow | `><>` | `<><` | Far |
| Tetra | `><('>` | `<')><` | Mid |
| Angelfish | `><((('>` | `<')))><` | Near |

Only ASCII characters are used, so every built-in font can draw them.

## 5. Motion rules

- **Positions are floats in pixels, not character cells.** Glyphs are drawn at the exact rounded pixel position, so fish glide instead of jumping one cell at a time ([ADR 0005](adr/0005-motion-subpixel-steering.md)).
- **Steering behaviors** (Reynolds-style): separation, alignment, cohesion, wander and edge avoidance are combined as weighted forces. Each fish's steering is capped at `maxForce` and its speed at the layer's `maxSpeed`.
- **Facing hysteresis.** A fish flips its glyph only when `|vel.x| > facingThreshold`. Without this, a fish moving straight up or down would flicker between `><>` and `<><` every frame.
- **Fixed timestep.** The simulation advances in exact 1/60 s steps, separate from the render rate. If a frame stalls (for example a WiFi hiccup in v2), at most 5 steps run and the extra time is dropped, so fish never teleport through walls.
- **Deterministic.** The same seed always gives the same tank. That makes bugs reproducible and tests exact.

## 6. Tuning parameters (starting values)

These starting values will be tuned in the browser prototype ([ADR 0007](adr/0007-browser-prototype-first.md)) and then copied into `lib/tanksim/tuning.h`.

| Field | Default | Unit | Meaning |
|---|---|---|---|
| `maxSpeed[3]` | 18, 28, 40 | px/s | Far, Mid, Near top speed |
| `maxForce` | 30 | px/s² | Steering cap |
| `neighborRadius` | 40 | px | Range for schooling |
| `separationRadius` | 14 | px | Personal space |
| `cohesionWeight` | 0.6 | | Pull toward the school's center |
| `alignmentWeight` | 1.0 | | Match neighbors' heading |
| `separationWeight` | 1.6 | | Push away from crowding |
| `wanderWeight` | 0.8 | | Strength of the random meander |
| `wanderJitter` | 2.0 | rad/s | How fast the wander direction drifts |
| `edgeMargin` | 24 | px | Distance from a wall where turning starts |
| `edgeWeight` | 2.5 | | Strength of the wall turn |
| `facingThreshold` | 3 | px/s | Horizontal speed needed to flip the glyph |
| `fishPerLayer[3]` | 6, 5, 3 | fish | Far, Mid, Near |
| `bubblesPerSec` | 1.5 | 1/s | Spawn rate |
| `bubbleRise` | 25 | px/s | Rise speed |
| `bubbleWobbleAmp` | 2 | px | Side-to-side wobble |
| `bubbleWobbleHz` | 1.2 | Hz | Wobble frequency |
| `seaweedSwayAmp` | 6 | px | Sway at the tip |
| `seaweedHz[2]` | 0.31, 0.83 | Hz | The two sway frequencies |

## 7. Visual rules

- **Layer brightness:** Far 35%, Mid 65%, Near 100%. The Far layer is shifted toward blue. In a Pepper's Ghost box, dimmer layers look further away inside the glass, which gives the scene depth.
- **Fonts:** Far uses the 6×8 built-in font. Mid and Near use progressively larger monospace fonts.
- **Draw order:** seaweed, then Far, Mid and Near fish, then bubbles.

## 8. Performance targets

| Metric | Target | Reasoning |
|---|---|---|
| Simulation rate | 60 Hz fixed | Smooth integration and stable steering |
| Render rate | ≥ 30 fps | A full 320×240×16-bit frame is 153,600 bytes, about 31 ms over 40 MHz SPI, so roughly 32 fps is the ceiling for a full redraw |
| Frame jitter | No visible tearing | Draw the whole frame off-screen, then push it in one transfer ([ADR 0003](adr/0003-rendering-lovyangfx-sprite.md)) |
| Uptime | Weeks without a reset | Task watchdog, no heap allocation per frame |

## 9. Hardware

| Part | Choice | Decision |
|---|---|---|
| MCU | ESP32-S3-DevKitC-1 **N8R8** (8 MB flash, 8 MB octal PSRAM) | [ADR 0004](adr/0004-toolchain-platformio-wokwi.md) |
| Display | 2.8" 320×240 ILI9341 SPI TFT | [ADR 0002](adr/0002-display-ili9341-28.md) |
| Reflector | About 1mm clear PETG or acrylic, optionally with beam-splitter film | §3 |
| Enclosure | 3D-printed base (FDM) with a matte black interior | [ROADMAP](ROADMAP.md) |

### Pin map (ESP32-S3)

| Signal | GPIO | Note |
|---|---|---|
| SPI SCLK | 12 | |
| SPI MOSI | 11 | |
| SPI MISO | 13 | Not used by the display. Reserved for an SD card later. |
| TFT CS | 10 | |
| TFT DC | 9 | |
| TFT RST | 14 | |
| TFT backlight | 21 | PWM for dimming |
| Feed button (v2) | 4 | Internal pull-up |

GPIO 35–37 are taken by the octal PSRAM on N8R8 modules. GPIO 0, 3, 45 and 46 are strapping pins. Neither group is used.

## 10. Code constraints

- C++17. No exceptions and no RTTI in the simulation library.
- `lib/tanksim/` must not include Arduino, ESP-IDF or display headers. It has to build and test on a laptop ([ADR 0006](adr/0006-host-testable-sim-library.md)).
- No heap allocation inside `Tank::step()` or `Renderer::draw()` after setup. Vectors reserve their capacity up front.
- All randomness comes from the seeded `tank::Rng`. No `rand()`, `millis()` or `esp_random()` inside the simulation.

## 11. Future (v2, specced separately)

- **Feed button:** pellets drift down and nearby fish steer toward them.
- **Living tank:** hunger and growth, with state saved to flash (NVS) so it survives power loss.
- **Phone control:** the ESP32 serves a web page on the local WiFi to feed, name fish or toggle mirroring.
- **Real-world link:** Open-Meteo (free, no API key) supplies the sunset time and rain for a configurable location. The tank dims at night and gets darker, with more bubbles, when it rains.
