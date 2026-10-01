# Pixelblaze and MariMapper Compatibility

This project supports Pixelblaze and MariMapper mapping files as offline conversion formats. The ESP32 keeps the realtime path lightweight: it receives Art-Net pixel data and drives the LED outputs, while larger mapping files are parsed and converted on the PC.

## Supported Imports

### Pixelblaze

Supported:

- JSON coordinate arrays such as `[[0,0,0],[1,0,0]]`.
- JSON with JavaScript-style `//` or `/* */` comments.
- JSON arrays with trailing commas.
- Pixelblaze JavaScript mapper generators are detected and preserved as non-executable source metadata.

Not supported on the ESP32:

- Executing Pixelblaze JavaScript.
- Compiling Pixelblaze `render`, `render2D`, `render3D`, or `mapPixels()` code on the controller.

That boundary is intentional. Running script parsing or generation on the ESP32 would add heap pressure and could disturb Art-Net output timing.

### MariMapper

Supported CSV columns:

- `index,x,y,z`
- `index,tx,ty,tz`
- `index,x,y` or `index,tx,ty` for 2D maps

Extra CSV columns are preserved as per-row metadata in the neutral JSON output. Missing index slots are not compressed; they are filled with `[0,0,0]` placeholders and recorded in `compatibility.missingIndices`.

## Conversion Commands

Import a Pixelblaze coordinate array into neutral JSON:

```sh
node tools/mapping-convert.js --format pixelblaze --input pixelblaze-map.json --output config/examples/pixelblaze-import.json --id pixelblaze-import --fit contain --dimensions 3
```

Import a MariMapper CSV into neutral JSON:

```sh
node tools/mapping-convert.js --format marimapper --input marimapper.csv --output config/examples/marimapper-import.json --id marimapper-import --fit none --dimensions 3
```

Export neutral JSON back to Pixelblaze coordinates:

```sh
node tools/mapping-convert.js --format neutral --input config/examples/marimapper-import.json --export pixelblaze --output build/marimapper.pixelblaze.json --id marimapper-import --fit none
```

Export neutral JSON to MariMapper CSV:

```sh
node tools/mapping-convert.js --format neutral --input config/examples/pixelblaze-import.json --export marimapper --output build/pixelblaze.marimapper.csv --id pixelblaze-import --fit none
```

## Runtime Recommendation

For TouchDesigner, MadMapper, Pixelblaze-style layouts, or MariMapper-generated coordinates:

1. Convert or validate the layout on the PC with `tools/mapping-convert.js`.
2. Use the resulting mapping in TouchDesigner or the local visualizer to generate the final Art-Net pixel stream.
3. Send Art-Net to the ESP32.
4. Let the ESP32 focus on packet assembly and physical output timing.

The firmware `/api/mappings` endpoint can store mapping JSON for UI/metadata workflows, but this compatibility layer does not make the ESP32 parse and apply large coordinate maps during realtime output.
