# Ghost Aquarium

**ASCII fish that swim in mid-air.** Ghost Aquarium is a desk aquarium built on an ESP32-S3. A small color screen is hidden in a 3D-printed base, and a clear sheet at 45° reflects it toward the viewer (the *Pepper's Ghost* illusion). Schools of ASCII fish, swaying seaweed and rising bubbles appear to float inside an empty box. It's designed to run 24/7 as a desk piece.

> **Status: design complete, build starting (November 2026).** This README describes the target. The [roadmap](docs/ROADMAP.md) tracks what is actually finished, and a box is ticked only when the work is merged and demonstrated.

---

## How it works

```
   eye ───────────────►   ><((('>   ghost image floats behind the sheet
                      ╲
                       ╲   clear sheet at 45°
                        ╲
   ┌──────────────────────────────┐
   │  ▓▓▓▓  screen, face up  ▓▓▓▓  │   hidden in the 3D-printed base
   └──────────────────────────────┘
```

Light from the screen reflects off the angled sheet toward your eye. Black pixels reflect nothing, so only the fish are visible and they seem to hang in the air. The reflection is mirrored left to right, and the display controller flips it back in hardware at no CPU cost.

## Engineering highlights

| Area | Approach |
|---|---|
| **Smooth motion** | Fish have float positions and velocities and are drawn at exact pixel positions, not snapped to a text grid. Steering behaviors (separation, alignment, cohesion, wander, edge avoidance) produce schooling. |
| **No flicker or tearing** | Each frame is drawn into an off-screen sprite in PSRAM, then pushed to the display with DMA. The frame-time budget is worked out from the SPI bandwidth: [DESIGN §8](docs/DESIGN.md#8-performance-targets). |
| **Stable simulation** | A fixed 60 Hz timestep runs separately from the render rate, with a cap on catch-up steps, so a stalled frame never makes fish teleport. |
| **Depth** | Three layers. Far fish are dimmer, slower and bluer. In a Pepper's Ghost box this reads as real depth. |
| **Testable firmware** | All of the simulation is a hardware-free C++ library, unit-tested on a laptop with PlatformIO's `native` environment and run in CI on every push. |
| **Simulator first** | Every feature is tested in the [Wokwi](https://wokwi.com) ESP32 simulator before it goes onto the hardware. |
| **Built for 24/7** | Task watchdog, no heap allocation per frame, deterministic seeded randomness, and a night-dimming brightness schedule. |

## Tech stack

C++17 · ESP32-S3 (Arduino on ESP-IDF) · [LovyanGFX](https://github.com/lovyan03/LovyanGFX) · PlatformIO · Unity test framework · Wokwi · GitHub Actions · CAD + FDM 3D printing

## Repository layout (target)

```
lib/tanksim/        Hardware-free simulation: fish, schooling, bubbles, seaweed, timestep
src/                Firmware: display config, renderer, main loop
test/test_sim/      Unit tests for tanksim (run on the laptop)
prototype/          Browser canvas prototype for tuning motion and color
hardware/           CAD for the base, reflector dimensions, wiring
docs/               Design spec, roadmap, ADRs, implementation plans
```

## Build and run

*Coming in Phase 0, see the [plan](docs/plans/2026-11-14-phase0-sim-and-renderer.md).*

```bash
pio test -e native      # run the simulation unit tests on your machine
pio run  -e esp32s3     # build the firmware
# then run it in Wokwi: VS Code → "Wokwi: Start Simulator"
```

## Documentation

| Doc | What's in it |
|---|---|
| [Design spec](docs/DESIGN.md) | What the tank does and looks like: scene, motion rules, tuning values, hardware, constraints |
| [Roadmap](docs/ROADMAP.md) | Phases, hardware milestones, current progress |
| [Architecture decisions](docs/adr/) | Why each technical choice was made, and the alternatives that were rejected |
| [Phase 0 plan](docs/plans/2026-11-14-phase0-sim-and-renderer.md) | Task-by-task implementation plan for the simulation and renderer |

## Roadmap at a glance

- [ ] **Phase 0: Simulate.** Browser tuning prototype, tested simulation library, renderer running in Wokwi
- [ ] **Phase 1: Ghost tank (v1).** Real hardware, cardboard optics test, 3D-printed base, demo video
- [ ] **Phase 2: Interaction (v2).** Feed button, living tank with saved state, phone control page, weather and sunset link

## Credits

Inspired by a maker video by `_re_pete` on TikTok. This is an independent build with its own code and design.

## License

[MIT](LICENSE)
