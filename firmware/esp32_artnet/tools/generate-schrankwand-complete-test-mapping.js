#!/usr/bin/env node

const fs = require("fs");
const path = require("path");
const { normalize, validateMapping } = require("./mapping-utils");

const SIDE_INPUT = "Mappings/schrankwand -onlyu.txt";
const JSON_OUTPUT = "config/examples/schrankwand-complete-test-3739.json";
const OBJ_OUTPUT = "build/schrankwand-complete-test-3739.obj";
const ZONES_OUTPUT = "shader-library/metadata/schrankwand-complete-test-zones.csv";

// Test assumption from the cabinet/front photos:
// ten square panels arranged as 2 columns × 5 rows.
// They are mounted on the back wall of each cabinet compartment, not on the
// front/opening plane.
// LED order inside every panel:
// pixel 0 starts bottom-right, then runs serpentine/zigzag row-by-row upward.
const PANEL_COLS = Number(process.env.SCHRANK_PANEL_COLS || 2);
const PANEL_ROWS = Number(process.env.SCHRANK_PANEL_ROWS || 5);
const PANEL_SIZE = Number(process.env.SCHRANK_PANEL_SIZE || 16);
const PANEL_COUNT = PANEL_COLS * PANEL_ROWS;
const FRONT_MARGIN_X = Number(process.env.SCHRANK_FRONT_MARGIN_X || 0.04);
const FRONT_MARGIN_Y = Number(process.env.SCHRANK_FRONT_MARGIN_Y || 0.08);
const PANEL_BACK_WALL_Z_OFFSET = Number(process.env.SCHRANK_PANEL_BACK_WALL_Z_OFFSET || 0.02);
const PANEL_Z = process.env.SCHRANK_PANEL_Z === undefined ? null : Number(process.env.SCHRANK_PANEL_Z);
const SERPENTINE = (process.env.SCHRANK_PANEL_SERPENTINE || "1") !== "0";

function bounds(points) {
  const mins = [0, 1, 2].map(axis => Math.min(...points.map(point => point[axis] || 0)));
  const maxs = [0, 1, 2].map(axis => Math.max(...points.map(point => point[axis] || 0)));
  return { mins, maxs, spans: maxs.map((value, axis) => value - mins[axis]) };
}

function pointInPanel(panelCol, panelRow, localX, localY, b) {
  const usableX = b.spans[0] * (1.0 - FRONT_MARGIN_X * 2.0);
  const usableY = b.spans[1] * (1.0 - FRONT_MARGIN_Y * 2.0);
  const panelSizeWorld = Math.min(usableX / PANEL_COLS, usableY / PANEL_ROWS);
  const totalW = panelSizeWorld * PANEL_COLS;
  const totalH = panelSizeWorld * PANEL_ROWS;

  const x0 = b.mins[0] + (b.spans[0] - totalW) * 0.5 + panelCol * panelSizeWorld;
  const y0 = b.maxs[1] - (b.spans[1] - totalH) * 0.5 - panelRow * panelSizeWorld;

  const x = x0 + (localX / Math.max(1, PANEL_SIZE - 1)) * panelSizeWorld;
  const y = y0 - (localY / Math.max(1, PANEL_SIZE - 1)) * panelSizeWorld;
  const z = PANEL_Z === null ? b.maxs[2] - b.spans[2] * PANEL_BACK_WALL_Z_OFFSET : PANEL_Z;
  return [x, y, z];
}

function makeFrontPanels(b) {
  const points = [];
  const zones = [];
  let startPixel = 0;

  for (let panelIndex = 0; panelIndex < PANEL_COUNT; panelIndex++) {
    const panelCol = panelIndex % PANEL_COLS;
    const panelRow = Math.floor(panelIndex / PANEL_COLS);
    const zoneStart = startPixel;

    for (let rowFromBottom = 0; rowFromBottom < PANEL_SIZE; rowFromBottom++) {
      const y = PANEL_SIZE - 1 - rowFromBottom;
      for (let xRaw = 0; xRaw < PANEL_SIZE; xRaw++) {
        const x = SERPENTINE && rowFromBottom % 2 === 1 ? xRaw : PANEL_SIZE - 1 - xRaw;
        points.push(pointInPanel(panelCol, panelRow, x, y, b));
        startPixel++;
      }
    }

    zones.push({
      id: `front_panel_${String(panelIndex + 1).padStart(2, "0")}`,
      type: "apa102_panel_16x16",
      startPixel: zoneStart,
      pixelCount: PANEL_SIZE * PANEL_SIZE,
      panelCol,
      panelRow,
      mappingMode: SERPENTINE ? "16x16_serpentine_rows_start_bottom_right" : "16x16_linear_rows_start_bottom_right"
    });
  }

  return { points, zones };
}

function writeObj(points, outputPath, splitIndex) {
  const lines = [
    "# Generated complete Schrankwand test mapping.",
    "# front_panel vertices first, side_contour vertices second.",
    "g front_panels",
    ...points.slice(0, splitIndex).map(point => `v ${point[0]} ${point[1]} ${point[2]}`),
    "g side_contour",
    ...points.slice(splitIndex).map(point => `v ${point[0]} ${point[1]} ${point[2]}`)
  ];
  fs.mkdirSync(path.dirname(outputPath), { recursive: true });
  fs.writeFileSync(outputPath, `${lines.join("\n")}\n`);
}

function writeZones(zones, sideStart, sideCount, outputPath) {
  const rows = [
    "zone,type,startPixel,pixelCount,mappingMode,notes",
    ...zones.map(zone => [
      zone.id,
      zone.type,
      zone.startPixel,
      zone.pixelCount,
      zone.mappingMode,
      `front col ${zone.panelCol} row ${zone.panelRow}`
    ].join(",")),
    ["side_contour_existing", "existing_3d_points", sideStart, sideCount, "source_order", SIDE_INPUT].join(",")
  ];
  fs.mkdirSync(path.dirname(outputPath), { recursive: true });
  fs.writeFileSync(outputPath, `${rows.join("\n")}\n`);
}

function main() {
  const sideRaw = JSON.parse(fs.readFileSync(SIDE_INPUT, "utf8"));
  const b = bounds(sideRaw);
  const { points: frontRaw, zones } = makeFrontPanels(b);
  const combinedRaw = [...frontRaw, ...sideRaw];
  const combined = normalize(combinedRaw, "contain");

  const mapping = {
    id: "schrankwand-complete-test-3739",
    version: 1,
    pixelCount: combined.length,
    dimensions: 3,
    fit: "contain",
    source: {
      frontPanels: {
        generated: true,
        panelCount: PANEL_COUNT,
        panelCols: PANEL_COLS,
        panelRows: PANEL_ROWS,
        panelSize: PANEL_SIZE,
        serpentineRows: SERPENTINE,
        firstPixel: "bottom-right",
        rowDirection: "zigzag upward",
        panelAspect: "square",
        mountingPlane: "back-wall-of-each-compartment",
        zMode: PANEL_Z === null ? "side-bounds-max-z-minus-offset" : "explicit-env-SCHRANK_PANEL_Z"
      },
      sideContour: SIDE_INPUT
    },
    outputs: [
      {
        id: "front_panels_generated",
        startPixel: 0,
        pixelCount: frontRaw.length,
        type: "generated-front-panels"
      },
      {
        id: "side_contour_existing",
        startPixel: frontRaw.length,
        pixelCount: sideRaw.length,
        type: "existing-side-leds"
      }
    ],
    points: combined
  };

  const errors = validateMapping(mapping);
  if (errors.length) throw new Error(errors.join("\n"));

  fs.mkdirSync(path.dirname(JSON_OUTPUT), { recursive: true });
  fs.writeFileSync(JSON_OUTPUT, `${JSON.stringify(mapping, null, 2)}\n`);
  writeObj(combined, OBJ_OUTPUT, frontRaw.length);
  writeZones(zones, frontRaw.length, sideRaw.length, ZONES_OUTPUT);

  console.log(`Wrote ${mapping.pixelCount} total pixels`);
  console.log(`Front panels: ${frontRaw.length} (${PANEL_COLS}x${PANEL_ROWS} panels, ${PANEL_SIZE}x${PANEL_SIZE})`);
  console.log(`Existing side contour: ${sideRaw.length}`);
  console.log(`JSON: ${JSON_OUTPUT}`);
  console.log(`OBJ: ${OBJ_OUTPUT}`);
  console.log(`Zones: ${ZONES_OUTPUT}`);
}

main();
