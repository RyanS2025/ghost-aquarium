# Phase 0: Simulation and Renderer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Get the full ghost tank (schooling ASCII fish in 3 depth layers, seaweed, bubbles) running in the Wokwi ESP32-S3 simulator from a unit-tested simulation library, without buying any hardware.

**Architecture:** A browser prototype sets the motion and color values. The hardware-free C++ library `lib/tanksim` (namespace `tank`) holds all simulation state and logic and is tested on the laptop with Unity. The firmware in `src/` runs a fixed 60 Hz simulation, draws each frame into a LovyanGFX full-screen sprite, and pushes it to an ILI9341 over DMA, with the display controller mirroring it for the Pepper's Ghost reflection.

**Tech Stack:** C++17, PlatformIO (`espressif32` platform, Arduino framework, `native` platform), LovyanGFX `^1.2.0`, Unity, Wokwi VS Code extension, GitHub Actions; vanilla HTML/JS canvas for the prototype.

**Spec:** [docs/DESIGN.md](../DESIGN.md). Decisions: [ADR 0003](../adr/0003-rendering-lovyangfx-sprite.md), [0004](../adr/0004-toolchain-platformio-wokwi.md), [0005](../adr/0005-motion-subpixel-steering.md), [0006](../adr/0006-host-testable-sim-library.md), [0007](../adr/0007-browser-prototype-first.md).

## Global Constraints

- Screen: 320×240 landscape. `tank::Bounds{320.f, 240.f}`.
- C++17. No exceptions or RTTI in `lib/tanksim`.
- `lib/tanksim` includes only standard-library headers. No `Arduino.h`, ESP-IDF or LovyanGFX.
- No heap allocation in `Tank::step()` or `Renderer::draw()` after setup. Reserve vector capacity in constructors.
- All simulation randomness comes from `tank::Rng`. No `rand()`, `millis()` or `esp_random()` in `lib/tanksim`.
- Simulation step is `1.f / 60.f` s, with at most 5 steps per loop.
- Tuning field names and defaults exactly as in [DESIGN §6](../DESIGN.md#6-tuning-parameters-starting-values).
- Pins exactly as in [DESIGN §9](../DESIGN.md#pin-map-esp32-s3). Never use GPIO 0, 3, 35–37, 45 or 46.
- Glyphs are ASCII only, as in [DESIGN §4](../DESIGN.md#species-ascii-glyphs).
- Layer index: `Far = 0`, `Mid = 1`, `Near = 2`. In v1 the species index equals the layer index.

## Review Focus

1. **Zero-length vectors** (two fish at the same spot, a stationary fish). `normalized()` must return `{0,0}`, not NaN, or a fish silently disappears. Tests: Task 2 (`test_normalized_zero_is_zero`) and Task 4 (`test_school_coincident_fish_is_finite`).
2. **Frame stalls** (a slow push, WiFi in v2). One huge `dt` would make fish tunnel through walls. Run at most 5 fixed steps and drop the rest. Test: Task 5 (`test_stepper_caps_steps_and_drops_excess`).
3. **Vertical swimming.** When `vel.x ≈ 0` the glyph must not flip every frame. Test: Task 3 (`test_facing_hysteresis_holds_below_threshold`).
4. **Running for hours.** No NaN, no fish leaving the screen, no slow drift into one corner. Test: Task 5 (`test_soak_100k_steps_finite_and_bounded`).
5. **Seed 0.** A raw xorshift seeded with 0 outputs 0 forever, which freezes wander and stacks every fish in one place. Test: Task 2 (`test_rng_seed_zero_still_varies`).

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

### Task 3: Fish kinematics: integrate, facing, edges, wander

**Files:**
- Create: `lib/tanksim/fish.h`, `lib/tanksim/fish.cpp`
- Test: `test/test_fish/test_main.cpp`

**Interfaces:**
- Consumes: `Vec2`, `Rng`, `Tuning`, `Bounds`, `Layer` (Task 2).
- Produces:
  - `struct tank::Fish { Vec2 pos, vel; Layer layer; uint8_t species; float wanderAngle; bool facingRight; };`
  - `void tank::integrate(Fish& f, Vec2 steering, float dt, float maxSpeed, float maxForce);` Caps the steering at `maxForce`, then `vel += steering*dt`, caps `vel` at `maxSpeed`, then `pos += vel*dt`.
  - `void tank::updateFacing(Fish& f, float threshold);` Sets `facingRight = vel.x > 0` only when `|vel.x| > threshold`.
  - `Vec2 tank::steerEdges(const Fish& f, const Bounds& b, const Tuning& t);` For each axis, within `edgeMargin` of a wall, pushes inward with magnitude `edgeWeight * maxForce * clamp((edgeMargin - dist)/edgeMargin, 0, 1)`. Past the wall (`dist < 0`) the factor is 1. Zero in the interior.
  - `Vec2 tank::steerWander(Fish& f, Rng& rng, float dt, const Tuning& t);` Does `wanderAngle += rng.uniform(-1,1) * wanderJitter * dt` and returns `{cos, sin}(wanderAngle) * wanderWeight * maxForce`.

- [ ] **Step 1: Write the failing tests**
  ```cpp
  void test_integrate_never_exceeds_max_speed()  // steering {1e6,1e6}, 100 steps of 1/60: vel.length() <= 28 + 1e-3
  void test_integrate_moves_by_velocity()        // vel {10,0}, zero steering, dt 0.5: pos.x advanced by 5
  void test_facing_hysteresis_holds_below_threshold() {  // Review Focus #3
    tank::Fish f{}; f.facingRight = true; f.vel = {-2.f, 20.f};
    tank::updateFacing(f, 3.f); TEST_ASSERT_TRUE(f.facingRight);
    f.vel = {-4.f, 0.f}; tank::updateFacing(f, 3.f); TEST_ASSERT_FALSE(f.facingRight);
  }
  void test_edges_zero_in_interior()             // pos {160,120}: steerEdges == {0,0}
  void test_edges_push_inward_near_left_wall()   // pos {5,120}: x > 0, y == 0
  void test_edges_push_inward_past_right_wall()  // pos {330,120}: x == -(edgeWeight*maxForce)
  void test_wander_magnitude_is_weight_times_force()  // length == wanderWeight*maxForce within 1e-4
  void test_wander_deterministic_for_seed()      // two Rng(7) and identical fish: equal outputs over 50 calls
  ```
- [ ] **Step 2: Run and confirm the tests fail.** `pio test -e native -f test_fish` gives compile errors.
- [ ] **Step 3: Implement** `fish.h` and `fish.cpp` to the Interfaces above.
- [ ] **Step 4: Run and confirm the tests pass.** `pio test -e native -f test_fish` gives 8/8 PASSED.
- [ ] **Step 5: Commit** `git commit -m "feat(tanksim): fish integration, facing hysteresis, edge + wander steering"`

---

### Task 4: Schooling (boids). **Owner: human (learning task)**

**Files:**
- Create: `lib/tanksim/school.h`, `lib/tanksim/school.cpp`
- Test: `test/test_school/test_main.cpp`

**Interfaces:**
- Consumes: `Fish`, `Tuning`, `Vec2` (Tasks 2 and 3).
- Produces: `Vec2 tank::steerSchool(const Fish& self, const std::vector<Fish>& all, const Tuning& t);` Considers only fish in the **same layer** within `neighborRadius`, skipping `self` by address (`&other == &self`). Returns `separationWeight*sep + alignmentWeight*ali + cohesionWeight*coh`. It does not cap the result; `integrate` does.

- [ ] **Step 1: Write the failing tests** (the assistant writes these)
  ```cpp
  void test_school_lone_fish_is_zero()
  void test_school_ignores_other_layers()      // neighbor 5px away in a different layer: result {0,0}
  void test_school_ignores_far_neighbors()     // same-layer neighbor 100px away: {0,0}
  void test_school_separation_pushes_away()    // only separationWeight=1, neighbor 5px to the right: result.x < 0
  void test_school_alignment_matches_heading() // only alignmentWeight=1, self vel {0,0}, neighbor 20px away with vel {10,0}: result.x > 0
  void test_school_cohesion_pulls_toward_center()  // only cohesionWeight=1, neighbor 30px to the right: result.x > 0
  void test_school_coincident_fish_is_finite() // Review Focus #1: two fish at the same pos, all weights 1: isfinite(x) && isfinite(y)
  ```
- [ ] **Step 2: Run and confirm the tests fail.** `pio test -e native -f test_school`
- [ ] **Step 3: The human implements `steerSchool`** (Learn by Doing). Decisions for the human: how separation scales with distance (1/d, 1/d² or linear falloff); whether each component is a raw average or Reynolds' "desired velocity minus current velocity"; how to guard the coincident case.
- [ ] **Step 4: Run and confirm the tests pass.** 7/7 PASSED.
- [ ] **Step 5: Commit** `git commit -m "feat(tanksim): per-layer boids schooling"`

---

### Task 5: Ambient life, the Tank world and the fixed timestep

**Files:**
- Create: `lib/tanksim/ambient.h`, `lib/tanksim/ambient.cpp`, `lib/tanksim/stepper.h`, `lib/tanksim/tank.h`, `lib/tanksim/tank.cpp`
- Test: `test/test_ambient/test_main.cpp`, `test/test_tank/test_main.cpp`

**Interfaces:**
- Consumes: everything from Tasks 2, 3 and 4.
- Produces:
  - `struct tank::Bubble { Vec2 pos; float phase; float rise; };`
  - `struct tank::Seaweed { float x; float height; float phase; };`
  - `Vec2 tank::bubbleDrawPos(const Bubble& b, float t, const Tuning& tu);` Returns `pos` plus an x offset of `bubbleWobbleAmp * sin(2π·bubbleWobbleHz·t + phase)`.
  - `float tank::seaweedSway(float heightFrac, float t, float phase, const Tuning& tu);` Equals `seaweedSwayAmp * heightFrac² * (0.7·sin(2π·hz[0]·t + phase) + 0.3·sin(2π·hz[1]·t + 1.7·phase))`.
  - `class tank::FixedStepper { public: FixedStepper(float stepSec, int maxSteps); int advance(float elapsedSec); };` Returns the number of steps to run. When the count exceeds `maxSteps`, it returns `maxSteps` and resets the accumulator to 0.
  - `class tank::Tank { public: Tank(Bounds b, const Tuning& t, uint32_t seed); void step(float dt); const std::vector<Fish>& fish() const; const std::vector<Bubble>& bubbles() const; const std::vector<Seaweed>& seaweed() const; float time() const; const Bounds& bounds() const; const Tuning& tuning() const; };`
  - The constructor spawns `fishPerLayer[L]` fish per layer at uniform positions inside the edge margin, with a random heading at half the layer speed. It also creates 5 seaweed stalks and reserves 64 bubbles.
  - `step()` first computes **every** fish's steering (school + edges + wander) from the current state, **then** integrates them all. This order keeps the result independent of fish order. Then: update facing; spawn bubbles from a rate accumulator at `y = h + 2`, `x ∈ [8, w-8]`, skipping spawns at capacity; move bubbles up at `rise`; remove those with `y < -4`; `time += dt`.

- [ ] **Step 1: Write the failing tests**
  ```cpp
  // test_ambient
  void test_seaweed_root_does_not_move()       // heightFrac 0, any t: 0
  void test_seaweed_sway_bounded_by_amp()      // heightFrac 1, t over 0..60 s in 0.01 steps: |sway| <= seaweedSwayAmp
  void test_bubble_wobble_bounded()            // |bubbleDrawPos.x - pos.x| <= bubbleWobbleAmp
  // test_tank
  void test_tank_spawns_fish_per_layer()       // counts per layer == {6, 5, 3}
  void test_stepper_runs_whole_steps()         // FixedStepper(1/60, 5).advance(0.034f) == 2
  void test_stepper_caps_steps_and_drops_excess() { // Review Focus #2
    tank::FixedStepper s(1.f/60.f, 5);
    TEST_ASSERT_EQUAL(5, s.advance(1.0f)); TEST_ASSERT_EQUAL(0, s.advance(0.0f));
  }
  void test_bubbles_rise_and_despawn()         // run 20 s: some bubbles spawned; every live bubble has y in [-4, h+2]
  void test_same_seed_is_deterministic()       // two Tank(seed 42), 600 steps: all fish pos bit-identical
  void test_soak_100k_steps_finite_and_bounded()  // Review Focus #4: seed 42, 100,000 steps:
                                                  //   every fish pos finite and in [-edgeMargin, w+edgeMargin] x [-edgeMargin, h+edgeMargin]
  ```
- [ ] **Step 2: Run and confirm the tests fail.** `pio test -e native -f test_ambient -f test_tank`
- [ ] **Step 3: Implement** `ambient`, `stepper.h` (header-only) and `tank` to the Interfaces above.
- [ ] **Step 4: Run and confirm the tests pass.** `pio test -e native` shows every suite PASSED, with the soak test under 10 s on a laptop.
- [ ] **Step 5: Commit** `git commit -m "feat(tanksim): bubbles, seaweed, Tank world, fixed-timestep stepper + soak test"`

---

### Task 6: Display, renderer and main loop in Wokwi

**Files:**
- Create: `src/display/lgfx_config.h`, `src/render/palette.h`, `src/render/renderer.h`, `src/render/renderer.cpp`, `wokwi.toml`, `diagram.json`
- Modify: `src/main.cpp` (replace the Task 2 placeholder)

**Interfaces:**
- Consumes: `tank::Tank`, `tank::FixedStepper` (Task 5); `kLayerColor` from the Task 1 export.
- Produces:
  - `class LGFX : public lgfx::LGFX_Device` with `Panel_ILI9341`, `Bus_SPI` (SPI2_HOST, 40 MHz write, DMA channel `SPI_DMA_CH_AUTO`) and `Light_PWM` on GPIO 21. Pins as in DESIGN §9.
  - `class Renderer { public: bool begin(bool mirrored); void draw(const tank::Tank& t); uint32_t lastDrawMicros() const; uint32_t lastPushMicros() const; };`
  - `begin()` allocates a 320×240 16-bit `LGFX_Sprite`, trying PSRAM first and then internal SRAM. It returns `false` if both fail. `mirrored` selects a mirrored rotation (4–7).
  - `draw()` clears to black, then draws seaweed, then Far, Mid and Near fish (glyph by `facingRight`, color `kLayerColor[layer]`), then bubbles (`o`, or a 1–2 px circle), then `pushSprite(0,0)`.
- `main.cpp`: in `setup()`, start Serial at 115200; call `renderer.begin(true)` and halt with a serial error if it fails; build the `Tank` with seed `esp_random()`; enable the task watchdog (5 s, using the API for the installed Arduino core). In `loop()`: `n = stepper.advance(elapsed)`; run `tank.step(1/60)` n times; `renderer.draw(tank)`; reset the watchdog. Every 5 s, log `fps=<f> draw_us=<u> push_us=<u> heap=<u> psram=<u>`.

- [ ] **Step 1: Write `wokwi.toml`** pointing `firmware` and `elf` at `.pio/build/esp32s3/firmware.bin` and `.elf`. Write `diagram.json` with `board-esp32-s3-devkitc-1` and `wokwi-ili9341`, wired as in DESIGN §9 (LED to GPIO 21).
- [ ] **Step 2: Implement** `lgfx_config.h`, `palette.h`, `renderer`, `main.cpp`.
- [ ] **Step 3: Build.** Run: `pio run -e esp32s3`. Expected: SUCCESS, with no warnings from `lib/tanksim`.
- [ ] **Step 4: Run in Wokwi.** VS Code: "Wokwi: Start Simulator". Expected: a black screen showing 3 visibly separate layers of schooling fish, swaying seaweed and rising bubbles; the image is mirrored (text reads backwards, which is correct for Pepper's Ghost); serial prints an `fps=` line every 5 s. Wokwi's fps is **not** the real target; it gets measured on hardware in Phase 1 (ADR 0004).
- [ ] **Step 5: Record a GIF** of the Wokwi run (with mirroring off, so it's readable) to `docs/media/phase0-wokwi.gif` and embed it in the README.
- [ ] **Step 6: Tick the Phase 0 boxes in `docs/ROADMAP.md` and commit**
  ```bash
  git add src/ wokwi.toml diagram.json docs/
  git commit -m "feat(firmware): LovyanGFX sprite renderer + fixed-step main loop in Wokwi"
  ```
