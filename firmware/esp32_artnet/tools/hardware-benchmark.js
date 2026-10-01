#!/usr/bin/env node
const dgram = require("dgram");
const fs = require("fs");
const http = require("http");
const path = require("path");
const { performance } = require("perf_hooks");

const SCENARIOS = {
  mixed_4500_30: { pixels: 4500, universes: 27, fps: 30 },
  mixed_4500_40: { pixels: 4500, universes: 27, fps: 40 },
  mixed_4500_60: { pixels: 4500, universes: 27, fps: 60 },
  ws2812_3000_60: { pixels: 3000, universes: 18, fps: 60 },
  apa102_1500_60: { pixels: 1500, universes: 9, fps: 60 }
};

function getArg(args, flag, fallback) {
  const index = args.indexOf(flag);
  return index >= 0 ? args[index + 1] : fallback;
}

function artDmxPacket(universe, sequence, frame) {
  const packet = Buffer.alloc(18 + 512);
  packet.write("Art-Net\0", 0, "ascii");
  packet.writeUInt16LE(0x5000, 8);
  packet.writeUInt16BE(14, 10);
  packet[12] = sequence || 1;
  packet[13] = 0;
  packet.writeUInt16LE(universe, 14);
  packet.writeUInt16BE(512, 16);
  for (let i = 18; i < packet.length; i += 3) {
    const pixel = universe * 170 + ((i - 18) / 3);
    packet[i] = (pixel + frame) & 255;
    packet[i + 1] = (pixel * 3 + frame) & 255;
    packet[i + 2] = (pixel * 7 + frame) & 255;
  }
  return packet;
}

function requestJson(host, route) {
  const started = performance.now();
  return new Promise((resolve, reject) => {
    const req = http.get({ host, port: 80, path: route, timeout: 3000 }, res => {
      let body = "";
      res.setEncoding("utf8");
      res.on("data", chunk => { body += chunk; });
      res.on("end", () => {
        const latencyMs = performance.now() - started;
        try {
          resolve({ statusCode: res.statusCode, latencyMs, body: JSON.parse(body) });
        } catch (error) {
          reject(new Error(`${route} returned non-JSON body: ${body.slice(0, 120)}`));
        }
      });
    });
    req.on("timeout", () => req.destroy(new Error(`${route} timed out`)));
    req.on("error", reject);
  });
}

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function sendArtNet(host, scenario, durationSeconds) {
  const socket = dgram.createSocket("udp4");
  const frames = Math.max(1, Math.round(durationSeconds * scenario.fps));
  const frameIntervalMs = 1000 / scenario.fps;
  const started = performance.now();
  let sentPackets = 0;

  for (let frame = 0; frame < frames; frame++) {
    const nextFrameAt = started + frame * frameIntervalMs;
    for (let universe = 0; universe < scenario.universes; universe++) {
      await new Promise((resolve, reject) => {
        socket.send(artDmxPacket(universe, (frame % 255) + 1, frame), 6454, host, error => {
          if (error) reject(error);
          else resolve();
        });
      });
      sentPackets++;
    }
    const waitMs = nextFrameAt + frameIntervalMs - performance.now();
    if (waitMs > 0) await sleep(waitMs);
  }
  socket.close();
  return { sentPackets, sentFrames: frames, elapsedSeconds: (performance.now() - started) / 1000 };
}

function derive(before, after, sent) {
  const packetDelta = Math.max(0, (after.packets || 0) - (before.packets || 0));
  const completeDelta = Math.max(0, (after.framesComplete || 0) - (before.framesComplete || 0));
  const incompleteDelta = Math.max(0, (after.framesIncomplete || 0) - (before.framesIncomplete || 0));
  return {
    receivedPackets: packetDelta,
    packetLossEstimated: Math.max(0, sent.sentPackets - packetDelta),
    framesCompleteDelta: completeDelta,
    framesIncompleteDelta: incompleteDelta,
    completionRatio: sent.sentFrames > 0 ? Number((completeDelta / sent.sentFrames).toFixed(4)) : 0
  };
}

async function run(options) {
  const scenario = SCENARIOS[options.scenario];
  if (!scenario) throw new Error(`Unknown scenario: ${options.scenario}`);

  if (options.dryRun) {
    return {
      measured: false,
      transport: "dry-run",
      scenario: options.scenario,
      targetHost: options.host,
      durationSeconds: options.durationSeconds,
      plannedPackets: Math.round(options.durationSeconds * scenario.fps) * scenario.universes
    };
  }

  const beforeRequest = await requestJson(options.host, "/api/status");
  const sent = await sendArtNet(options.host, scenario, options.durationSeconds);
  await sleep(options.settleMs);
  const afterRequest = await requestJson(options.host, "/api/status");
  const perfRequest = await requestJson(options.host, "/api/performance");

  return {
    measured: true,
    transport: "esp32-wifi-api",
    scenario: options.scenario,
    targetHost: options.host,
    startedAt: new Date().toISOString(),
    durationSeconds: options.durationSeconds,
    targetFps: scenario.fps,
    universes: scenario.universes,
    pixels: scenario.pixels,
    sent,
    before: beforeRequest.body,
    after: afterRequest.body,
    performance: perfRequest.body,
    webLatencyMs: {
      statusBefore: Number(beforeRequest.latencyMs.toFixed(2)),
      statusAfter: Number(afterRequest.latencyMs.toFixed(2)),
      performance: Number(perfRequest.latencyMs.toFixed(2))
    },
    derived: derive(beforeRequest.body, afterRequest.body, sent),
    notes: [
      "Measured through ESP32 HTTP API counters after UDP Art-Net input.",
      "LED signal timing and physical LED output are not verified by this tool."
    ]
  };
}

async function main() {
  const args = process.argv.slice(2);
  const options = {
    host: getArg(args, "--host", "10.0.0.246"),
    scenario: getArg(args, "--scenario", "mixed_4500_30"),
    durationSeconds: Number(getArg(args, "--duration", "5")),
    settleMs: Number(getArg(args, "--settle-ms", "750")),
    report: getArg(args, "--report", "reports/hardware-benchmark"),
    dryRun: args.includes("--dry-run")
  };

  const result = await run(options);
  fs.mkdirSync(path.dirname(options.report), { recursive: true });
  fs.writeFileSync(`${options.report}.json`, JSON.stringify(result, null, 2));
  console.log(`Wrote ${options.report}.json`);
  console.log(JSON.stringify({
    measured: result.measured,
    transport: result.transport,
    scenario: result.scenario,
    derived: result.derived || null,
    webLatencyMs: result.webLatencyMs || null
  }));
}

if (require.main === module) {
  main().catch(error => {
    console.error(error);
    process.exit(1);
  });
}

module.exports = { SCENARIOS, artDmxPacket, derive, run };
