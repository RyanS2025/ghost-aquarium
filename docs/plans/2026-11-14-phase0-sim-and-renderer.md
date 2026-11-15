# Phase 0: Simulation and Renderer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

> **Revision 2 (2026-11-14):** Tasks 3–6 were rewritten after the look was settled in the prototype ([ADR 0008](../adr/0008-device-render-pipeline-and-budgets.md), [ADR 0009](../adr/0009-motion-model.md)). Tasks 1–2 are done and unchanged.

**Goal:** Get the final ASCII ghost tank running from unit-tested, hardware-free C++ libraries, plus firmware that pushes it to an ILI9341 on an ESP32-S3 (Wokwi first), without buying any hardware.

**Architecture:**
- `lib/tanksim` (namespace `tank`) is the simulation: fish, schooling, coming and going, ambient life, seabed. It is a C++ port of `step()` in `prototype/index.html`.
- `lib/tankrender` (namespace `tank::render`) renders a `Tank` into a plain RGB565 framebuffer. It is a C++ port of `drawDevice()` in the prototype.
- Both libraries use only the standard library and are tested on the laptop.
- `src/` (firmware) owns the framebuffer memory, runs the 60 Hz stepper, and pushes each frame to the panel with LovyanGFX.

**Tech Stack:** C++17, PlatformIO (`espressif32` + Arduino, `native`), LovyanGFX `^1.2.0`, Unity, Wokwi, GitHub Actions.

**Spec:** [docs/DESIGN.md](../DESIGN.md). Reference implementation: `prototype/index.html` (Device preview). Decisions: ADR [0003](../adr/0003-rendering-lovyangfx-sprite.md), [0004](../adr/0004-toolchain-platformio-wokwi.md), [0005](../adr/0005-motion-subpixel-steering.md), [0006](../adr/0006-host-testable-sim-library.md), [0008](../adr/0008-device-render-pipeline-and-budgets.md), [0009](../adr/0009-motion-model.md).

## Global Constraints

- Screen: 320×240. Water surface at y = 20. Gravity 170 px/s². `tank::kScreen{320.f, 240.f}`.
- C++17. No exceptions or RTTI in `lib/`. `lib/tanksim` and `lib/tankrender` include only standard-library headers.
- No heap allocation in `Tank::step()` or `renderFrame()`: fixed pools (fish ≤ 24, particles 96, bubbles 24, ripples 12, plankton 32). When a pool is full, new items are dropped.
- All simulation randomness comes from `tank::Rng`.
- Simulation step 1/60 s, at most 5 steps per loop. Render at ≤ 30 fps.
- Tuning field names, order and defaults exactly as in [DESIGN §6](../DESIGN.md#6-tuning-parameters). Motion rules as in DESIGN §5, look as in DESIGN §4/§7.
- Pins as in DESIGN §9. Layer index `Far = 0, Mid = 1, Near = 2`; species index = layer.
- Where DESIGN is silent, the prototype's `step()` / `drawDevice()` is the reference behavior.

## Review Focus

1. **Zero-length vectors** (coincident fish, a stationary fish, zero steering) must never produce NaN. Tests: Task 2 `test_normalized_zero_is_zero`, Task 4 `test_school_coincident_fish_is_finite`, Task 5 soak.
2. **Frame stalls:** a huge `dt` must not tunnel fish through walls. Test: Task 5 `test_stepper_caps_steps_and_drops_excess`.
3. **Glyph/heading mismatch** (the "messy fish" bug class): outside a flip-turn, the glyph must always face the direction of vx. Tests: Task 3 `test_safety_net_starts_flip_on_mismatch`, Task 5 `test_soak_facing_always_matches`.
4. **Long runs (24/7):** no NaN, no vertical escapes, fish always return from away, the tank never empties. Test: Task 5 `test_soak_100k_steps_finite_bounded_and_populated`.
5. **Pool exhaustion:** a burst of splashes when particles are at their cap must drop items, not overflow or allocate. Test: Task 5 `test_pools_drop_when_full`.

---

### Task 1: Browser tuning prototype

**Files:**
- Create: `prototype/index.html` (one self-contained file, no build step, no external libraries)

**Interfaces:**
- Consumes: the `Tuning` fields and defaults in DESIGN §6; species glyphs in DESIGN §4; layer brightness in DESIGN §7.
- Produces: tuned values in the exact text format of `lib/tanksim/tuning.h` (Task 2) and `src/render/palette.h` (Task 6).

- [ ] **Step 1: Build the canvas scene**
  A 320×240 logical canvas drawn at 3× with `image-rendering: pixelated`, background `#000`, monospace glyphs. The JS fish, school, bubble and seaweed logic follows DESIGN §5 and is a deliberately simple version of ADR 0005, ported informally. Use `requestAnimationFrame` with the same fixed-step accumulator (1/60 s, at most 5 steps).
- [ ] **Step 2: Add the controls**
  One slider per `Tuning` field, using the DESIGN §6 defaults. Also: a per-layer RGB color picker, a "Mirror" checkbox (CSS `scaleX(-1)` on the canvas), an FPS readout, and a "Reseed" button.
- [ ] **Step 3: Add "Copy as C++"**
  Copies two blocks to the clipboard: `constexpr tank::Tuning kTuning{ /* designated-order initializer */ };` and `constexpr uint16_t kLayerColor[3] = {0x...., 0x...., 0x....};`, with colors converted to RGB565.
- [ ] **Step 4: Verify**
  Open `prototype/index.html` in a browser. Expected: 14 fish in 3 visibly separate layers, schooling; moving any slider changes behavior right away; FPS ≥ 55; "Copy as C++" pastes valid-looking C++.
- [ ] **Step 5: Tune and commit**
  Tune until it feels right (the human makes the call). Save the exported block to `prototype/tuned-values.txt`.
  ```bash
  git add prototype/
  git commit -m "feat(prototype): browser tuning playground with C++ export"
  ```

---

### Task 2: Project scaffold, core types and CI

**Files:**
- Create: `platformio.ini`, `.github/workflows/ci.yml`
- Create: `lib/tanksim/vec2.h`, `lib/tanksim/rng.h`, `lib/tanksim/rng.cpp`, `lib/tanksim/tuning.h`
- Test: `test/test_vec2/test_main.cpp`, `test/test_rng/test_main.cpp`

**Interfaces:**
- Produces:
  - `struct tank::Vec2 { float x = 0, y = 0; }` with `+ - += -=`, `* (float)`, `/ (float)`, `float length() const`, `float dot(Vec2) const`, `Vec2 normalized() const` (returns `{0,0}` when length < 1e-6), `Vec2 limited(float maxLen) const`.
  - `class tank::Rng { explicit Rng(uint32_t seed); uint32_t next(); float uniform(float lo, float hi); }`, xorshift32. A seed of 0 is replaced with `0x9E3779B9`.
  - `enum class tank::Layer : uint8_t { Far = 0, Mid = 1, Near = 2 };`
  - `struct tank::Bounds { float w, h; };`
  - `struct tank::Tuning` with every DESIGN §6 field. Use the Task 1 tuned values if Task 1 is done, otherwise the DESIGN defaults.

- [ ] **Step 1: Write `platformio.ini`**
  ```ini
  [platformio]
  default_envs = esp32s3

  [env:esp32s3]
  platform = espressif32
  board = esp32-s3-devkitc-1
  framework = arduino
  board_build.arduino.memory_type = qio_opi
  build_unflags = -std=gnu++11
  build_flags = -std=gnu++17 -Wall -Wextra -DBOARD_HAS_PSRAM
  lib_deps = lovyan03/LovyanGFX@^1.2.0
  monitor_speed = 115200

  [env:native]
  platform = native
  test_framework = unity
  build_flags = -std=c++17 -Wall -Wextra
  build_src_filter = -<*>
  ```
  `build_src_filter = -<*>` keeps the Arduino `src/` out of the laptop build. `lib/tanksim` is pulled in by the tests' includes.
- [ ] **Step 2: Write the failing tests** in `test/test_vec2/test_main.cpp`
  ```cpp
  void test_normalized_zero_is_zero() {
    auto n = tank::Vec2{0, 0}.normalized();
    TEST_ASSERT_EQUAL_FLOAT(0.f, n.x); TEST_ASSERT_EQUAL_FLOAT(0.f, n.y);
  }
  void test_normalized_has_unit_length() {
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.f, tank::Vec2{3, 4}.normalized().length());
  }
  void test_limited_caps_length_keeps_direction() {
    auto l = tank::Vec2{30, 40}.limited(10.f);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 6.f, l.x); TEST_ASSERT_FLOAT_WITHIN(1e-4f, 8.f, l.y);
  }
  void test_limited_leaves_short_vectors_alone() {
    auto l = tank::Vec2{1, 1}.limited(10.f);
    TEST_ASSERT_EQUAL_FLOAT(1.f, l.x); TEST_ASSERT_EQUAL_FLOAT(1.f, l.y);
  }
  ```
  And in `test/test_rng/test_main.cpp`:
  ```cpp
  void test_rng_same_seed_same_sequence()   // Rng a(42), b(42): 100 x next() equal
  void test_rng_seed_zero_still_varies() {   // Review Focus #5
    tank::Rng r(0); uint32_t a = r.next(), b = r.next();
    TEST_ASSERT_NOT_EQUAL(0u, a); TEST_ASSERT_NOT_EQUAL(a, b);
  }
  void test_rng_uniform_in_range()         // 10,000 draws of uniform(-2, 5) all in [-2, 5)
  ```
  Each file has a `main()` that calls `UNITY_BEGIN()`, `RUN_TEST(...)` for each test, then `UNITY_END()`.
- [ ] **Step 3: Run and confirm the tests fail**
  Run: `pio test -e native`
  Expected: compile errors (`tank::Vec2` and `tank::Rng` not defined).
- [ ] **Step 4: Implement** `vec2.h` (header-only), `rng.h` and `rng.cpp`, and `tuning.h` to the Interfaces above.
- [ ] **Step 5: Run and confirm the tests pass**
  Run: `pio test -e native`. Expected: `test_vec2` and `test_rng` PASSED.
- [ ] **Step 6: Add CI**
  `.github/workflows/ci.yml` runs on push and pull_request on `ubuntu-latest`: `actions/checkout@v4`, `actions/setup-python@v5` (3.12), `actions/cache@v4` on `~/.platformio`, `pip install platformio`, `pio test -e native`, `pio run -e esp32s3`. For the firmware step to pass on its own, add a placeholder `src/main.cpp` with empty `setup()` and `loop()`.
- [ ] **Step 7: Commit**
  ```bash
  git add platformio.ini lib/ test/ src/main.cpp .github/
  git commit -m "feat(tanksim): project scaffold, Vec2, seeded Rng, Tuning, CI"
  ```

---


### Task 3: Tuning v2, palette data and fish kinematics

**Files:**
- Modify: `lib/tanksim/tuning.h` (DESIGN §6 defaults; add `FxTuning`, world constants), `test/test_tuning/test_main.cpp` (pin both structs' order)
- Create: `lib/tanksim/palette.h`, `lib/tanksim/fish.h`, `lib/tanksim/fish.cpp`
- Test: `test/test_fish/test_main.cpp`

**Interfaces:**
- Consumes: `Vec2`, `Rng` (Task 2).
- Produces:
  - `tuning.h`: `Tuning` (core, same field order, new defaults), `FxTuning` (DESIGN §6 order). Constants: `kScreen`, `kSurfaceY = 20`, `kFloorY = 210` (bottom edge for fish = H − 30), `kGravity = 170`, `kCellW = 6`, `kCellH = 8`, `kFontScale[3] = {1, 1.5, 2}`, `kGlyphLen[3] = {3, 5, 7}`, `kMaxFish = 24`, `kHomeMin = 52`, `kHomeMax = 192`, `kLeapVy = 100`.
  - `palette.h`: `kFishPalette[7][2]`, `kReefColors[7]`, `SceneColors kScene` (RGB888 `uint32_t`, values from DESIGN §4 and the prototype).
  - `fish.h`: `struct Fish` (`pos, vel, layer, palette, id, facingRight, airborne, away, crossing, leaving, sideWalls, flipTurn, turnSwapped, wanderAngle, wavePhase, tilt, turnP, boostT, leapT, flipCooldown, reverseT, homeBase, homeY, returnT, body[7], bodyN`) and these functions:
    - `charAdvance(Layer)`, `bodyLength(Layer)`
    - `rigidBody(const Fish&, int n, float spacing, float squeeze, Vec2* out)`
    - `flipSqueeze(const Fish&)`
    - `steerEdges(const Fish&, const Tuning&)`
    - `steerWander(Fish&, Rng&, float dt, const Tuning&)` (Reynolds: circle radius 1, 2 ahead)
    - `steerLevel(Fish&, Rng&, float dt, const FxTuning&)` (homeY drift + level/depth pull)
    - `prioritize(Vec2 others, Vec2 edges, float boostMul, const Tuning&, float* edgeFrac)`
    - `swim(Fish&, Vec2 force, Vec2 nav, float edgeFrac, float dt, const Tuning&, const FxTuning&)` (turn-rate and pitch limits, minimum speed, flip trigger with debounce and cooldown, leap and airborne cases)
    - `advanceTurn(Fish&, float dt, const Tuning&, const FxTuning&)` (flip animation, safety net, tilt smoothing)

- [ ] **Step 1: Write the failing tests** (`test_fish`)
  ```cpp
  test_tuning_order_pins_core_and_fx          // positional initializers == defaults (in test_tuning)
  test_rigid_body_is_straight_row_behind_head // facing right, tilt 0: x = pos.x − j·spacing, y == pos.y
  test_rigid_body_squeezes_about_middle        // squeeze 0.5: middle point unchanged, ends at half distance
  test_edges_zero_in_interior_and_sides_gated  // sideWalls=false near x=5 → x force 0; true → > 0
  test_edges_ignore_surface_while_leaping
  test_wander_never_points_backward           // 2,000 calls: angle(force, vel) ≤ 30.1°
  test_prioritize_scales_others_near_walls    // edges at full strength → others limited to 0.4·maxForce, edgeFrac == 1
  test_swim_turn_rate_is_limited              // force straight up: heading change per step ≤ maxTurnRate·dt·(1+e)
  test_swim_pitch_is_limited                  // after 200 steps of upward force: |pitch| ≤ maxPitch + 1e-3
  test_swim_keeps_min_speed                   // zero force: speed ≥ minSpeedFrac·maxSpeed
  test_reversal_starts_flip_after_debounce    // nav straight back: no flip at 0.2 s, flip by 0.3 s
  test_flip_reverses_at_midpoint              // vx sign and facing flip exactly once, when turnP crosses 0.5
  test_flip_cooldown_blocks_unless_wall       // second reversal within 3 s blocked; allowed when edgeFrac > 0.3
  test_safety_net_starts_flip_on_mismatch     // facingRight with vx < −threshold → flip starts, never an instant swap
  test_tilt_limited_and_smoothed              // steady dive: |tilt| ≤ maxTilt; first step moves ≤ dt·4 of the gap
  ```
- [ ] **Step 2: Run, confirm they fail:** `pio test -e native -f test_fish -f test_tuning`
- [ ] **Step 3: Implement**: port `steerEdges`, `steerWander`, the level/depth force, the force mixing, the swim integration and the turn/tilt block from `prototype/index.html` `step()`.
- [ ] **Step 4: Run, confirm they pass**, plus the full suite with no warnings.
- [ ] **Step 5: Commit** `feat(tanksim): tuning v2, palettes, fish kinematics (rigid body, flip-turns, wall priority)`

---

### Task 4: Schooling. **Owner: human (learning task)**

**Files:**
- Create: `lib/tanksim/school.h`, `lib/tanksim/school.cpp` (the assistant writes a stub that returns `{}`, marked `TODO(human)`)
- Test: `test/test_school/test_main.cpp` (written by the assistant; fails until the human implements it)

**Interfaces:**
- Consumes: `Fish` (with `body[]`/`bodyN` filled), `Tuning`, `FxTuning`.
- Produces: `Vec2 steerSchool(const Fish& self, const Fish* fish, int count, const Tuning& t, const FxTuning& fx);` It skips `self` (by address), `away` fish and `airborne` fish. Same-layer separation is measured to the **nearest body point** within `separationRadius × {0.7, 1.0, 1.35}[layer]`. Fish in other layers get spacing at `0.7×` that radius, weighted by `layerSpacing`. Alignment and cohesion use same-layer fish whose **heads** are within `neighborRadius`. Each component is a Reynolds "seek" (`normalize(dir)·maxSpeed[layer] − vel`, or zero for a zero direction), combined with the three weights.

- [ ] **Step 1: Tests**
  ```cpp
  test_school_lone_fish_is_zero
  test_school_ignores_away_and_airborne
  test_school_separation_pushes_away_from_body   // neighbour's head far, its TAIL 6 px away → push away from the tail
  test_school_cross_layer_push_is_weaker         // same geometry, other layer → nonzero and smaller than same-layer
  test_school_alignment_matches_heading          // only alignment: neighbour moving +x, self still → x > 0
  test_school_cohesion_pulls_toward_center       // only cohesion: neighbour 40 px right → x > 0
  test_school_far_neighbors_ignored              // > neighborRadius and beyond separation → {0,0}
  test_school_coincident_fish_is_finite          // Review Focus #1
  ```
- [ ] **Step 2: Run, confirm they fail** (stub).
- [ ] **Step 3: The human implements `steerSchool`.** It's a Learn by Doing task; the prototype's JS `steerSchool` is the behavioral reference.
- [ ] **Step 4: All 8 pass; full suite green.**
- [ ] **Step 5: Commit** `feat(tanksim): body-aware schooling`

Tasks 5–6 don't wait for this: the stub returns `{}` (fish without schooling), and their test gates run `pio test -e native -i test_school` until Task 4 lands.

---

### Task 5: Ambient life, seabed and the `Tank` world

**Files:**
- Create: `lib/tanksim/pool.h`, `lib/tanksim/ambient.h`, `lib/tanksim/ambient.cpp`, `lib/tanksim/seabed.h`, `lib/tanksim/seabed.cpp`, `lib/tanksim/stepper.h`, `lib/tanksim/tank.h`, `lib/tanksim/tank.cpp`
- Test: `test/test_ambient/test_main.cpp`, `test/test_seabed/test_main.cpp`, `test/test_tank/test_main.cpp`

**Interfaces:**
- Consumes: Tasks 2–4.
- Produces:
  - `template <class T, int N> struct Pool` with `add(const T&) → bool` (false when full), `removeIf(pred)`, iteration and `count`.
  - `Bubble`, `Particle {pos, vel, life, maxLife, rgb, ch, kind}`, `Ripple {x, t0, amp}`, `Plankton {pos, vel, phase, rgb}`.
  - `seaweedSway(hf, t, phase, Tuning)`, `surfaceAt(ripples, x, t)`.
  - `RockCell {x, y, ch, top, moss}`, `Rock {x, w, h, tone, lMul, hueShift, star, cells[64], cellN}`, `Seaweed`, `Coral {x, tpl, color, phase}`, `Sand`, `Ray`, `Seabed`, `buildSeabed(Rng&, Seabed&)` (pack rocks/weeds/coral left→right with even gaps, as in the prototype), `kCoral[5]` templates.
  - `FixedStepper(stepSec, maxSteps).advance(elapsed) → int`.
  - `class Tank { Tank(const Tuning&, const FxTuning&, uint32_t seed); void step(float dt); fish() / fishCount() / bubbles() / particles() / ripples() / plankton() / seabed() / time() / tuning() / fx() / awayCount(); bool tryLeap(); }`. `step()` ports the prototype's `step()`: the away cap and side walls, a force snapshot, `prioritize`, `swim`, `advanceTurn`, body points, the crossing/away/re-enter logic, darts (only when the water ahead is clear), leaps, splashes, bubbles, particles, ripples and plankton. No taps (that's the v2 feed button).

- [ ] **Step 1: Tests**
  ```cpp
  // ambient
  test_pool_add_fails_when_full_and_remove_compacts
  test_seaweed_root_fixed_and_sway_bounded
  test_surface_flat_without_ripples_and_bounded_with
  // seabed
  test_seabed_pieces_never_overlap_for_50_seeds          // packing invariant
  test_rock_cells_follow_outline                         // top cells use / \ -, cells inside rock height
  // tank
  test_tank_spawns_per_layer_counts_and_stratified_depths
  test_stepper_runs_whole_steps
  test_stepper_caps_steps_and_drops_excess               // Review Focus #2
  test_same_seed_is_deterministic                        // 600 steps, bit-identical fish
  test_fish_leave_and_return                             // within 3 sim-min: ≥1 fish goes away and comes back ≤ returnDelayMax + 0.1 s
  test_away_cap_holds                                    // away ≤ floor(maxAwayFrac·N) + 1 at every step
  test_leap_goes_airborne_and_splashes                   // tryLeap() → airborne within 1 s, particles > 0
  test_pools_drop_when_full                              // Review Focus #5
  test_soak_facing_always_matches                        // Review Focus #3
  test_soak_100k_steps_finite_bounded_and_populated      // Review Focus #4: no NaN, y ∈ [−60, 240], on-screen ≥ 1 always, every away fish returns
  ```
- [ ] **Step 2: Run, confirm they fail.**
- [ ] **Step 3: Implement** by porting the prototype.
- [ ] **Step 4:** `pio test -e native -i test_school` all pass (school excluded until Task 4), no warnings.
- [ ] **Step 5: Commit** `feat(tanksim): Tank world — ambient life, seabed, coming & going, leaps`

---

### Task 6: Framebuffer renderer, firmware and Wokwi

**Files:**
- Create: `lib/tankrender/glyphs.h`, `lib/tankrender/glyphs.cpp`, `lib/tankrender/color.h`, `lib/tankrender/frame.h`, `lib/tankrender/background.h`, `lib/tankrender/background.cpp`, `lib/tankrender/renderer.h`, `lib/tankrender/renderer.cpp`
- Create: `src/display/lgfx_config.h`, `wokwi.toml`, `diagram.json`. Modify: `src/main.cpp`
- Test: `test/test_render/test_main.cpp`

**Interfaces:**
- Consumes: `tank::Tank` and the sim data types.
- Produces:
  - `glyphRows(char) → const uint8_t*` (7 rows, the prototype's `GLYPH_ART`, or null for unknown or space)
  - `rgb565()`, `hsl565()`, `rgbToHsl()`
  - `struct Frame { uint16_t* px; int w, h; uint32_t glyphs, writes; blit(ch, cx, cy, scale, color); plot(); addRgb(); }`
  - `buildBackground(uint16_t* out, const FxTuning&)` (Bayer-dithered RGB565, black edges)
  - `renderFrame(const Tank&, const uint16_t* background, Frame&)` (a port of `drawDevice`)
- Firmware (`src/main.cpp`): framebuffer `heap_caps_malloc(…, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)` with a PSRAM fallback; background in PSRAM; a `Tank` seeded with `esp_random()`; a `FixedStepper` driving the 60 Hz sim; `renderFrame` → `lcd.pushImage`; a 5 s task watchdog; and a 5 s log line `fps=… render_us=… push_us=… glyphs=… heap=… psram=…`.

- [ ] **Step 1: Tests**
  ```cpp
  test_glyph_table_covers_every_scene_character   // every char used by fish, coral, rocks, sand, particles, surface
  test_blit_writes_expected_pixels_and_clips      // '|' at scale 1 → 7 px in one column; off-screen blit writes 0 px
  test_blit_fractional_scale_covers_scaled_box    // scale 2 '#' writes 4× the pixels of scale 1
  test_background_edges_are_black                 // whole border ring == 0x0000, and everything above the surface
  test_background_has_glow_inside                 // centre pixel non-black
  test_render_frame_is_deterministic              // same seed + steps → identical frame
  test_render_budget                              // after 600 steps: glyphs ≤ 450 per frame
  test_render_snapshot (optional)                 // if GHOST_SNAPSHOT is set, write frame to .pio/snapshot.ppm for a visual check
  ```
- [ ] **Step 2: Run, confirm they fail.**
- [ ] **Step 3: Implement** the renderer (port `drawDevice`), then the firmware and the Wokwi files (ESP32-S3 + ILI9341 wired as in DESIGN §9).
- [ ] **Step 4: Verify:**
  - `pio test -e native -i test_school` passes.
  - `GHOST_SNAPSHOT=1 pio test -e native -f test_render` writes a snapshot that matches the prototype's device preview (checked by eye).
  - `pio run -e esp32s3` succeeds.
  - Wokwi run: fish visible and mirrored; serial `fps=` lines. This needs the Wokwi VS Code extension or a `WOKWI_CLI_TOKEN`; otherwise the step is deferred to the human.
- [ ] **Step 5: Tick the ROADMAP boxes that are actually done, and commit** `feat: framebuffer renderer + firmware main loop (Wokwi-ready)`
