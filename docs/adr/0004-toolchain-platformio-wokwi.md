# 0004. One PlatformIO codebase for the Wokwi simulator and the hardware

- **Status:** Accepted
- **Date:** 2026-11-14

## Context

The project starts with $0 of hardware. The code must be written and checked in a simulator first, then flashed to a real ESP32-S3 without being rewritten. The simulation logic also needs automated tests that run without any board ([ADR 0006](0006-host-testable-sim-library.md)).

## Decision

- Use **PlatformIO** (in VS Code) as the build system with two environments:
  - `esp32s3`: board `esp32-s3-devkitc-1`, Arduino framework, octal PSRAM enabled (`board_build.arduino.memory_type = qio_opi`, `-DBOARD_HAS_PSRAM`) to match the **N8R8** module.
  - `native`: the laptop, used only to run the Unity tests for `lib/tanksim`.
- Use the **Wokwi VS Code extension** to run the exact `esp32s3` firmware build in the simulator, with `wokwi.toml` and `diagram.json` in the repo root.
- **GitHub Actions** runs `pio test -e native` and `pio run -e esp32s3` on every push.

## Alternatives considered

| Option | Why not |
|---|---|
| Arduino IDE | No native test environment, weak dependency pinning, not reproducible in CI. |
| Wokwi web editor only | The code lives on a website instead of in git, and there's no CI or local tests. |
| Plain ESP-IDF (no Arduino) | More control, but more setup. The Arduino layer runs on ESP-IDF anyway, and IDF APIs (task watchdog, NVS) are still available from it. |

## Consequences

- One `platformio.ini` describes both targets. Library versions are pinned there.
- The simulator's timing isn't cycle-accurate. Frame-rate targets are checked on real hardware in Phase 1, not in Wokwi.
- Contributors need VS Code with the PlatformIO and Wokwi extensions installed (documented in the README).
