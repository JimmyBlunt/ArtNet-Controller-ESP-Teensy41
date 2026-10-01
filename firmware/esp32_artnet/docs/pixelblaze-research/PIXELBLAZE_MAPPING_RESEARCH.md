# Pixelblaze Mapping Research

Date: 2026-08-03

Scope was read-only against `http://10.0.0.87`. The index page was fetched with `curl --compressed` and saved to `reports/pixelblaze-index.html`; no save, delete, upload, firmware, or configuration-changing request was sent.

## Observed Interface

- The Pixelblaze UI is a single large HTML document with embedded JavaScript.
- The browser connects to `ws://<host>:81/`.
- Text WebSocket frames carry JSON commands such as configuration reads, status updates, program lists, active program changes, and source requests.
- Binary WebSocket packets are typed. The observed `PacketType` table includes `PREVIEWFRAME = 5`, `SOURCESDATA = 6`, `PROGRAMLIST = 7`, `PIXELMAP = 8`, and `OUTPUTBOARDCONFIG = 9`.
- The JavaScript names `mapperFit`, `Pixel Map Functions`, `mapPixels(index, x, y, z)`, `has2DMap`, and `has3DMap` are present in the served UI.

## Mapping Behavior Inferred From Public UI

- Mapping coordinates are used by Pixelblaze render functions through `mapPixels(fn)`.
- If no map is installed, `x` is equivalent to `index / pixelCount`, while `y` and `z` are zero.
- 2D maps provide `x` and `y`; 3D maps provide `x`, `y`, and `z`.
- Coordinate transforms are applied at render time before the callback receives coordinates.
- The browser UI has Mapper examples for ring, matrix, volumetric cube, and walled cube based on the local screenshots in `screenshots-pixlebaze/`.
- The UI contains Fill/Contain controls through `mapperFit`.

## Save/Storage Finding

The served JavaScript proves that the browser can send binary blobs and JSON commands to the device over WebSocket. Because this run was intentionally read-only, I did not observe a live save transaction. The current compatibility layer therefore treats Pixelblaze import as an offline conversion into neutral coordinates, not as a claim of full device write compatibility.

## Security Boundary For This Project

- Pixelblaze-like scripts may be imported only in an offline tool or sandbox process.
- The ESP32 firmware must not execute arbitrary JavaScript.
- Firmware consumes validated neutral JSON coordinates and index maps.
