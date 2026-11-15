# Architecture Decision Records

Each ADR records one significant decision: the context, what was chosen, what was rejected, and the consequences. ADRs are never rewritten after the fact. If a decision changes, a new ADR replaces the old one, and the old one is marked **Superseded**.

| # | Decision | Status |
|---|---|---|
| [0001](0001-record-architecture-decisions.md) | Record architecture decisions | Accepted |
| [0002](0002-display-ili9341-28.md) | Use a 2.8" ILI9341 TFT for v1; OLED as an optional upgrade | Proposed (pending the optics test) |
| [0003](0003-rendering-lovyangfx-sprite.md) | Render with LovyanGFX: full-frame sprite, DMA push, mirroring in hardware | Accepted |
| [0004](0004-toolchain-platformio-wokwi.md) | One PlatformIO codebase for the Wokwi simulator and the hardware | Accepted |
| [0005](0005-motion-subpixel-steering.md) | Sub-pixel float motion with steering behaviors and a fixed timestep | Accepted |
| [0006](0006-host-testable-sim-library.md) | Keep the simulation in a hardware-free library tested on the laptop | Accepted |
| [0007](0007-browser-prototype-first.md) | Tune the look in a browser prototype before porting to C++ | Accepted |

New ADRs copy [template.md](template.md).
