#include <unity.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "background.h"
#include "glyphs.h"
#include "renderer.h"
#include "seabed.h"
#include "tank.h"

using tank::render::Frame;

namespace {
constexpr int W = 320, H = 240;
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
  a.blit('#', 100.f, 100.f, 1.f, 1);
  b.blit('#', 200.f, 100.f, 2.f, 1);
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
  TEST_ASSERT_NOT_EQUAL(0, bg[110 * W + 160]);
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
  TEST_ASSERT_TRUE(fa.glyphs > 100);  // something was actually drawn
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

int main() {
  UNITY_BEGIN();
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
