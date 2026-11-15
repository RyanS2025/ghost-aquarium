# v2 spec: interaction (feed, living tank, phone control, real-world link)

- **Status:** Draft. Built after v1 ships (ROADMAP Phase 2).
- **Date:** 2026-11-14
- **Builds on:** [DESIGN](../DESIGN.md), [ADR 0008](../adr/0008-device-render-pipeline-and-budgets.md) (the banded renderer freed 88 KB of heap, enough for WiFi), [ADR 0009](../adr/0009-motion-model.md)

Decisions marked **[call]** are defaults chosen to keep v2 moving. Each is easy to revisit.

## Architecture rule (unchanged from v1)

`lib/tanksim` stays hardware-free and deterministic. v2 adds **inputs** (`Tank::feed()`, `Tank::setEnvironment()`) and **state I/O** (`serialize` / `deserialize` to a byte buffer). Only `src/` touches the button, WiFi, HTTP, NVS and the clock. Every v2 behavior is unit-tested on the laptop, like v1.

## 1. Feed button

| | |
|---|---|
| Input | Momentary button on the base's front wall, GPIO 4 (internal pull-up, DESIGN §9), 25 ms debounce. Also `POST /api/feed`. **[call]** The display's touch panel can't be used: the screen sits inside the base |
| Effect | 6–10 **pellets** (`.`, warm color) appear at the surface at random x, sink at ~12 px/s with a slight sway, and settle on the sand. Uneaten ones fade after 20 s |
| Fish | A fish within 90 px of a pellet adds a **seek** force toward the nearest one, weighted by its hunger. On contact the pellet is eaten: removed, a small bubble burst, `hunger = 0` |
| Limits | Pellet pool of 16 (dropped when full). A press is ignored while 8 or more pellets are still sinking (no spamming) |
| Tests | pellets sink and settle; a hungry fish reaches and eats one; a full fish ignores food; no pool overflow; same seed + same press time gives the same result |

## 2. Living tank

| | |
|---|---|
| Hunger | Per fish, 0 → 1 over ~6 h of real time. **Hungry fish** (> 0.6) hold a shallower depth and swim 15% faster. **Starving fish** never die **[call]**: they just look listless (slower, dimmer). The piece stays pleasant if you forget it for a week |
| Growth | Each feeding adds to `fedCount`. At 12 / 30 feedings a fish moves up a species: `><>` (Far) → `><('>` (Mid) → `><((('>` (Near), which also moves it to a nearer depth layer. Capped by the per-layer counts, so the tank never fills with giants |
| Names | Optional, up to 12 characters, set from the phone page. Shown nowhere on the glass **[call]** (only on the phone), so the illusion stays pure |
| Persistence | `serialize()` → versioned byte blob (magic, version, CRC32). Saved to **NVS** on every feed and every 10 min (~52k writes/year; NVS spreads wear across the partition). Restored at boot; a bad CRC or version starts a fresh tank |
| Tests | the hunger curve; behavior thresholds; growth limits; serialize → deserialize gives an identical tank; a corrupted blob is rejected |

## 3. Phone control

| | |
|---|---|
| Joining WiFi | First boot (or the button held 5 s): the device opens an access point `GhostAquarium-XXXX` with a setup page for WiFi credentials, saved in NVS. **No credentials in the repo or the firmware image** |
| Page | One small page served from flash (gzipped). Buttons: feed, mirror on/off, brightness, night mode. Fish list with names, hunger and size. Location for the weather. Reached at **`ghost-aquarium.local`** (mDNS) |
| API | `GET /api/state` (JSON), `POST /api/feed`, `POST /api/settings`, `POST /api/fish/{id}/name` |
| Security | Local network only, with no cloud. Writes need a 4–6 digit **PIN** shown in the serial log at first setup **[call]**. Inputs are length-limited and validated; names are escaped on output |
| Budget | WiFi + server about 60–80 KB heap; ~260 KB is free with DMA bands. Networking runs on CPU0, rendering on CPU1, so the frame loop never blocks on the network |

## 4. Real-world link

| | |
|---|---|
| Source | [Open-Meteo](https://open-meteo.com/) forecast API: free, no API key. Polled every 30 min, using the latitude/longitude set on the phone page (no IP geolocation **[call]**) |
| Night | From `is_day` / sunrise / sunset: backlight dims to 25%, the water glow darkens, rays fade out, plankton twinkles more (bioluminescence). Fades over 10 min, never in a jump |
| Rain | `precipitation > 0.2 mm/h`: more bubbles, raindrop ripples across the surface, a slightly darker tint |
| Offline | Without a network or on errors, keep the last known state for 3 h, then fall back to a sunrise/sunset schedule from the stored location |
| Tests | `Environment` → tank parameters (pure function); fade timing; offline fallback |

## Order of work

1. Feed button + pellets (sim + firmware button). Smallest, and the most visible.
2. Living tank + persistence (sim serialize/deserialize, NVS in firmware).
3. WiFi setup + phone page + API.
4. Weather and night link.

Each gets its own plan in `docs/plans/` before code, as Phase 0 did.

## Open questions for later
- Should a fish ever leave for good, or should the population change over weeks? (Default: no.)
- Sound? (Out of scope; a small piezo for a feeding "plip" would be cheap.)
