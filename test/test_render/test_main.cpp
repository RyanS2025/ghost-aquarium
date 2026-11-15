#include <unity.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "background.h"
#include "color.h"
#include "glyphs.h"
#include "palette.h"
#include "renderer.h"
#include "seabed.h"
#include "tank.h"

using tank::render::Frame;

namespace {
constexpr int W = tank::kScreenW, H = tank::kScreenH;
constexpr int kSeabedTop = H * 3 / 4;  // rocks, sand and starfish are all below this row
const tank::Tuning kT{};
const tank::FxTuning kFx{};

std::vector<uint16_t> background() {
  std::vector<uint16_t> bg(W * H);
  tank::render::buildBackground(bg.data(), kFx);
  return bg;
}
}  // namespace

void setUp() {}
void tearDown() {}

void test_glyph_table_covers_every_scene_character() {
  std::string used = "><(')" "/\\-()#%:\"*" ",.`o" "'.*o" "~'";
  for (const auto& c : tank::kCoral)
    for (int r = 0; r < c.rowN; ++r) used += c.rows[r];
  for (char ch : used) {
    if (ch == ' ') continue;
    TEST_ASSERT_NOT_NULL_MESSAGE(tank::render::glyphRows(ch), std::string(1, ch).c_str());
  }
  TEST_ASSERT_NULL(tank::render::glyphRows(' '));
}

void test_blit_writes_expected_pixels_and_clips() {
  std::vector<uint16_t> px(W * H, 0);
  Frame f{px.data(), W, H};
  f.blit('|', 100.f, 100.f, 1.f, 0xFFFF);
  TEST_ASSERT_EQUAL_UINT32(7, f.writes);  // a 1×7 vertical bar
  int cols = 0;
  for (int x = 0; x < W; ++x) {
    bool any = false;
    for (int y = 0; y < H; ++y) any |= px[y * W + x] != 0;
    cols += any;
  }
  TEST_ASSERT_EQUAL_INT(1, cols);
  f.writes = 0;
  f.blit('#', -100.f, 50.f, 1.f, 0xFFFF);  // fully off-screen
  TEST_ASSERT_EQUAL_UINT32(0, f.writes);
}

void test_blit_fractional_scale_covers_scaled_box() {
  std::vector<uint16_t> px(W * H, 0);
  Frame a{px.data(), W, H}, b{px.data(), W, H};
  a.blit('#', 30.f, 60.f, 1.f, 1);
  b.blit('#', 90.f, 60.f, 2.f, 1);
  TEST_ASSERT_EQUAL_UINT32(a.writes * 4, b.writes);
}

void test_background_edges_are_black() {
  const auto bg = background();
  for (int x = 0; x < W; ++x) {
    TEST_ASSERT_EQUAL_HEX16(0, bg[x]);
    TEST_ASSERT_EQUAL_HEX16(0, bg[(H - 1) * W + x]);
  }
  for (int y = 0; y < H; ++y) {
    TEST_ASSERT_EQUAL_HEX16(0, bg[y * W]);
    TEST_ASSERT_EQUAL_HEX16(0, bg[y * W + W - 1]);
  }
  for (int y = 0; y < static_cast<int>(tank::kSurfaceY); ++y)
    for (int x = 0; x < W; ++x) TEST_ASSERT_EQUAL_HEX16(0, bg[y * W + x]);  // air above the water
}

void test_background_has_glow_inside() {
  const auto bg = background();
  TEST_ASSERT_NOT_EQUAL(0, bg[(H / 2) * W + W / 2]);
}

void test_render_frame_is_deterministic() {
  const auto bg = background();
  tank::Tank a(kT, kFx, 42), b(kT, kFx, 42);
  for (int i = 0; i < 300; ++i) { a.step(1.f / 60.f); b.step(1.f / 60.f); }
  std::vector<uint16_t> pa(W * H), pb(W * H);
  Frame fa{pa.data(), W, H}, fb{pb.data(), W, H};
  tank::render::renderFrame(a, bg.data(), fa);
  tank::render::renderFrame(b, bg.data(), fb);
  TEST_ASSERT_EQUAL_MEMORY(pa.data(), pb.data(), pa.size() * sizeof(uint16_t));
  TEST_ASSERT_TRUE(fa.glyphs > 50);  // something was actually drawn
}

void test_render_budget() {
  const auto bg = background();
  tank::Tank tk(kT, kFx, 7);
  std::vector<uint16_t> px(W * H);
  uint32_t maxGlyphs = 0;
  for (int i = 0; i < 600; ++i) {
    tk.step(1.f / 60.f);
    if (i % 30) continue;
    Frame f{px.data(), W, H};
    tank::render::renderFrame(tk, bg.data(), f);
    if (f.glyphs > maxGlyphs) maxGlyphs = f.glyphs;
  }
  TEST_ASSERT_TRUE(maxGlyphs <= 450);
}

// Without PSRAM there may be no background buffer: the frame must still render (on black).
void test_render_without_background_clears_to_black() {
  tank::Tank tk(kT, kFx, 3);
  std::vector<uint16_t> px(W * H, 0x1234);
  Frame f{px.data(), W, H};
  tank::render::renderFrame(tk, nullptr, f);
  TEST_ASSERT_EQUAL_HEX16(0, px[5 * W + 5]);  // top-left air: cleared, nothing drawn there
}

// Visual check: GHOST_SNAPSHOT=/path/frame.ppm pio test -e native -f test_render
void test_render_snapshot() {
  const char* path = std::getenv("GHOST_SNAPSHOT");
  if (!path) TEST_IGNORE_MESSAGE("set GHOST_SNAPSHOT=<file.ppm> to write a frame");
  const auto bg = background();
  tank::Tank tk(kT, kFx, 42);
  for (int i = 0; i < 60 * 8; ++i) tk.step(1.f / 60.f);
  std::vector<uint16_t> px(W * H);
  Frame f{px.data(), W, H};
  tank::render::renderFrame(tk, bg.data(), f);
  FILE* out = std::fopen(path, "wb");
  TEST_ASSERT_NOT_NULL(out);
  std::fprintf(out, "P6\n%d %d\n255\n", W, H);
  for (uint16_t c : px) {
    const uint8_t r = ((c >> 11) & 31) << 3, g = ((c >> 5) & 63) << 2, b = (c & 31) << 3;
    std::fputc(r | (r >> 5), out); std::fputc(g | (g >> 6), out); std::fputc(b | (b >> 5), out);
  }
  std::fclose(out);
}

// Pins the framebuffer convention: native RGB565 (red = 0xF800). The firmware must therefore
// call lcd.setSwapBytes(true) before pushImage (LovyanGFX reads a plain uint16_t* as byte-swapped).
void test_framebuffer_is_native_rgb565() {
  TEST_ASSERT_EQUAL_HEX16(0xF800, tank::render::rgb565(255, 0, 0));
  TEST_ASSERT_EQUAL_HEX16(0x07E0, tank::render::rgb565(0, 255, 0));
  TEST_ASSERT_EQUAL_HEX16(0x001F, tank::render::rgb565(0, 0, 255));
}

// Review: renderFrame assumed 320×240; a band-sized Frame (ADR 0008's banding upgrade) got
// writes far past its end. Until banding exists, any other size must be refused untouched.
void test_render_refuses_non_full_frames() {
  const auto bg = background();
  tank::Tank tk(kT, kFx, 3);
  std::vector<uint16_t> px(W * H, 0xABCD);
  Frame band{px.data(), W, 40};
  tank::render::renderFrame(tk, bg.data(), band);
  for (int i = 0; i < W * H; ++i) TEST_ASSERT_EQUAL_HEX16(0xABCD, px[i]);
}

// Performance path (measured in Wokwi: rays were 47% of render time, rocks + sand 10%):
// static scenery is baked into the background once, and rays live in a backdrop buffer that is
// refreshed one band per frame. Above the seabed the result must equal the live render.
void test_baked_backdrop_matches_live_render_above_seabed() {
  auto bg = background();
  tank::Tank tk(kT, kFx, 11);
  for (int i = 0; i < 240; ++i) tk.step(1.f / 60.f);

  std::vector<uint16_t> live(W * H), baked(W * H), backdrop(W * H);
  Frame fl{live.data(), W, H};
  tank::render::renderFrame(tk, bg.data(), fl);

  tank::render::bakeStaticScenery(tk, bg.data());
  for (int band = 0; band < tank::render::kRayBands; ++band) tank::render::refreshBackdrop(tk, bg.data(), backdrop.data(), band);
  Frame fb{baked.data(), W, H};
  tank::render::renderFrame(tk, backdrop.data(), fb, tank::render::kBackdropHasRaysAndScenery);

  for (int y = 0; y < kSeabedTop; ++y)
    for (int x = 0; x < W; ++x) TEST_ASSERT_EQUAL_HEX16(live[y * W + x], baked[y * W + x]);
  TEST_ASSERT_TRUE(fb.glyphs < fl.glyphs);  // rocks + sand no longer drawn per frame
}

void test_refresh_backdrop_touches_only_its_band() {
  const auto bg = background();
  tank::Tank tk(kT, kFx, 2);
  std::vector<uint16_t> backdrop(W * H, 0xBEEF);
  tank::render::refreshBackdrop(tk, bg.data(), backdrop.data(), 3);
  const int rowsPerBand = H / tank::render::kRayBands;
  for (int y = 0; y < H; ++y) {
    const bool inBand = y >= 3 * rowsPerBand && y < 4 * rowsPerBand;
    if (!inBand) TEST_ASSERT_EQUAL_HEX16(0xBEEF, backdrop[y * W + 10]);
  }
  TEST_ASSERT_NOT_EQUAL(0xBEEF, backdrop[(3 * rowsPerBand) * W + 10]);
}

void test_bake_static_scenery_only_touches_the_seabed() {
  auto bg = background();
  const auto before = bg;
  tank::Tank tk(kT, kFx, 2);
  tank::render::bakeStaticScenery(tk, bg.data());
  for (int y = 0; y < kSeabedTop; ++y)
    for (int x = 0; x < W; ++x) TEST_ASSERT_EQUAL_HEX16(before[y * W + x], bg[y * W + x]);
  int changed = 0;
  for (size_t i = 0; i < bg.size(); ++i) changed += bg[i] != before[i];
  TEST_ASSERT_TRUE(changed > 100);  // rocks and sand were drawn
}

// Demo frames for the README GIF, through the exact firmware path (baked scenery + one ray band
// per frame). Opt-in: GHOST_FRAMES=<dir> pio test -e native -f test_render  (see `make gif`).
void test_render_demo_frames() {
  const char* dir = std::getenv("GHOST_FRAMES");
  if (!dir) TEST_IGNORE_MESSAGE("set GHOST_FRAMES=<dir> to write demo frames");
  auto bg = background();
  tank::Tank tk(kT, kFx, 42);
  for (int i = 0; i < 60 * 4; ++i) tk.step(1.f / 60.f);
  tank::render::bakeStaticScenery(tk, bg.data());
  std::vector<uint16_t> backdrop(W * H), px(W * H);
  for (int b = 0; b < tank::render::kRayBands; ++b) tank::render::refreshBackdrop(tk, bg.data(), backdrop.data(), b);
  constexpr int kFrames = 200, kStepsPerFrame = 3;  // 20 fps GIF, real-time motion
  for (int n = 0; n < kFrames; ++n) {
    for (int s = 0; s < kStepsPerFrame; ++s) tk.step(1.f / 60.f);
    if (n % 40 == 10) tk.tryLeap();  // show off a leap + splash when a fish is in position
    tank::render::refreshBackdrop(tk, bg.data(), backdrop.data(), n % tank::render::kRayBands);
    Frame f{px.data(), W, H};
    tank::render::renderFrame(tk, backdrop.data(), f, tank::render::kBackdropHasRaysAndScenery);
    char path[512];
    std::snprintf(path, sizeof(path), "%s/frame_%03d.ppm", dir, n);
    FILE* out = std::fopen(path, "wb");
    TEST_ASSERT_NOT_NULL(out);
    std::fprintf(out, "P6\n%d %d\n255\n", W, H);
    for (uint16_t c : px) {
      const uint8_t r = ((c >> 11) & 31) << 3, g = ((c >> 5) & 63) << 2, b = (c & 31) << 3;
      std::fputc(r | (r >> 5), out); std::fputc(g | (g >> 6), out); std::fputc(b | (b >> 5), out);
    }
    std::fclose(out);
  }
}

// Banded rendering (ADR 0008 §7): the display list is built once per frame, then rasterised
// into 32-row bands (as the firmware does) that DMA can send while the next band is drawn. Stitched bands must be
// byte-identical to the full-frame render, in both the live and the baked-backdrop modes.
namespace {
void expectBandsEqualFullFrame(uint32_t flags) {
  auto bg = background();
  tank::Tank tk(kT, kFx, 21);
  for (int i = 0; i < 300; ++i) tk.step(1.f / 60.f);
  std::vector<uint16_t> source = bg;
  if (flags & tank::render::kBackdropHasRaysAndScenery) {
    tank::render::bakeStaticScenery(tk, bg.data());
    for (int b = 0; b < tank::render::kRayBands; ++b) tank::render::refreshBackdrop(tk, bg.data(), source.data(), b);
  }
  std::vector<uint16_t> full(W * H);
  Frame ff{full.data(), W, H};
  tank::render::renderFrame(tk, source.data(), ff, flags);

  static tank::render::DisplayList list;
  tank::render::buildDisplayList(tk, list, flags);
  constexpr int kBandH = 32;
  static_assert(H % kBandH == 0, "bands must tile the screen");
  std::vector<uint16_t> stitched(W * H), band(W * kBandH);
  for (int b = 0; b < H / kBandH; ++b) {
    Frame fb{band.data(), W, kBandH};
    fb.originY = b * kBandH;
    tank::render::rasterize(tk, list, source.data(), fb, flags);
    std::memcpy(stitched.data() + b * kBandH * W, band.data(), band.size() * sizeof(uint16_t));
  }
  TEST_ASSERT_EQUAL_MEMORY(full.data(), stitched.data(), full.size() * sizeof(uint16_t));
}
}  // namespace

void test_banded_render_equals_full_frame_live() { expectBandsEqualFullFrame(tank::render::kLive); }
void test_banded_render_equals_full_frame_baked() { expectBandsEqualFullFrame(tank::render::kBackdropHasRaysAndScenery); }

// DMA sends the band as-is, so it must already be in the panel's byte order.
void test_swap_bytes_in_place() {
  uint16_t px[3] = {0xF800, 0x1234, 0x00FF};
  tank::render::swapBytes(px, 3);
  TEST_ASSERT_EQUAL_HEX16(0x00F8, px[0]);
  TEST_ASSERT_EQUAL_HEX16(0x3412, px[1]);
  TEST_ASSERT_EQUAL_HEX16(0xFF00, px[2]);
}

void test_display_list_fits_a_busy_frame() {
  tank::Tank tk(kT, kFx, 7);
  static tank::render::DisplayList list;
  for (int i = 0; i < 20; ++i) tk.splash(6.f + 6.f * i, 200.f, tank::kWhite);  // particle pool full
  tk.step(1.f / 60.f);
  tank::render::buildDisplayList(tk, list, tank::render::kLive);
  TEST_ASSERT_EQUAL_UINT32(0, list.dropped);
  TEST_ASSERT_TRUE(list.n < tank::render::kMaxOps);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_banded_render_equals_full_frame_live);
  RUN_TEST(test_banded_render_equals_full_frame_baked);
  RUN_TEST(test_swap_bytes_in_place);
  RUN_TEST(test_display_list_fits_a_busy_frame);
  RUN_TEST(test_render_demo_frames);
  RUN_TEST(test_baked_backdrop_matches_live_render_above_seabed);
  RUN_TEST(test_refresh_backdrop_touches_only_its_band);
  RUN_TEST(test_bake_static_scenery_only_touches_the_seabed);
  RUN_TEST(test_framebuffer_is_native_rgb565);
  RUN_TEST(test_render_refuses_non_full_frames);
  RUN_TEST(test_glyph_table_covers_every_scene_character);
  RUN_TEST(test_blit_writes_expected_pixels_and_clips);
  RUN_TEST(test_blit_fractional_scale_covers_scaled_box);
  RUN_TEST(test_background_edges_are_black);
  RUN_TEST(test_background_has_glow_inside);
  RUN_TEST(test_render_frame_is_deterministic);
  RUN_TEST(test_render_budget);
  RUN_TEST(test_render_without_background_clears_to_black);
  RUN_TEST(test_render_snapshot);
  return UNITY_END();
}
