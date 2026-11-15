#pragma once
// A plain RGB565 framebuffer the renderer draws into (the firmware owns the memory and pushes it
// to the panel). Only cheap operations: glyph blits at whole pixels, pixel plots, saturating adds.

#include <cmath>
#include <cstdint>

#include "glyphs.h"

namespace tank::render {

struct Frame {
  uint16_t* px;
  int w, h;
  int originY = 0;      // screen row of px[0]: a band of a taller screen (ADR 0008 §7); 0 = full frame
  uint32_t glyphs = 0;  // stats for the budget (ADR 0008)
  uint32_t writes = 0;

  // Optional per-stage timing: the firmware passes a µs clock (micros); null on the laptop.
  using ClockFn = uint32_t (*)();
  ClockFn clock = nullptr;
  enum Stage { kBackground, kRays, kPlankton, kRocks, kSeaweed, kCoral, kSand, kFish, kAmbient, kRaster, kStageCount };
  uint32_t stageUs[kStageCount] = {};

  // Glyph centred at (cx, cy) in screen coordinates, nearest-neighbour scaled by s, 6×8 cell.
  void blit(char ch, float cx, float cy, float s, uint16_t color) {
    blitAt(ch, roundHalfUp(cx - 6.f * s / 2.f), roundHalfUp(cy - 8.f * s / 2.f), s, color);
  }

  // Glyph with its cell's top-left at screen (x0, y0); clipped to this frame's rows and columns.
  void blitAt(char ch, int x0, int sy0, float s, uint16_t color) {
    const uint8_t* g = glyphRows(ch);
    if (!g) return;
    ++glyphs;
    const int y0 = sy0 - originY;
    if (y0 >= h || y0 + static_cast<int>(8.f * s) < 0) return;  // whole glyph outside this band
    for (int r = 0; r < 7; ++r) {
      const uint8_t bits = g[r];
      if (!bits) continue;
      const int ya = clampi(y0 + static_cast<int>(std::floor(r * s)), 0, h);
      const int yb = clampi(y0 + static_cast<int>(std::floor((r + 1) * s)), 0, h);
      if (ya >= yb) continue;
      for (int c = 0; c < 5; ++c) {
        if (!(bits & (16 >> c))) continue;
        const int xa = clampi(x0 + static_cast<int>(std::floor(c * s)), 0, w);
        const int xb = clampi(x0 + static_cast<int>(std::floor((c + 1) * s)), 0, w);
        for (int y = ya; y < yb; ++y)
          for (int x = xa; x < xb; ++x) { px[y * w + x] = color; ++writes; }
      }
    }
  }

  void plot(float fx, float fy, uint16_t color) { plotAt(roundHalfUp(fx), roundHalfUp(fy), color); }
  void plotAt(int x, int sy, uint16_t color) {
    const int y = sy - originY;
    if (x >= 0 && x < w && y >= 0 && y < h) { px[y * w + x] = color; ++writes; }
  }

  // Saturating add of (r5, g6, b5) to pixel index o (light rays).
  void add565(int o, int r5, int g6, int b5) {
    const uint16_t c = px[o];
    const int r = clampi(((c >> 11) & 31) + r5, 0, 31), g = clampi(((c >> 5) & 63) + g6, 0, 63), b = clampi((c & 31) + b5, 0, 31);
    px[o] = static_cast<uint16_t>((r << 11) | (g << 5) | b);
    ++writes;
  }

  static int roundHalfUp(float v) { return static_cast<int>(std::floor(v + 0.5f)); }  // = JS Math.round
  static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
};

}  // namespace tank::render
