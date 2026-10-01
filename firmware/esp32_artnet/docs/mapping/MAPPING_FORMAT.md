# Mapping Format

Mappings are neutral JSON files consumed by firmware, web preview, tools, and the Processing visualizer.

Required fields:

- `id`: stable mapping identifier.
- `version`: currently `1`.
- `pixelCount`: number of logical pixels.
- `dimensions`: `1`, `2`, or `3`.
- `points`: coordinate list. Length must equal `pixelCount`.

Optional fields:

- `fit`: `none`, `fill`, or `contain`.
- `source`: import/source hint.
- `outputs`: physical output ranges for visualization.

Validation rejects zero pixel counts, missing dimensions, NaN, Infinity, and mismatched point counts. The firmware receives validated coordinates or index maps only; script execution stays outside the controller.

Examples live in `config/examples/`.

## Compatibility Imports

The toolchain can import and export external mapping formats without adding realtime parser load to the ESP32:

- Pixelblaze coordinate arrays, including comments and trailing commas.
- Pixelblaze JavaScript mapper generators as preserved, non-executable source records.
- MariMapper CSV with `index,x,y,z` or `index,tx,ty,tz`.
- MariMapper missing indices as explicit `[0,0,0]` placeholders.

See [PIXELBLAZE_MARIMAPPER_COMPATIBILITY.md](PIXELBLAZE_MARIMAPPER_COMPATIBILITY.md) for commands and runtime limits.
