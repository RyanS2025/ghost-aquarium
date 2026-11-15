# Roadmap

A box is ticked only when the work is merged **and** shown working (a test run, a simulator recording, or a photo or video of the hardware).

## Phase 0: Simulate ($0)

Goal: the full tank running in the Wokwi simulator from tested code, before buying any parts.

- [x] Browser tuning prototype: look settled, device-accurate preview ([ADR 0007](adr/0007-browser-prototype-first.md), [0008](adr/0008-device-render-pipeline-and-budgets.md), [0009](adr/0009-motion-model.md))
- [x] PlatformIO project with `esp32s3` and `native` environments (CI workflow in place; first GitHub run happens when `phase0` is pushed)
- [ ] `tanksim` library: fish movement, coming & going, ambient life, seabed, fixed timestep, with unit tests. **Done except schooling** (Task 4, in progress)
- [x] 100,000-step soak test passes: no NaN, no vertical escapes, facing always correct, away cap exact, same seed gives the same result
- [ ] Renderer and main loop running in Wokwi, with frame time logged. **Renderer done and tested; firmware builds; Wokwi run pending**
- [ ] Wokwi recording added to the README

Detailed tasks: [Phase 0 plan](plans/2026-11-14-phase0-sim-and-renderer.md)

## Phase 1: Ghost tank, v1 (about $45–55)

Goal: a finished desk piece and a 30-second demo video.

**Optics test (before ordering parts)**
- [ ] Cardboard Pepper's Ghost box using a phone as the screen. Check the ghost's size, brightness, sheet thickness and how much a double image shows. This test confirms or overturns [ADR 0002](adr/0002-display-ili9341-28.md).

**Parts**
- [ ] ESP32-S3-DevKitC-1 N8R8
- [ ] 2.8" 320×240 ILI9341 SPI TFT
- [ ] Breadboard, jumper wires, USB-C cable
- [ ] About 1mm clear PETG or acrylic sheet (optionally beam-splitter film)

**Bring-up**
- [ ] Firmware from Phase 0 running on the real screen on a breadboard
- [ ] Mirroring confirmed on the real panel; frame rate measured (target ≥ 30 fps)
- [ ] Backlight PWM and a night-dim schedule

**Enclosure**
- [ ] Makerspace safety orientation and FDM 3D-printing training
- [ ] Base designed in CAD: screen pocket, 45° reflector slot, cable exit, matte black interior. Files go in `hardware/`.
- [ ] Base printed and assembled
- [ ] Optional: reflector laser-cut from acrylic

**Ship**
- [ ] 72-hour continuous run with no reset (watchdog never triggers)
- [ ] Demo video and GIF in the README
- [ ] Tagged `v1.0.0` release

## Phase 2: Interaction, v2 (about $0–5)

Each feature gets its own spec and plan before work starts.

- [ ] **Feed button:** pellets drift down and fish steer toward them
- [ ] **Living tank:** hunger and growth, with state saved to NVS so it survives power loss
- [ ] **Phone control:** the ESP32 hosts a web page to feed, name fish and toggle mirroring
- [ ] **Real-world link:** Open-Meteo sunset time and rain for a configurable location
- [ ] v2 demo video; tagged `v2.0.0`

## Optional upgrade

- [ ] 1.5" SSD1351 color OLED for true blacks (a cleaner ghost). It needs burn-in protection: slow pixel drift and no static UI.

## Timeline (target)

| Week | Milestone |
|---|---|
| 1 | Prototype tuned; `tanksim` started; makerspace orientation; cardboard optics test; parts ordered |
| 2 | Phase 0 finished in Wokwi; real screen working on a breadboard; FDM training |
| 3 | Optics tuned; base designed in CAD |
| 4 | Base printed and assembled; 72-hour soak; v1 demo; v1.0.0 |
| 5 | Feed button and living tank |
| 6 | Phone control and weather link; v2 demo |
