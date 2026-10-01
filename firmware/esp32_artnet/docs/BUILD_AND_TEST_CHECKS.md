# Build-Einstieg im zusammengeführten Repository

Stand 01.10.2026: Aktuelle Befehle stehen in [../README.md](../README.md).
`build.ps1` nutzt kurze isolierte Cachepfade; `python tools/test_host.py` prüft
Hostcode ohne Gerätezugriff. Mit `--include-historical` bleibt der bekannte
ESP251-Profilfehler sichtbar. Die folgenden Pfade, Profile und COM6-Angaben
stammen aus der ursprünglichen Projektchronik und sind keine aktuelle Uploadvorgabe.

---

# Build and Test Checks

Run these checks before changing firmware, web UI, mapping, Art-Net sender logic or Processing preview code.

## Recommended Full Check

In the Codex desktop environment:

```sh
NODE_PATH="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules" \
make check NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"
```

`make check` runs:

1. Native C++ firmware core tests.
2. Mapping schema and Pixelblaze/MariMapper compatibility tests.
3. Browser calculation tests.
4. Processing protocol tests.
5. Processing visualizer logic tests.
6. Hardware benchmark tool tests.
7. ESP web menu Playwright tests that operate the UI like a user.
8. Local browser UI Playwright test.
9. PlatformIO build for `esp32-wifi-ws2812-apa102-1679`.

## Fast Local Firmware Test

```sh
make test NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"
```

## ESP32 Firmware Build Only

```sh
.venv-platformio/Scripts/pio.exe run -e esp32-wifi-ws2812-apa102-1679
```

## ESP32 Flash and Monitor

Always identify the serial port before flashing:

```sh
.venv-platformio/Scripts/pio.exe pkg exec -p tool-esptoolpy -- \
  esptool.py --chip auto --port COM6 --baud 115200 chip_id
```

Known good ESP32:

- Port: `COM6`.
- Chip: `ESP32-D0WD`.
- MAC: `e8:68:e7:0d:38:a4`.

Flash:

```sh
.venv-platformio/Scripts/pio.exe run \
  -e esp32-wifi-ws2812-apa102-1679 \
  -t upload --upload-port COM6
```

Monitor:

```sh
.venv-platformio/Scripts/pio.exe device monitor \
  -e esp32-wifi-ws2812-apa102-1679 \
  --port COM6 --baud 115200
```

Serial parameters:

- `115200`
- `8-N-1`
- no flow control

## Screenshot Documentation

Capture current documentation screenshots:

```sh
NODE_PATH="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules" \
"/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe" \
tools/capture-doc-screenshots.js
```

The script writes:

- `docs/assets/screenshots/esp-web-menu-runtime-config.png`
- `docs/assets/screenshots/desktop-web-ui-overview.png`

## What Is Not Fully Automated

Some checks still require real hardware or visual confirmation:

- Actual LED color visibility.
- Power stability at high brightness.
- TouchDesigner mapping correctness on the physical installation.
- Long-duration Art-Net stability.
- Visual correctness of imported Pixelblaze/MariMapper layouts on the real installation.

Document those results in `STATUS.md` with what was actually observed.
