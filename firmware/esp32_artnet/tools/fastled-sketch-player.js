#!/usr/bin/env node
const dgram = require("dgram");
const fs = require("fs");
const path = require("path");

function arg(name, fallback) {
  const index = process.argv.indexOf(name);
  return index >= 0 ? process.argv[index + 1] : fallback;
}

function csvInts(value) {
  return String(value || "").split(",").map(item => Math.max(0, Math.floor(Number(item)))).filter(item => item > 0);
}

const initialHost = arg("--host", "10.0.0.246");
const initialPort = Number(arg("--port", "6454"));
const bindAddress = arg("--bind", "");
const initialStartUniverse = Math.max(0, Math.floor(Number(arg("--start-universe", "0"))));
const initialFps = Number(arg("--fps", "60"));
const seconds = Number(arg("--seconds", "180"));
const initialBrightness = Math.max(0, Math.min(1, Number(arg("--brightness", "0.70"))));
const initialMode = arg("--mode", "fit16");
const initialWiring = arg("--wiring", "serpentine");
const initialSpeed = Math.max(0.01, Number(arg("--speed", "0.35")));
const initialCrossfadeSeconds = Number(arg("--crossfade", "8"));
const initialPatternSeconds = Number(arg("--pattern-seconds", "35"));
const initialPixelCount = Math.max(0, Math.floor(Number(arg("--pixel-count", "0"))));
const initialPortPixels = csvInts(arg("--port-pixels", ""));
const initialPortUniverses = csvInts(arg("--port-universes", ""));
const initialDmxLength = Math.max(2, Math.min(512, Math.floor(Number(arg("--dmx-length", "510")))));
const sequenceMode = arg("--sequence-mode", "increment");
const mappingFile = arg("--mapping-file", "");
const playlistArg = arg("--playlist", "");
const controlFile = arg("--control-file", "");
const dryRun = process.argv.includes("--dry-run");
const loop = process.argv.includes("--loop") || seconds <= 0;

const sourceWidth = 24;
const sourceHeight = 32;
const socket = dgram.createSocket("udp4");

const runtime = {
  host: initialHost,
  port: initialPort,
  startUniverse: initialStartUniverse,
  fps: initialFps,
  brightness: initialBrightness,
  mode: initialMode,
  wiring: initialWiring,
  speed: initialSpeed,
  crossfadeSeconds: initialCrossfadeSeconds,
  patternSeconds: initialPatternSeconds,
  pixelCount: initialPixelCount,
  portPixels: initialPortPixels,
  portUniverses: initialPortUniverses,
  dmxLength: initialDmxLength % 2 === 0 ? initialDmxLength : initialDmxLength + 1,
  playlistNames: [],
  controlMtimeMs: 0,
  nextControlPollMs: 0,
};

function loadMappedCoords(filePath) {
  if (!filePath) return [];
  const absolutePath = path.resolve(process.cwd(), filePath);
  const raw = fs.readFileSync(absolutePath, "utf8");
  const rows = JSON.parse(raw);
  if (!Array.isArray(rows)) throw new Error(`mapping is not an array: ${absolutePath}`);
  const points = rows
    .map(row => Array.isArray(row) ? { x: Number(row[0]), y: Number(row[1]) } : null)
    .filter(point => point && Number.isFinite(point.x) && Number.isFinite(point.y));
  if (!points.length) throw new Error(`mapping contains no usable points: ${absolutePath}`);
  const xs = points.map(point => point.x);
  const ys = points.map(point => point.y);
  const minX = Math.min(...xs);
  const maxX = Math.max(...xs);
  const minY = Math.min(...ys);
  const maxY = Math.max(...ys);
  const spanX = Math.max(0.000001, maxX - minX);
  const spanY = Math.max(0.000001, maxY - minY);
  return points.map(point => ({
    x: (point.x - minX) / spanX * (sourceWidth - 1),
    y: (point.y - minY) / spanY * (sourceHeight - 1),
  }));
}

const mappedCoords = loadMappedCoords(mappingFile);

function isMappedMode() {
  return runtime.mode === "schrankwand_onlyu";
}

function outWidth() {
  if (isMappedMode()) return runtime.pixelCount || mappedCoords.length || 1;
  return runtime.mode === "native24x32" ? 24 : 16;
}

function outHeight() {
  if (isMappedMode()) return 1;
  return runtime.mode === "native24x32" ? 32 : 16;
}

function pixelCount() {
  if (runtime.portPixels.length) return runtime.portPixels.reduce((sum, count) => sum + count, 0);
  if (isMappedMode()) return runtime.pixelCount || mappedCoords.length;
  return outWidth() * outHeight();
}

function universeCount() {
  if (runtime.portPixels.length && runtime.portUniverses.length) {
    return runtime.portUniverses.reduce((sum, count) => sum + count, 0);
  }
  return Math.ceil(pixelCount() / 170);
}

function artnetSlotToLogicalPixel(artnetPixel) {
  if (!runtime.portPixels.length || !runtime.portUniverses.length) return artnetPixel;
  let slotBase = 0;
  let pixelBase = 0;
  for (let portIndex = 0; portIndex < runtime.portPixels.length; portIndex++) {
    const portSlots = (runtime.portUniverses[portIndex] || 0) * 170;
    if (artnetPixel >= slotBase && artnetPixel < slotBase + portSlots) {
      const inPortSlot = artnetPixel - slotBase;
      if (inPortSlot >= runtime.portPixels[portIndex]) return -1;
      return pixelBase + inPortSlot;
    }
    slotBase += portSlots;
    pixelBase += runtime.portPixels[portIndex];
  }
  return -1;
}

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

function clampByte(value) {
  return Math.max(0, Math.min(255, Math.round(value)));
}

function smoothstep(t) {
  return t * t * (3 - 2 * t);
}

function easeInOutCubic(t) {
  return t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2;
}

function hash2(x, y, seed) {
  let n = Math.imul(x, 374761393) ^ Math.imul(y, 668265263) ^ Math.imul(seed, 1442695041);
  n = (n ^ (n >>> 13)) >>> 0;
  n = Math.imul(n, 1274126177) >>> 0;
  return ((n ^ (n >>> 16)) >>> 0) / 4294967295;
}

function valueNoise(x, y, seed) {
  const xi = Math.floor(x);
  const yi = Math.floor(y);
  const xf = x - xi;
  const yf = y - yi;
  const u = smoothstep(xf);
  const v = smoothstep(yf);
  const a = hash2(xi, yi, seed);
  const b = hash2(xi + 1, yi, seed);
  const c = hash2(xi, yi + 1, seed);
  const d = hash2(xi + 1, yi + 1, seed);
  const x1 = a + (b - a) * u;
  const x2 = c + (d - c) * u;
  return x1 + (x2 - x1) * v;
}

function fbm(x, y, seed) {
  let amp = 0.55;
  let freq = 1;
  let total = 0;
  let norm = 0;
  for (let octave = 0; octave < 4; octave++) {
    total += valueNoise(x * freq, y * freq, seed + octave * 31) * amp;
    norm += amp;
    amp *= 0.5;
    freq *= 2.05;
  }
  return total / norm;
}

function triwave(value) {
  const t = ((value % 1) + 1) % 1;
  return t < 0.5 ? t * 2 : 2 - t * 2;
}

function frac(value) {
  return value - Math.floor(value);
}

function triangle(value) {
  return triwave(value);
}

function wave(value) {
  return (Math.sin(value * Math.PI * 2) + 1) * 0.5;
}

function gamma2(value, minimum = 0) {
  const corrected = Math.pow(Math.max(0, Math.min(1, value)), 2);
  return clampByte(corrected * 255 + minimum);
}

function outPixelToXY(pixel) {
  const width = outWidth();
  const row = Math.floor(pixel / width);
  const wiredCol = pixel % width;
  const col = runtime.wiring === "straight" || row % 2 === 0 ? wiredCol : width - 1 - wiredCol;
  return { x: col, y: row };
}

function sourceCoord(pixel) {
  if (isMappedMode()) {
    const mapped = mappedCoords[pixel];
    if (mapped) return mapped;
    return { x: 0, y: 0, unmapped: true };
  }
  const { x, y } = outPixelToXY(pixel);
  if (runtime.mode === "native24x32") return { x, y };
  return {
    x: x * (sourceWidth - 1) / Math.max(1, outWidth() - 1),
    y: y * (sourceHeight - 1) / Math.max(1, outHeight() - 1),
  };
}

function addRgb(a, b) {
  return [clampByte(a[0] + b[0]), clampByte(a[1] + b[1]), clampByte(a[2] + b[2])];
}

function blend(a, b, amount) {
  return [
    a[0] + (b[0] - a[0]) * amount,
    a[1] + (b[1] - a[1]) * amount,
    a[2] + (b[2] - a[2]) * amount,
  ];
}

function hsvToRgb(h, s, v) {
  const hue = ((h % 1) + 1) % 1;
  const i = Math.floor(hue * 6);
  const f = hue * 6 - i;
  const p = v * (1 - s);
  const q = v * (1 - f * s);
  const t = v * (1 - (1 - f) * s);
  const table = [
    [v, t, p], [q, v, p], [p, v, t],
    [p, q, v], [t, p, v], [v, p, q],
  ];
  return table[i % 6].map(channel => clampByte(channel * 255));
}

function noiseNoise2(x, y, t) {
  const n1 = fbm(x * 0.10 + Math.sin(t * 0.33) * 2.2, y * 0.10 + t * 0.22, 11);
  const n2 = fbm(x * 0.13 - t * 0.18, y * 0.11 + Math.cos(t * 0.21) * 2.0, 43);
  const red = gamma2(triwave(n1 * 1.8 + t * 0.045), 1);
  const blue = gamma2(triwave(n2 * 1.9 - t * 0.035), 1);
  const mix = Math.max(0.08, Math.min(0.95, n2 * 0.72 + 0.10));
  return blend([red, 0, 0], [0, 0, blue], mix);
}

function rotatingBlob(x, y, t) {
  const cx = 12 + Math.cos(t * 0.42) * 7.5;
  const cy = 16 + Math.sin(t * 0.31) * 10.5;
  const dx = x - cx;
  const dy = y - cy;
  const angle = Math.atan2(dy, dx);
  const dist = Math.hypot(dx / 12, dy / 16);
  const field = fbm(x * 0.12 + Math.cos(angle + t) * 2, y * 0.10 + Math.sin(angle - t) * 2, 89);
  const v = Math.max(0, 1.25 - dist * 1.35) * 0.75 + field * 0.55;
  const wave = triwave(v + t * 0.055);
  const red = gamma2(wave, 1);
  const magentaLift = gamma2(wave * 0.10, 0);
  return [red, 0, magentaLift];
}

function wavesAnimation(x, y, t) {
  const waveA = Math.sin((x * 0.42) + t * 1.7);
  const waveB = Math.sin((y * 0.38) - t * 1.3);
  const field = fbm(x * 0.08 + t * 0.11, y * 0.16 - t * 0.08, 137);
  const v = (waveA + waveB + 2) / 4 * 0.58 + field * 0.42;
  const blue = gamma2(triwave(v + t * 0.025), 1);
  return [gamma2(blue / 255 * 0.12, 0), 0, blue];
}

const patterns = [
  { name: "test_black", fn: () => [0, 0, 0] },
  { name: "test_solid_red", fn: () => [255, 0, 0] },
  { name: "test_solid_green", fn: () => [0, 255, 0] },
  { name: "test_solid_blue", fn: () => [0, 0, 255] },
  { name: "test_solid_white", fn: () => [255, 255, 255] },
  { name: "noise_noise2", fn: noiseNoise2 },
  { name: "rotating_blob", fn: rotatingBlob },
  { name: "waves_animation", fn: wavesAnimation },
];

const patternMap = new Map(patterns.map(pattern => [pattern.name, pattern]));
runtime.playlistNames = playlistArg
  ? playlistArg.split(",").map(name => name.trim()).filter(name => patternMap.has(name))
  : patternMap.has(initialMode)
    ? [initialMode]
  : patterns.slice(0, 3).map(pattern => pattern.name);

function activePatterns() {
  const names = runtime.playlistNames.length ? runtime.playlistNames : patterns.slice(0, 3).map(pattern => pattern.name);
  return names.map(name => patternMap.get(name)).filter(Boolean);
}

function applyLiveSettings(settings) {
  if (!settings || typeof settings !== "object") return;
  if (settings.host) runtime.host = String(settings.host);
  if (Number.isFinite(Number(settings.port))) runtime.port = Number(settings.port);
  if (Number.isFinite(Number(settings.startUniverse))) runtime.startUniverse = Math.max(0, Math.floor(Number(settings.startUniverse)));
  if (Number.isFinite(Number(settings.fps))) runtime.fps = Math.max(1, Math.min(120, Number(settings.fps)));
  if (Number.isFinite(Number(settings.brightness))) runtime.brightness = Math.max(0, Math.min(1, Number(settings.brightness)));
  if (Number.isFinite(Number(settings.speed))) runtime.speed = Math.max(0.01, Math.min(3, Number(settings.speed)));
  if (Number.isFinite(Number(settings.patternSeconds))) runtime.patternSeconds = Math.max(5, Math.min(600, Number(settings.patternSeconds)));
  if (Number.isFinite(Number(settings.crossfade))) runtime.crossfadeSeconds = Math.max(0, Math.min(120, Number(settings.crossfade)));
  if (settings.mode === "fit16" || settings.mode === "native24x32" || settings.mode === "schrankwand_onlyu") runtime.mode = settings.mode;
  if (settings.wiring === "serpentine" || settings.wiring === "straight") runtime.wiring = settings.wiring;
  if (Array.isArray(settings.portPixels)) runtime.portPixels = settings.portPixels.map(value => Math.max(0, Math.floor(Number(value)))).filter(value => value > 0);
  if (Array.isArray(settings.portUniverses)) runtime.portUniverses = settings.portUniverses.map(value => Math.max(0, Math.floor(Number(value)))).filter(value => value > 0);
  if (!Array.isArray(settings.portUniverses) && Number.isFinite(Number(settings.portUniverses))) runtime.portUniverses = [Math.max(0, Math.floor(Number(settings.portUniverses)))].filter(value => value > 0);
  if (Array.isArray(settings.playlist)) {
    const next = settings.playlist.filter(name => patternMap.has(name));
    if (next.length) runtime.playlistNames = next;
  }
  console.log(`live settings brightness=${runtime.brightness} speed=${runtime.speed} fps=${runtime.fps} playlist=${runtime.playlistNames.join(",")}`);
}

function startLiveControl() {
  if (!process.stdin || !process.stdin.setEncoding) return;
  process.stdin.setEncoding("utf8");
  let pending = "";
  process.stdin.on("data", chunk => {
    pending += chunk;
    let newline;
    while ((newline = pending.indexOf("\n")) >= 0) {
      const line = pending.slice(0, newline).trim();
      pending = pending.slice(newline + 1);
      if (!line) continue;
      try {
        applyLiveSettings(JSON.parse(line));
      } catch (error) {
        console.error(`live settings failed: ${error.message}`);
      }
    }
  });
}

function pollControlFile() {
  if (!controlFile) return;
  const now = Date.now();
  if (now < runtime.nextControlPollMs) return;
  runtime.nextControlPollMs = now + 150;
  try {
    const stat = fs.statSync(controlFile);
    if (stat.mtimeMs <= runtime.controlMtimeMs) return;
    runtime.controlMtimeMs = stat.mtimeMs;
    applyLiveSettings(JSON.parse(fs.readFileSync(controlFile, "utf8")));
  } catch (error) {
    if (error.code !== "ENOENT") {
      console.error(`control file failed: ${error.message}`);
    }
  }
}

function frameColors(frame) {
  const fps = runtime.fps;
  const active = activePatterns();
  const realT = frame / fps;
  const t = realT * runtime.speed;
  const cycle = runtime.patternSeconds;
  const currentIndex = Math.floor(realT / cycle) % active.length;
  const nextIndex = (currentIndex + 1) % active.length;
  const cycleT = realT % cycle;
  const fadeStart = Math.max(0, cycle - runtime.crossfadeSeconds);
  const fadeAmount = cycleT >= fadeStart
    ? easeInOutCubic((cycleT - fadeStart) / Math.max(0.001, runtime.crossfadeSeconds))
    : 0;
  const activePattern = fadeAmount > 0 ? `${active[currentIndex].name}->${active[nextIndex].name}` : active[currentIndex].name;
  const colors = new Array(pixelCount());
  for (let pixel = 0; pixel < colors.length; pixel++) {
    const { x, y } = sourceCoord(pixel);
    if (isMappedMode() && pixel >= mappedCoords.length) {
      colors[pixel] = [0, 0, 0];
      continue;
    }
    const a = active[currentIndex].fn(x, y, t, pixel);
    const b = fadeAmount > 0 ? active[nextIndex].fn(x, y, t, pixel) : a;
    colors[pixel] = blend(a, b, fadeAmount).map(channel => clampByte(channel * runtime.brightness));
  }
  return { colors, activePattern };
}

function artDmxPacket(universe, sequence, colors) {
  const dmxLength = runtime.dmxLength;
  const packet = Buffer.alloc(18 + dmxLength);
  packet.write("Art-Net\0", 0, "ascii");
  packet.writeUInt16LE(0x5000, 8);
  packet.writeUInt16BE(14, 10);
  packet[12] = sequence;
  packet[13] = 0;
  packet.writeUInt16LE(runtime.startUniverse + universe, 14);
  packet.writeUInt16BE(dmxLength, 16);
  for (let i = 0; i < Math.floor(dmxLength / 3); i++) {
    const pixel = artnetSlotToLogicalPixel(universe * 170 + i);
    const [r, g, b] = pixel >= 0 && pixel < colors.length ? colors[pixel] : [0, 0, 0];
    const offset = 18 + i * 3;
    if (offset + 2 >= packet.length) break;
    packet[offset] = r;
    packet[offset + 1] = g;
    packet[offset + 2] = b;
  }
  return packet;
}

async function sendFrame(frame) {
  const { colors, activePattern } = frameColors(frame);
  const universes = universeCount();
  for (let universe = 0; universe < universes; universe++) {
    const sequence = sequenceMode === "fixed" ? 0 : (frame % 255) + 1;
    const packet = artDmxPacket(universe, sequence, colors);
    await new Promise((resolve, reject) => {
      socket.send(packet, runtime.port, runtime.host, error => error ? reject(error) : resolve());
    });
  }
  return activePattern;
}

async function main() {
  console.log(`FastLED sketch player target=${runtime.host}:${runtime.port} mode=${runtime.mode} pixels=${pixelCount()} universes=${universeCount()} startUniverse=${runtime.startUniverse} dmxLength=${runtime.dmxLength} sequenceMode=${sequenceMode} bind=${bindAddress || "-"} fps=${runtime.fps} brightness=${runtime.brightness} speed=${runtime.speed} ${loop ? "loop=true" : `seconds=${seconds}`}`);
  if (isMappedMode()) {
    console.log(`mapping=${mappingFile || "-"} mappedPixels=${mappedCoords.length} requestedPixels=${pixelCount()}`);
    if (mappedCoords.length && mappedCoords.length < pixelCount()) {
      console.log(`mapping warning: ${pixelCount() - mappedCoords.length} requested pixels have no mapping and stay black`);
    }
  }
  if (runtime.portPixels.length && runtime.portUniverses.length) {
    console.log(`ports=${runtime.portPixels.join(",")} portUniverses=${runtime.portUniverses.join(",")} artnetUniverses=${universeCount()} artnetSlots=${universeCount() * 170}`);
  }
  console.log(`patterns=${activePatterns().map(pattern => pattern.name).join(", ")} crossfade=${runtime.crossfadeSeconds}s every=${runtime.patternSeconds}s`);
  if (bindAddress) {
    await new Promise((resolve, reject) => {
      socket.once("error", reject);
      socket.bind(0, bindAddress, () => {
        socket.removeListener("error", reject);
        resolve();
      });
    });
    console.log(`bound=${bindAddress}`);
  }
  startLiveControl();
  if (dryRun) {
    const { colors, activePattern } = frameColors(0);
    console.log(`dry-run ok firstPattern=${activePattern} firstPixel=${colors[0].join(",")}`);
    socket.close();
    return;
  }

  const totalFrames = loop ? Number.POSITIVE_INFINITY : Math.max(1, Math.round(seconds * runtime.fps));
  let nextFrameAt = Date.now();
  let lastPattern = "";
  for (let frame = 0; frame < totalFrames; frame++) {
    pollControlFile();
    const activePattern = await sendFrame(frame);
    if (activePattern !== lastPattern) {
      console.log(`pattern ${activePattern}`);
      lastPattern = activePattern;
    }
    nextFrameAt += 1000 / runtime.fps;
    const waitMs = nextFrameAt - Date.now();
    if (waitMs > 0) await sleep(waitMs);
  }
  socket.close();
  console.log("done");
}

main().catch(error => {
  socket.close();
  console.error(error);
  process.exit(1);
});
