#pragma once
// Colors (RGB888) from DESIGN §4 / the prototype. Shared by the simulation (splash droplets take
// the fish's head color) and the renderer.

#include <cstdint>

namespace tank {

constexpr int kPaletteCount = 7;
constexpr uint32_t kFishPalette[kPaletteCount][2] = {  // {head, tail}
    {0x00F0FF, 0xFF2D55},  // neon tetra
    {0xFF2BD6, 0x7A3CFF},  // betta
    {0xFFD400, 0xFF5A00},  // goldfish
    {0xFF8C00, 0xFFF1D6},  // clownfish
    {0xB6FF00, 0x00D5A0},  // lime
    {0xFF5E7A, 0xFFC46B},  // coral
    {0x5AD1FF, 0xB07BFF},  // sky
};

constexpr int kReefColorCount = 7;
constexpr uint32_t kReefColors[kReefColorCount] = {0xFF3D9A, 0xFF7A1A, 0xA64DFF, 0xFFD23F,
                                                    0x22E3FF, 0xFF3355, 0x9DFF3A};

struct SceneColors {
  uint32_t waterTop, waterMid, waterDeep, ray, seaweedBase, seaweedTip, sand, rock, bubble, surface;
};
constexpr SceneColors kScene{0x0AA6C9, 0x2340B8, 0x5A1A9A, 0x7FE8FF, 0x0D5C3A,
                             0x7DFF6A, 0xD9A35F, 0x5B5394, 0x9FE8FF, 0x2FD0FF};

constexpr uint32_t kRockTones[3] = {0x5B5394, 0x4D6B86, 0x7A4B3A};  // scene rock, blue-grey, red-brown
constexpr uint32_t kPlanktonCyan = 0x4FE9FF;
constexpr uint32_t kWhite = 0xFFFFFF;

}  // namespace tank
