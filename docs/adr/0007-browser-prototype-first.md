# 0007. Tune the look in a browser prototype before porting to C++

- **Status:** Accepted
- **Date:** 2026-11-14

## Context

"Beautiful and fluid" comes down to about 20 interacting numbers: schooling weights, speeds, wander, sway frequencies, layer brightness. Tuning them with a compile → flash or simulate → watch loop takes 30–90 seconds per change, so you only get to try a few values. Tuning is about feel and needs fast feedback.

## Decision

Build a **single-file browser prototype** (`prototype/index.html`) first:

- A 320×240 canvas scaled up with nearest-neighbor, a black background and monospace glyphs, so it looks like the real screen.
- **A live slider for every `Tuning` field** in [DESIGN §6](../DESIGN.md#6-tuning-parameters-starting-values), plus a mirror toggle and an FPS meter.
- A **"Copy as C++"** button that outputs a `tank::Tuning` initializer to paste into `lib/tanksim/tuning.h`.

Once the values are copied, the C++ `tanksim` library is the source of truth. The prototype is kept as a reference and a demo, and isn't kept in sync.

## Alternatives considered

| Option | Why not |
|---|---|
| Tune in Wokwi | 30–90 seconds per change, and its frame timing isn't representative. |
| Compile `tanksim` to WebAssembly now | One codebase, but Emscripten adds setup cost before any visuals exist. Worth doing later for an online demo ([ADR 0006](0006-host-testable-sim-library.md)). |
| Skip tuning and use the defaults | The defaults are guesses. The feel *is* the product. |

## Consequences

- The motion logic exists twice for a while (JS and C++). This is accepted because the JS version is throwaway, and the `Tuning` field names must match exactly.
- Canvas fonts and the display's bitmap fonts differ slightly. The final color and size check happens in Wokwi and on hardware.
