#pragma once
// LovyanGFX panel setup (pins: docs/DESIGN.md §9). Swapping panels means changing only this file.
//   default            1.5" 128×128 SSD1351 color OLED: the real device (ADR 0010)
//   GHOST_PANEL_SIM=1  2.8" 320×240 ILI9341: Wokwi has no SSD1351 part, so the simulator shows the
//                      128×128 tank 1:1 in the middle of this screen (`make sim` sets the flag)

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#ifndef GHOST_PANEL_SIM
#define GHOST_PANEL_SIM 0
#endif

#if GHOST_PANEL_SIM

// Landscape is rotation 1 (MADCTL MV|MX). Rotation 7 adds MY, which in landscape mirrors
// LEFT-RIGHT for free in the panel (ADR 0003; verified against Panel_LCD::getMadCtl).
constexpr uint8_t kRotationNormal = 1, kRotationMirrored = 7;

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9341 panel_;
  lgfx::Bus_SPI bus_;
  lgfx::Light_PWM light_;

 public:
  LGFX() {
    {
      auto cfg = bus_.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = 12;
      cfg.pin_mosi = 11;
      cfg.pin_miso = 13;
      cfg.pin_dc = 9;
      bus_.config(cfg);
      panel_.setBus(&bus_);
    }
    {
      auto cfg = panel_.config();
      cfg.pin_cs = 10;
      cfg.pin_rst = 14;
      cfg.pin_busy = -1;
      cfg.panel_width = 240;
      cfg.panel_height = 320;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.readable = true;
      cfg.invert = false;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;
      panel_.config(cfg);
    }
    {
      auto cfg = light_.config();
      cfg.pin_bl = 21;
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;
      light_.config(cfg);
      panel_.setLight(&light_);
    }
    setPanel(&panel_);
  }
};

#else  // the real device: SSD1351 OLED

// From Panel_SSD1351::update_madctl's table (LovyanGFX 1.3.0): rotation 0 is the plain scan;
// rotation 6 is rotation 0 with only the column order reversed (remap bit 1), i.e. a free
// LEFT-RIGHT mirror for the reflector. If the image is upside down on the real module, use 4 / 2
// instead (the same pair with the row scan reversed). Confirm in Phase 1 (ADR 0010).
constexpr uint8_t kRotationNormal = 0, kRotationMirrored = 6;

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_SSD1351 panel_;
  lgfx::Bus_SPI bus_;

 public:
  LGFX() {
    {
      auto cfg = bus_.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 20000000;  // SSD1351 serial clock limit (50 ns cycle): ≈13 ms per 32 KB frame
      cfg.freq_read = 10000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = 12;
      cfg.pin_mosi = 11;
      cfg.pin_miso = -1;  // write-only panel; GPIO 13 stays free for an SD card later
      cfg.pin_dc = 9;
      bus_.config(cfg);
      panel_.setBus(&bus_);
    }
    {
      auto cfg = panel_.config();
      cfg.pin_cs = 10;
      cfg.pin_rst = 14;
      cfg.pin_busy = -1;
      cfg.panel_width = 128;
      cfg.panel_height = 128;
      cfg.memory_width = 128;
      cfg.memory_height = 128;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.readable = false;
      cfg.invert = false;
      cfg.rgb_order = false;  // if red and blue come out swapped on the real module, flip this
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;
      panel_.config(cfg);
    }
    setPanel(&panel_);  // no backlight: an OLED's brightness is its drive current (contrast)
  }
};

#endif
