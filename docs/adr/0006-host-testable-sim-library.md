# 0006. Keep the simulation in a hardware-free library tested on the laptop

- **Status:** Accepted
- **Date:** 2026-11-14

## Context

Movement bugs are subtle: NaN from normalizing a zero vector, a fish slowly drifting off the screen over hours, glyphs flickering when a fish moves straight up. Finding them by watching the screen is slow and unreliable. The tank must also run 24/7, so problems that only appear after a long run need automated coverage.

## Decision

- Put all simulation logic (vectors, RNG, fish, steering, bubbles, seaweed, timestep, the `Tank` world) in **`lib/tanksim/`** under namespace `tank`.
- `tanksim` includes **only the C++ standard library**. No Arduino, ESP-IDF or display headers.
- Test it with **Unity** in PlatformIO's **`native`** environment (`test/test_sim/`). This includes a 100,000-step soak test and a same-seed determinism test.
- The firmware (`src/`) only *reads* the simulation state to draw it.

## Alternatives considered

| Option | Why not |
|---|---|
| Tests on the device (`pio test -e esp32s3`) | Needs a board connected, slow to flash, can't run in CI. |
| No automated tests, check visually | Misses long-run and edge-case bugs, the kind that break a 24/7 device. |
| GoogleTest | Heavier. Unity is PlatformIO's default and runs on both the laptop and the device if needed later. |

## Consequences

- The boundary between simulation and rendering is enforced by the include rules, and CI breaks if it's crossed.
- The simulation can be reused later, for example compiled to WebAssembly for an online demo.
- Rendering code isn't unit-tested. It's checked in Wokwi and on hardware.
