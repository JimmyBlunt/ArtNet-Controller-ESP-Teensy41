# Schrank LED Mixed Art-Net Controller

Mixed WS2812B/APA102 Art-Net controller firmware and tools for the Schrank LED setup.

The currently verified ESP32 target is:

- ESP32 at `10.0.0.251`
- WS2812B output 0: GPIO23, GRB, default 512 LEDs
- APA102 output 1: GPIO18 data / GPIO19 clock, BGR, default 1167 LEDs
- Total default layout: 1679 pixels / 10 Art-Net universes

## Documentation

- [Shared controller UI, blackout, persistence and receive behavior](docs/CONTROLLER_REFRESH.md)
- [Code map](docs/CODE_MAP.md)
- [ESP32 web menu and runtime configuration](docs/ESP32_WEB_MENU_RUNTIME_CONFIG.md)
- [30-pin extension board pins and power](docs/EXTENSION_BOARD_PROFILE.md)
- [Build and test checks](docs/BUILD_AND_TEST_CHECKS.md)
- [Pixelblaze and MariMapper compatibility](docs/mapping/PIXELBLAZE_MARIMAPPER_COMPATIBILITY.md)
- [System architecture](docs/architecture/SYSTEM_ARCHITECTURE.md)
- [Test plan](docs/testing/TEST_PLAN.md)
- [Processing visualizer](docs/processing/PROCESSING_VISUALIZER.md)

## ESP32 Web Menu

The firmware serves a built-in web menu:

- `http://10.0.0.251/`
- `http://10.0.0.251/live`

The local desktop page and embedded ESP menu now share the same blue/violet interface.
It loads the actual controller outputs, supports individual-output tests and held
blackout, and distinguishes applying RAM configuration from explicitly saving to NVS
for restoration after reboot. Auto Layout updates downstream pixel ranges and universes.
See the controller refresh document for API behavior and verification limits.

## Full Build Check

In this Codex desktop environment:

```sh
NODE_PATH="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules" \
make check NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"
```

This runs native firmware tests, JS tests, Playwright UI checks and the PlatformIO build for `esp32-wifi-ws2812-apa102-1679`.

## Mapping Compatibility

Pixelblaze coordinate arrays and MariMapper CSV files can be converted with `tools/mapping-convert.js`. The conversion stays PC-side so the ESP32 realtime Art-Net and LED output path does not need to parse large mapping files while frames are running.

## Screenshots

Regenerate documentation screenshots:

```sh
NODE_PATH="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules" \
"/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe" \
tools/capture-doc-screenshots.js
```

## Hardware Notes

Some results can only be marked complete after real visual confirmation:

- LED color output on the physical chains.
- High-brightness power stability.
- TouchDesigner/MadMapper/Processing mapping accuracy.
- Long-duration Art-Net stability.

Keep those observations in `STATUS.md` with exact dates and what was actually seen.
