# Parts to buy (Phase 1)

I checked these prices and links on **2026-11-14**. They change, so confirm at checkout. Do the [cardboard optics test](ROADMAP.md#phase-1-ghost-tank-v1-about-45-55) before ordering. It's free and confirms the screen size.

| # | Part | Exact item | Price | Where |
|---|---|---|---|---|
| 1 | Microcontroller | **ESP32-S3-DevKitC-1-N8R8** (8 MB flash, **8 MB PSRAM**: the "R8" matters, since the backdrop buffers live in PSRAM) | ~$15–20 | [Adafruit #5336](https://www.adafruit.com/product/5336) ($19.95, ships fast) · [DigiKey](https://www.digikey.com/en/products/detail/espressif-systems/ESP32-S3-DEVKITC-1-N8R8/15295894) (~$15, can have a lead time) |
| 2 | Display | **2.8" 320×240 ILI9341 SPI TFT**, with or without touch. Touch is harmless but unused: the screen sits face-up **inside** the base, so v2's feed control is a physical button on the front | ~$11–14 | [ProtoSupplies](https://protosupplies.com/product/tft-lcd-2-8-240x320-rgb-spi-display-with-touchscreen/) ($13.95) · [ShillehTek](https://shillehtek.com/products/2-8-spi-tft-lcd-touch-screen-module-240x320-ili9341-for-arduino) ($10.99) |
| 3 | Reflector | **Clear PETG sheet, 0.030" (0.76 mm) or 0.060"**, 12×12". Thinner gives less double image; one sheet makes several reflectors | ~$5 | [eStreet Plastics 0.030"](https://estreetplastics.com/030-x-12-x-12-Clear-PETG-p/p100301212.htm) · [0.060"](https://estreetplastics.com/060-x-12-x-12-Clear-PETG-p/p100601212.htm). Free alternative: glass from a dollar-store picture frame (thicker, more doubling) |
| 4 | Prototyping | Breadboard, male–female jumper wires, USB-C data cable | ~$10 | Any electronics store |
| 5 | Screws | M3 × 6 self-tapping (×4), M3 × 10 (×2) | ~$3 | Hardware store |
| 6 | *(v2, optional)* Feed button | 12–16 mm momentary pushbutton, or a small arcade button | ~$1–5 | Any electronics store |

**Total: about $45–55.** The enclosure prints free at the EXP Makerspace ([`hardware/`](../hardware/README.md)).

## Before you order
- **N8R8, not N8.** The plain N8 has no PSRAM. The firmware would fall back to slower rendering on black, without the glow.
- **Measure the display when it arrives.** Clone modules vary by a millimetre or two. Put your caliper numbers into `hardware/ghost_base.py` (`DISP_*`) and run `make cad` before printing.
- **Check the pin labels.** Some ILI9341 boards label DC as "RS" or "A0", and LED as "BL". The wiring is in [DESIGN §9](DESIGN.md#pin-map-esp32-s3).

## Sources (checked 2026-11-14)
[Adafruit 5336](https://www.adafruit.com/product/5336) · [DigiKey](https://www.digikey.com/en/products/detail/espressif-systems/ESP32-S3-DEVKITC-1-N8R8/15295894) · [TME](https://www.tme.eu/en/details/esp32s3devkc-8r8/development-kits-for-data-transmission/espressif/esp32-s3-devkitc-1-n8r8/) · [ProtoSupplies](https://protosupplies.com/product/tft-lcd-2-8-240x320-rgb-spi-display-with-touchscreen/) · [ShillehTek](https://shillehtek.com/products/2-8-spi-tft-lcd-touch-screen-module-240x320-ili9341-for-arduino) · [eStreet Plastics](https://estreetplastics.com/030-x-12-x-12-Clear-PETG-p/p100301212.htm)
