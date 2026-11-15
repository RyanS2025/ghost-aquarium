# Parts to buy (Phase 1)

I checked these prices and links on **2026-11-14** (display row: **2026-11-15**). They change, so confirm at checkout. Do the [cardboard optics test](ROADMAP.md#phase-1-ghost-tank-v1-about-50-75) before ordering. It's free and shows you how big the ghost will be.

| # | Part | Exact item | Price | Where |
|---|---|---|---|---|
| 1 | Microcontroller | **ESP32-S3-DevKitC-1-N8R8** (8 MB flash, **8 MB PSRAM**: the "R8" matters, since the backdrop buffers live in PSRAM) | ~$15–20 | [Adafruit #5336](https://www.adafruit.com/product/5336) ($19.95, ships fast) · [DigiKey](https://www.digikey.com/en/products/detail/espressif-systems/ESP32-S3-DEVKITC-1-N8R8/15295894) (~$15, can have a lead time) |
| 2 | Display | **1.5" 128×128 SSD1351 16-bit color OLED, 4-wire SPI** ([ADR 0010](adr/0010-display-ssd1351-oled.md)). Make sure the listing says **SSD1351** and **RGB/color**: 1.3" and 0.96" OLEDs are usually single-color SSD1306/SH1106 | ~$17–40 | [BuyDisplay ER-OLED015-1C](https://www.buydisplay.com/serial-spi-1-5-inch-color-oled-display-128x128-graphic-module-ssd1351) ($20.25) · [DisplayModule](https://www.displaymodule.com/products/1-5-inch-oled-graphic-display-128x128-rgb-color-with-spi) ($24.90) · [Waveshare-style on Amazon](https://www.amazon.com/1-5inch-RGB-OLED-Module-Interface/dp/B07V579YK2) (~$23–28) · [Adafruit #1431](https://www.adafruit.com/product/1431) ($39.95, **out of stock** on 2026-11-15) |
| 3 | Reflector | **Clear PETG sheet, 0.030" (0.76 mm) or 0.060"**, 12×12". Thinner gives less double image; one sheet makes several reflectors | ~$5 | [eStreet Plastics 0.030"](https://estreetplastics.com/030-x-12-x-12-Clear-PETG-p/p100301212.htm) · [0.060"](https://estreetplastics.com/060-x-12-x-12-Clear-PETG-p/p100601212.htm). Free alternative: glass from a dollar-store picture frame (thicker, more doubling) |
| 4 | Prototyping | Breadboard, male–female jumper wires, USB-C data cable | ~$10 | Any electronics store |
| 5 | Screws | M3 × 6 self-tapping (×4), M3 × 10 (×2) | ~$3 | Hardware store |
| 6 | *(v2, optional)* Feed button | 12–16 mm momentary pushbutton, or a small arcade button | ~$1–5 | Any electronics store |

**Total: about $50–75**, depending on where the display comes from. The enclosure prints free at the EXP Makerspace ([`hardware/`](../hardware/README.md)).

## Before you order
- **N8R8, not N8.** The plain N8 has no PSRAM. The firmware would fall back to slower rendering on black, without the glow.
- **Measure the display when it arrives.** Clone modules vary by a millimetre or two. Put your caliper numbers into `hardware/ghost_base.py` (`DISP_*`) and run `make cad` before printing.
- **Check the pin labels.** OLED modules often label MOSI as "DIN" or "SI", SCLK as "CLK" or "CL", and DC as "D/C" or "RS". The OLED has no backlight or MISO pin. The wiring is in [DESIGN §9](DESIGN.md#pin-map-esp32-s3).
- **Pick the SPI version.** Some modules ship set to I2C or 3-wire SPI with a solder jumper or resistor. Use 4-wire SPI.

## Sources (checked 2026-11-14; display 2026-11-15)
[Adafruit 5336](https://www.adafruit.com/product/5336) · [DigiKey](https://www.digikey.com/en/products/detail/espressif-systems/ESP32-S3-DEVKITC-1-N8R8/15295894) · [TME](https://www.tme.eu/en/details/esp32s3devkc-8r8/development-kits-for-data-transmission/espressif/esp32-s3-devkitc-1-n8r8/) · [eStreet Plastics](https://estreetplastics.com/030-x-12-x-12-Clear-PETG-p/p100301212.htm) · [BuyDisplay](https://www.buydisplay.com/serial-spi-1-5-inch-color-oled-display-128x128-graphic-module-ssd1351) · [DisplayModule](https://www.displaymodule.com/products/1-5-inch-oled-graphic-display-128x128-rgb-color-with-spi) · [Amazon](https://www.amazon.com/1-5inch-RGB-OLED-Module-Interface/dp/B07V579YK2) · [Adafruit 1431](https://www.adafruit.com/product/1431)
