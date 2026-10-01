#!/usr/bin/env node
const fs = require("fs");

function apa102FrameTimeUs(pixels, spiHz) {
  return ((32 + pixels * 32 + Math.ceil(pixels / 2)) / spiHz) * 1000000;
}

function ws2812FrameTimeUs(pixels) {
  return pixels * 30 + 300;
}

function artNetDataRateMbps(pixels, fps) {
  const universes = Math.ceil(pixels / 170);
  return ((18 + 512) * universes * fps * 8) / 1000000;
}

function row(kind, layout, pixelsPerLongestOutput, targetFps, spiHz, totalPixels = pixelsPerLongestOutput) {
  const frameTimeUs = kind === "APA102"
    ? apa102FrameTimeUs(pixelsPerLongestOutput, spiHz)
    : ws2812FrameTimeUs(pixelsPerLongestOutput);
  return {
    kind,
    layout,
    totalPixels,
    apa102Pixels: kind === "APA102" ? totalPixels : 0,
    ws2812bPixels: kind === "WS2812B" ? totalPixels : 0,
    pixelsPerLongestOutput,
    spiHz: spiHz || null,
    targetFps,
    theoreticalFrameTimeUs: Number(frameTimeUs.toFixed(2)),
    theoreticalMaxFps: Number((1000000 / frameTimeUs).toFixed(2)),
    artNetDataRateMbps: Number(artNetDataRateMbps(totalPixels, targetFps).toFixed(3)),
    warning: (1000000 / frameTimeUs) < targetFps,
    measured: false
  };
}

function mixedRow(layout, apaPixels, wsPixels, targetFps, spiHz = 4000000) {
  const apaFrameUs = apa102FrameTimeUs(apaPixels, spiHz);
  const wsFrameUs = ws2812FrameTimeUs(wsPixels);
  const frameTimeUs = Math.max(apaFrameUs, wsFrameUs);
  const totalPixels = apaPixels + wsPixels;
  return {
    kind: "Mixed",
    layout,
    totalPixels,
    apa102Pixels: apaPixels,
    ws2812bPixels: wsPixels,
    spiHz,
    targetFps,
    theoreticalFrameTimeUs: Number(frameTimeUs.toFixed(2)),
    apa102FrameTimeUs: Number(apaFrameUs.toFixed(2)),
    ws2812bFrameTimeUs: Number(wsFrameUs.toFixed(2)),
    theoreticalMaxFps: Number((1000000 / frameTimeUs).toFixed(2)),
    artNetDataRateMbps: Number(artNetDataRateMbps(totalPixels, targetFps).toFixed(3)),
    warning: (1000000 / frameTimeUs) < targetFps,
    measured: false
  };
}

const rows = [];
for (const spiHz of [2000000, 4000000, 6000000, 8000000, 12000000]) {
  rows.push(row("APA102", "3x500", 500, 60, spiHz, 1500));
  rows.push(row("APA102", "5x300", 300, 60, spiHz, 1500));
  rows.push(row("APA102", "2x750", 750, 60, spiHz, 1500));
  rows.push(row("APA102", "1x1500 comparison", 1500, 60, spiHz));
}
for (const fps of [30, 40, 50, 60]) {
  rows.push(row("WS2812B", "4x500=2000", 500, fps, null, 2000));
  rows.push(row("WS2812B", "5x400=2000", 400, fps, null, 2000));
  rows.push(row("WS2812B", "6x500=3000", 500, fps, null, 3000));
  rows.push(row("WS2812B", "8x375=3000", 375, fps, null, 3000));
  rows.push(row("WS2812B", "4x750=3000 comparison", 750, fps, null, 3000));
  rows.push(mixedRow("1500 APA102 + 2000 WS2812B", 1500, 2000, fps));
  rows.push(mixedRow("1500 APA102 + 3000 WS2812B", 1500, 3000, fps));
  rows.push(mixedRow("2000 APA102 + 3000 WS2812B", 2000, 3000, fps));
}

fs.mkdirSync("reports", { recursive: true });
fs.writeFileSync("reports/performance-matrix.json", JSON.stringify(rows, null, 2));
const columns = [
  "kind",
  "layout",
  "totalPixels",
  "apa102Pixels",
  "ws2812bPixels",
  "pixelsPerLongestOutput",
  "spiHz",
  "targetFps",
  "theoreticalFrameTimeUs",
  "apa102FrameTimeUs",
  "ws2812bFrameTimeUs",
  "theoreticalMaxFps",
  "artNetDataRateMbps",
  "warning",
  "measured"
];
const csv = [
  columns.join(","),
  ...rows.map(r => columns.map(column => r[column] ?? "").join(","))
].join("\n");
fs.writeFileSync("reports/performance-matrix.csv", csv + "\n");
console.log("Wrote reports/performance-matrix.json and reports/performance-matrix.csv");
