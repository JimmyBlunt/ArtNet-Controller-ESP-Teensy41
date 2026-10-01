#!/usr/bin/env node
const dgram = require("dgram");

function arg(name, fallback) {
  const index = process.argv.indexOf(name);
  return index >= 0 ? process.argv[index + 1] : fallback;
}

const host = arg("--host", "10.0.0.246");
const port = Number(arg("--port", "6454"));
const pixelCount = Number(arg("--pixels", "4500"));
const universeCount = Number(arg("--universes", "27"));
const dryRun = process.argv.includes("--dry-run");

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

function pixelColor(pattern, pixel, frame) {
  if (pattern === "off") return [0, 0, 0];
  if (pattern === "white") return [255, 255, 255];
  if (pattern === "red") return [255, 0, 0];
  if (pattern === "green") return [0, 255, 0];
  if (pattern === "blue") return [0, 80, 255];
  if (pattern === "blocks") {
    const block = Math.floor(pixel / 170) % 6;
    return [[255, 0, 0], [255, 140, 0], [255, 255, 0], [0, 255, 0], [0, 120, 255], [180, 0, 255]][block];
  }
  if (pattern === "chase") {
    const head = (frame * 90) % pixelCount;
    const distance = (pixel - head + pixelCount) % pixelCount;
    if (distance < 60) return [255, 255, 255];
    if (distance < 140) return [255, 40, 0];
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
    const [r, g, b] = pixel < pixelCount ? pixelColor(pattern, pixel, frame) : [0, 0, 0];
    const offset = 18 + i * 3;
    if (offset + 2 >= packet.length) break;
    packet[offset] = r;
    packet[offset + 1] = g;
    packet[offset + 2] = b;
  }
  return packet;
}

async function sendFrame(socket, pattern, frame, gapMs = 8) {
  for (let universe = 0; universe < universeCount; universe++) {
    const packet = artDmxPacket(universe, (frame % 255) + 1, pattern, frame);
    await new Promise((resolve, reject) => {
      socket.send(packet, port, host, error => error ? reject(error) : resolve());
    });
    await sleep(gapMs);
  }
}

async function main() {
  console.log(`Visualizer test target=${host}:${port} pixels=${pixelCount} universes=${universeCount}`);
  if (dryRun) {
    console.log("dry-run ok");
    return;
  }
  const socket = dgram.createSocket("udp4");
  let frame = 20;
  for (const pattern of ["off", "white", "red", "green", "blue", "blocks"]) {
    console.log(`pattern ${pattern}`);
    await sendFrame(socket, pattern, frame++);
    await sleep(1100);
  }
  console.log("pattern chase");
  for (let i = 0; i < 80; i++) {
    await sendFrame(socket, "chase", frame++, 3);
    await sleep(55);
  }
  console.log("pattern white final");
  await sendFrame(socket, "white", frame++);
  socket.close();
  console.log("done");
}

main().catch(error => {
  console.error(error);
  process.exit(1);
});
