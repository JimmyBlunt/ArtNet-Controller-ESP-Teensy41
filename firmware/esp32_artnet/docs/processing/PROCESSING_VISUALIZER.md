# Processing Visualizer

The Processing 4 app is in `processing/ArtNetMixedLedVisualizer/`.

Implemented:

- 3D point rendering sketch shell.
- Demo performance modes via keyboard-loaded point counts.
- Tested JavaScript reference module for preview packet encoding, decoding, fragmentation, and reassembly.
- Tested Direct Art-Net ArtDMX decoder and universe assembler.
- Tested mapping loader/normalizer for neutral JSON mappings.
- Tested statistics module for received/rendered FPS, lost frames, incomplete frames, and throughput.
- Tested playback JSON read/write and hot-reload polling core.
- Sketch UI now exposes Controller Preview, Direct Art-Net, Playback, Quality/Balanced/Performance labels, axes, bounding box, zoom, glow toggle, and demo pixel counts.
- Sketch opens UDP sockets for Controller Preview on port `6455` and Direct Art-Net on port `6454`.
- Runtime smoke test passed with Processing 3.5.4 CLI: local Preview UDP, local Direct Art-Net UDP, and ESP32 Controller Preview to Processing.

Planned:

- Playback controls in the `.pde` sketch.
- Hot reload of neutral mapping JSON in the `.pde` sketch UI.
- UI panels for FPS, loss, incomplete chunks, latency, output ranges, and universe/channel hover details.

Verification:

```sh
node tests/test_processing_protocol.js
node tests/test_processing_visualizer.js
NODE_PATH="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules" make test NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"
```
