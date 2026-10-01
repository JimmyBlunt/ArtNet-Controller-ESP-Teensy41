# Test Plan

Automated local tests:

- Firmware core: `build/test_firmware_core`
- Mapping validator/importer: `tests/test_mapping_schema.js`
- Web performance calculations: `tests/test_web_calculations.js`
- Processing preview protocol: `tests/test_processing_protocol.js`
- Web UI browser flow: `tests/test_web_ui_playwright.js`

Run in this environment with:

```sh
NODE_PATH="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules" make test benchmark NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"
```

Hardware-in-the-loop tests, not complete:

- APA102 physical output at 2, 4, 6, 8, and 12 MHz.
- WS2812B parallel outputs at 30, 40, 50, and 60 FPS.
- Ethernet under Art-Net load.
- 60 FPS long-run soak.
- Signal-loss modes on real LEDs.
