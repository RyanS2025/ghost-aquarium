#pragma once
// 5×7 bitmap glyphs (classic GLCD style, as in LovyanGFX's built-in font), drawn in a 6×8 cell.
// Same table as GLYPH_ART in prototype/index.html.

#include <cstdint>

namespace tank::render {

// 7 rows; bit 4 (16) is the leftmost column. nullptr for space or unknown characters.
const uint8_t* glyphRows(char ch);

}  // namespace tank::render
