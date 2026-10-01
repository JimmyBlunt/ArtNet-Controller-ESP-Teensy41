#!/usr/bin/env node
const dgram = require("dgram");

function arg(name, fallback) {
  const index = process.argv.indexOf(name);
  return index >= 0 ? process.argv[index + 1] : fallback;
}

const host = arg("--host", "10.0.0.246");
const port = Number(arg("--port", "6454"));
const wiring = arg("--wiring", "serpentine");
const targetFps = Number(arg("--fps", "30"));
const universeGapMs = Number(arg("--gap", "0"));
const universeCount = Number(arg("--universes", "2"));
const brightness = Math.max(0, Math.min(1, Number(arg("--brightness", "1"))));
const background = Math.max(0, Math.min(255, Number(arg("--background", "0"))));
const durationScale = Math.max(0.1, Number(arg("--duration-scale", "1")));
const dryRun = process.argv.includes("--dry-run");
const socket = dgram.createSocket("udp4");

const width = 16;
const height = 16;
const activePixels = width * height;
const totalPixels = universeCount * 170;

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

function pixelToXY(pixel) {
  const row = Math.floor(pixel / width);
  const wiredCol = pixel % width;
  const col = wiring === "straight" || row % 2 === 0 ? wiredCol : width - 1 - wiredCol;
  return { x: col, y: row };
}

function hsvToRgb(h, s, v) {
  const c = v * s;
  const hp = (h % 360) / 60;
  const x = c * (1 - Math.abs((hp % 2) - 1));
  let [r, g, b] = [0, 0, 0];
  if (hp < 1) [r, g, b] = [c, x, 0];
  else if (hp < 2) [r, g, b] = [x, c, 0];
  else if (hp < 3) [r, g, b] = [0, c, x];
  else if (hp < 4) [r, g, b] = [0, x, c];
  else if (hp < 5) [r, g, b] = [x, 0, c];
  else [r, g, b] = [c, 0, x];
  const m = v - c;
  return [Math.round((r + m) * 255), Math.round((g + m) * 255), Math.round((b + m) * 255)];
}

function colorAt(pattern, pixel, frame) {
  if (pixel >= activePixels) return [0, 0, 0];
  const { x, y } = pixelToXY(pixel);
  const bg = [0, 0, background];
  if (pattern === "off") return [0, 0, 0];
  if (pattern === "white") return [120, 120, 120];
  if (pattern === "checker") return ((x + y + frame) % 2) === 0 ? [160, 160, 160] : bg;
  if (pattern === "row-scan") return y === frame % height ? [255, 80, 0] : bg;
  if (pattern === "col-scan") return x === frame % width ? [0, 180, 255] : bg;
  if (pattern === "diagonal") {
    const d = (x + y + frame) % 32;
    if (d < 3) return [255, 255, 255];
    if (d < 7) return [255, 0, 100];
    return bg;
  }
  if (pattern === "plasma") {
    const v = Math.sin((x + frame) * 0.45) + Math.sin((y - frame) * 0.35) + Math.sin((x + y + frame) * 0.22);
    return hsvToRgb((v + 3) * 60 + frame * 4, 0.9, 0.55);
  }
  if (pattern === "orbit") {
    const cx = 7.5 + Math.cos(frame * 0.18) * 5.5;
    const cy = 7.5 + Math.sin(frame * 0.13) * 5.5;
    const dist = Math.hypot(x - cx, y - cy);
    if (dist < 1.6) return [255, 255, 255];
    if (dist < 3.1) return [0, 100, 255];
    return bg;
  }
  return [0, 0, 0];
}

function artDmxPacket(universe, sequence, pattern, frame) {
  const packet = Buffer.alloc(18 + 512);
  packet.write("Art-Net\0", 0, "ascii");
  packet.writeUInt16LE(0x5000, 8);
  packet.writeUInt16BE(14, 10);
  packet[12] = sequence || 1;
  packet[13] = 0;
  packet.writeUInt16LE(universe, 14);
  packet.writeUInt16BE(512, 16);
  for (let i = 0; i < 170; i++) {
    const pixel = universe * 170 + i;
    const [r, g, b] = pixel < totalPixels ? colorAt(pattern, pixel, frame) : [0, 0, 0];
    const offset = 18 + i * 3;
    if (offset + 2 >= packet.length) break;
    packet[offset] = Math.round(r * brightness);
    packet[offset + 1] = Math.round(g * brightness);
    packet[offset + 2] = Math.round(b * brightness);
  }
  return packet;
}

async function sendFrame(pattern, frame, gapMs = universeGapMs) {
  for (let universe = 0; universe < universeCount; universe++) {
    const packet = artDmxPacket(universe, frame % 255 || 1, pattern, frame);
    await new Promise((resolve, reject) => {
      socket.send(packet, port, host, error => error ? reject(error) : resolve());
    });
    if (gapMs > 0) await sleep(gapMs);
  }
}

async function play(pattern, frames, fps = targetFps) {
  console.log(`pattern ${pattern}`);
  const frameMs = 1000 / fps;
  let nextFrameAt = Date.now();
  for (let frame = 0; frame < frames; frame++) {
    await sendFrame(pattern, frame);
    nextFrameAt += frameMs;
    const waitMs = nextFrameAt - Date.now();
    if (waitMs > 0) await sleep(waitMs);
  }
}

async function main() {
  console.log(`Output0 16x16 animation target=${host}:${port} wiring=${wiring} fps=${targetFps} gap=${universeGapMs}ms universes=${universeCount} brightness=${brightness} background=${background} durationScale=${durationScale}`);
  if (dryRun) {
    console.log("dry-run ok");
    return;
  }
  await play("off", Math.round(15 * durationScale), 15);
  await play("white", Math.round(20 * durationScale), 15);
  await play("row-scan", Math.round(48 * durationScale));
  await play("col-scan", Math.round(48 * durationScale));
  await play("checker", Math.round(32 * durationScale));
  await play("diagonal", Math.round(120 * durationScale));
  await play("orbit", Math.round(150 * durationScale));
  await play("plasma", Math.round(220 * durationScale));
  await play("white", Math.round(20 * durationScale), 15);
  socket.close();
  console.log("done");
}

main().catch(error => {
  console.error(error);
  process.exit(1);
});
