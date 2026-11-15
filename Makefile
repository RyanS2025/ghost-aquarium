# Ghost Aquarium: one command per workflow. Run `make` (or `make help`) to see them all.
# Works from any shell; uses ~/.local/bin/pio if `pio` isn't on PATH.

PIO := $(shell command -v pio 2>/dev/null || echo $(HOME)/.local/bin/pio)
WOKWI := $(shell command -v wokwi-cli 2>/dev/null || echo $(HOME)/.wokwi/bin/wokwi-cli)
OUT := .pio/out
PORT ?= 8765

.DEFAULT_GOAL := help
.PHONY: help setup test test-ready build build-sim sim snapshot gif cad prototype flash monitor check clean

help: ## Show this list
	@grep -E '^[a-z-]+:.*## ' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*## "}; {printf "  make %-12s %s\n", $$1, $$2}'

setup: ## Install PlatformIO (via pipx) and show how to enable the Wokwi simulator
	@command -v pio >/dev/null 2>&1 || test -x $(HOME)/.local/bin/pio || pipx install platformio
	@echo "PlatformIO: $$($(PIO) --version)"
	@if test -x "$(WOKWI)"; then echo "wokwi-cli: installed"; else \
	  echo "wokwi-cli: not installed → curl -L https://wokwi.com/ci/install.sh | sh"; fi
	@if test -n "$$WOKWI_CLI_TOKEN"; then echo "WOKWI_CLI_TOKEN: set"; else \
	  echo "WOKWI_CLI_TOKEN: not set → get a free token at https://wokwi.com/dashboard/ci, then add"; \
	  echo "  export WOKWI_CLI_TOKEN=...  to ~/.zshrc (and as a GitHub secret for CI)"; fi

test: ## Run every laptop test (includes your schooling task)
	$(PIO) test -e native

test-ready: ## Run every laptop test except the schooling task (Task 4)
	$(PIO) test -e native -i test_school

build: ## Build the device firmware (SSD1351 OLED, mirrored for the reflector)
	$(PIO) run -e esp32s3

build-sim: ## Build the simulator firmware: ILI9341 panel, un-mirrored, 128×128 tank centred
	PLATFORMIO_BUILD_FLAGS="-DGHOST_MIRROR=0 -DGHOST_PANEL_SIM=1" $(PIO) run -e esp32s3

sim: build-sim ## Boot the firmware in Wokwi headlessly: checks for an fps= line, saves log + screenshot
	@test -x "$(WOKWI)" || { echo "wokwi-cli missing: run 'make setup' for instructions"; exit 1; }
	@test -n "$$WOKWI_CLI_TOKEN" || { echo "WOKWI_CLI_TOKEN not set: run 'make setup' for instructions"; exit 1; }
	@mkdir -p $(OUT)
	@# Run a fixed 12 s (no --expect-text: it stops the sim at the first match, before the
	@# screenshot), screenshot at 11 s, then require an fps= line in the log ourselves.
	"$(WOKWI)" . --timeout 12000 --timeout-exit-code 0 --serial-log-file $(OUT)/wokwi-serial.log \
	  --screenshot-part lcd --screenshot-time 11000 --screenshot-file $(OUT)/wokwi-screen.png
	@grep -q "fps=" $(OUT)/wokwi-serial.log || { echo "FAIL: no fps= line in the serial log"; exit 1; }
	@echo "serial log: $(OUT)/wokwi-serial.log"; grep "ghost-aquarium up\|fps=" $(OUT)/wokwi-serial.log
	@echo "screenshot: $(OUT)/wokwi-screen.png"

snapshot: ## Render one frame with the C++ renderer → .pio/out/frame.png (and open it)
	@mkdir -p $(OUT)
	GHOST_SNAPSHOT=$(CURDIR)/$(OUT)/frame.ppm $(PIO) test -e native -f test_render
	@sips -s format png $(OUT)/frame.ppm --out $(OUT)/frame.png >/dev/null 2>&1 || \
	  convert $(OUT)/frame.ppm $(OUT)/frame.png
	@echo "frame: $(OUT)/frame.png"
	@open $(OUT)/frame.png 2>/dev/null || xdg-open $(OUT)/frame.png 2>/dev/null || true

gif: ## Render the README demo GIF with the firmware's renderer → docs/media/demo.gif (needs ffmpeg)
	@rm -rf $(OUT)/frames && mkdir -p $(OUT)/frames
	GHOST_FRAMES=$(CURDIR)/$(OUT)/frames $(PIO) test -e native -f test_render
	ffmpeg -loglevel error -y -framerate 20 -i $(OUT)/frames/frame_%03d.ppm \
	  -vf "scale=512:512:flags=neighbor,split[a][b];[a]palettegen=max_colors=128:stats_mode=diff[p];[b][p]paletteuse=dither=none:diff_mode=rectangle" \
	  -loop 0 docs/media/demo.gif
	@ls -lh docs/media/demo.gif

CAD_PY := .pio/cad-venv/bin/python
cad: ## Regenerate the 3D-printable enclosure (hardware/out/*.stl + cross-section preview)
	@test -x $(CAD_PY) || { python3 -m venv .pio/cad-venv && .pio/cad-venv/bin/pip install -q -r hardware/requirements.txt; }
	$(CAD_PY) hardware/ghost_base.py

prototype: ## Serve the browser tuner on http://127.0.0.1:8765 and open it (Ctrl+C to stop)
	@(sleep 1; open http://127.0.0.1:$(PORT)/index.html 2>/dev/null || xdg-open http://127.0.0.1:$(PORT)/index.html) &
	cd prototype && python3 -m http.server $(PORT) --bind 127.0.0.1

flash: ## Build and upload to a connected ESP32-S3 over USB
	$(PIO) run -e esp32s3 -t upload

monitor: ## Open the serial monitor (fps / render / push stats every 5 s)
	$(PIO) device monitor -e esp32s3

check: test-ready build snapshot ## Everything that runs without hardware or a Wokwi token

clean: ## Remove build outputs
	$(PIO) run -t clean -e esp32s3
	rm -rf $(OUT)
