# Roadmap

A box is ticked only when the work is merged **and** shown working (a test run, a simulator recording, or a photo or video of the hardware).

## Phase 0: Simulate ($0)

Goal: the full tank running in the Wokwi simulator from tested code, before buying any parts.

- [x] Browser tuning prototype: look settled, device-accurate preview ([ADR 0007](adr/0007-browser-prototype-first.md), [0008](adr/0008-device-render-pipeline-and-budgets.md), [0009](adr/0009-motion-model.md))
- [x] PlatformIO project with `esp32s3` and `native` environments (CI workflow in place; first GitHub run happens when `phase0` is pushed)
- [x] `tanksim` library: fish movement, body-aware schooling, coming & going, ambient life, seabed, fixed timestep, with unit tests
- [x] 100,000-step soak test passes: no NaN, no vertical escapes, facing always correct, away cap exact, same seed gives the same result
- [x] Renderer and main loop running in Wokwi, with frame time logged (`make sim`; Wokwi emulates slower than real time, so fps is measured on hardware in Phase 1)
- [x] Wokwi screenshot added to the README (a recording comes with the Phase 1 demo video)

Detailed tasks: [Phase 0 plan](plans/2026-11-14-phase0-sim-and-renderer.md)

## Phase 1: Ghost tank, v1 (about $50–75)

Goal: a finished desk piece and a 30-second demo video.

**Optics test (before ordering parts)**
- [ ] Cardboard Pepper's Ghost box using a phone as the screen, masked to the OLED's 27 × 27 mm. Check the ghost's size, brightness, sheet thickness and how much a double image shows ([ADR 0010](adr/0010-display-ssd1351-oled.md)).

**Parts** (exact items and links: [BUY.md](BUY.md))
- [ ] ESP32-S3-DevKitC-1 N8R8
- [ ] 1.5" 128×128 SSD1351 color OLED (SPI)
- [ ] Breadboard, jumper wires, USB-C cable
- [ ] About 1mm clear PETG or acrylic sheet (optionally beam-splitter film)

**128×128 port** ([ADR 0010](adr/0010-display-ssd1351-oled.md))
- [ ] Screen size in one place; scene re-tuned for 128×128; soak tests pass at the new size
- [ ] SSD1351 panel config, with the ILI9341 kept for Wokwi; `make sim` shows the 128×128 tank
- [ ] Burn-in protection: slow whole-image drift, no static UI

**Bring-up**
- [ ] Firmware from Phase 0 running on the real screen on a breadboard
- [ ] Rotation and mirroring confirmed on the real panel; frame rate measured (target ≥ 30 fps)
- [ ] Brightness control and a night-dim schedule (part of burn-in protection on an OLED)

**Enclosure**
- [ ] Makerspace safety orientation and FDM 3D-printing training
- [x] Base designed in CAD: screen window + bosses, 45° reflector grooves, USB-C notch, dark back wall. Parametric code-CAD + STLs in [`hardware/`](../hardware/) (`make cad`)
- [ ] Base resized for the 1.5" OLED module from caliper measurements (`DISP_*` in `ghost_base.py`)
- [ ] Base printed and assembled
- [ ] Optional: reflector laser-cut from acrylic

**Ship**
- [ ] 72-hour continuous run with no reset (watchdog never triggers)
- [ ] Demo video and GIF in the README
- [ ] Tagged `v1.0.0` release

## Phase 2: Interaction, v2 (about $0–5)

Spec: [v2 interaction](specs/v2-interaction.md). Each feature gets its own plan before code.

- [ ] **Feed button:** pellets drift down and fish steer toward them
- [ ] **Living tank:** hunger and growth, with state saved to NVS so it survives power loss
- [ ] **Phone control:** the ESP32 hosts a web page to feed, name fish and toggle mirroring
- [ ] **Real-world link:** Open-Meteo sunset time and rain for a configurable location
- [ ] v2 demo video; tagged `v2.0.0`

## Optional upgrade

- [ ] A bigger ghost: a 1.9–2.4" color AMOLED (true black, larger), if 1.5" feels small. It needs a new board or a QSPI driver.

## Timeline (target)

| Week | Milestone |
|---|---|
| 1 | Prototype tuned; `tanksim` started; makerspace orientation; cardboard optics test; parts ordered |
| 2 | Phase 0 finished in Wokwi; real screen working on a breadboard; FDM training |
| 3 | Optics tuned; base designed in CAD |
| 4 | Base printed and assembled; 72-hour soak; v1 demo; v1.0.0 |
| 5 | Feed button and living tank |
| 6 | Phone control and weather link; v2 demo |
