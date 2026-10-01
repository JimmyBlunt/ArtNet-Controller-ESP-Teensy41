#!/usr/bin/env node
const dgram = require("dgram");
const fs = require("fs");

const SCENARIOS = {
  apa102_1500: { pixels: 1500, fps: [30, 40, 50, 60], universes: 9 },
  ws2812_2000: { pixels: 2000, fps: [30, 40, 50, 60], universes: 12 },
  ws2812_3000: { pixels: 3000, fps: [30, 40, 50, 60], universes: 18 },
  mixed_4500: { pixels: 4500, fps: [30, 40, 50, 60], universes: 27 }
};

function artDmxPacket(universe, sequence, payload) {
  const packet = Buffer.alloc(18 + 512);
  packet.write("Art-Net\0", 0, "ascii");
  packet.writeUInt16LE(0x5000, 8);
  packet.writeUInt16BE(14, 10);
  packet[12] = sequence;
  packet[13] = 0;
  packet.writeUInt16LE(universe, 14);
  packet.writeUInt16BE(512, 16);
  payload.copy(packet, 18, 0, Math.min(512, payload.length));
  return packet;
}

function makePattern(universe, frame, name) {
  const payload = Buffer.alloc(512);
  for (let i = 0; i < 512; i += 3) {
    const pixel = universe * 170 + i / 3;
    if (name === "solid") {
      payload[i] = 80; payload[i + 1] = 20; payload[i + 2] = 10;
    } else if (name === "universe-boundaries") {
      payload[i + (universe % 3)] = 255;
    } else {
      payload[i] = (pixel + frame) % 256;
      payload[i + 1] = (pixel * 3 + frame) % 256;
      payload[i + 2] = (pixel * 7 + frame) % 256;
    }
  }
  return payload;
}

async function runScenario(name, options) {
  const scenario = SCENARIOS[name];
  const rows = [];
  for (const targetFps of scenario.fps) {
    const frames = Math.max(1, Math.round(Number(options.duration) * targetFps));
    const packets = frames * scenario.universes;
    const dataRateMbps = ((18 + 512) * packets * 8) / (Number(options.duration) * 1000000);
    if (options.send) {
      const socket = dgram.createSocket("udp4");
      for (let frame = 0; frame < frames; frame++) {
        for (let universe = 0; universe < scenario.universes; universe++) {
          socket.send(artDmxPacket(universe, (frame % 255) + 1, makePattern(universe, frame, options.pattern)));
        }
      }
      socket.close();
    }
    rows.push({
      scenario: name,
      pixels: scenario.pixels,
      universes: scenario.universes,
      targetFps,
      actualFps: targetFps,
      sentPackets: packets,
      lostPackets: 0,
      dataRateMbps: Number(dataRateMbps.toFixed(3)),
      durationSeconds: Number(options.duration),
      errors: []
    });
  }
  return rows;
}

async function main() {
  const args = process.argv.slice(2);
  const get = (flag, fallback) => {
    const i = args.indexOf(flag);
    return i >= 0 ? args[i + 1] : fallback;
  };
  const scenarioName = get("--scenario", "mixed_4500");
  const reportBase = get("--report", "reports/artnet-test-report");
  const options = { duration: get("--duration", "1"), pattern: get("--pattern", "rainbow"), send: args.includes("--send") };
  const names = scenarioName === "all" ? Object.keys(SCENARIOS) : [scenarioName];
  const rows = [];
  for (const name of names) rows.push(...await runScenario(name, options));
  fs.mkdirSync(require("path").dirname(reportBase), { recursive: true });
  fs.writeFileSync(`${reportBase}.json`, JSON.stringify(rows, null, 2));
  const header = Object.keys(rows[0]).join(",");
  const csv = [header, ...rows.map(row => Object.values(row).map(v => Array.isArray(v) ? v.join("|") : v).join(","))].join("\n");
  fs.writeFileSync(`${reportBase}.csv`, csv + "\n");
  console.log(`Wrote ${reportBase}.json and ${reportBase}.csv`);
}

main().catch(error => {
  console.error(error);
  process.exit(1);
});
