#!/usr/bin/env node
const fs = require("fs");
const http = require("http");
const path = require("path");
const { spawn } = require("child_process");

const rootDir = path.resolve(__dirname, "..");
const webDir = path.join(rootDir, "web");
const reportsDir = path.join(rootDir, "reports");
const playerPath = path.join(rootDir, "tools", "fastled-sketch-player.js");
const defaultPort = Number(process.env.SCHRANK_PLAYER_GUI_PORT || 8765);
const profile = process.env.SCHRANK_PLAYER_PROFILE || arg("--profile", "matrix");
const isSchrankwandProfile = profile === "schrankwand";
const settingsPath = path.join(reportsDir, isSchrankwandProfile ? "player-gui-settings-schrankwand.json" : "player-gui-settings.json");
const liveSettingsPath = path.join(reportsDir, isSchrankwandProfile ? "player-live-settings-schrankwand.json" : "player-live-settings.json");

function arg(name, fallback) {
  const index = process.argv.indexOf(name);
  return index >= 0 ? process.argv[index + 1] : fallback;
}

const defaultSettings = isSchrankwandProfile ? {
  host: "10.0.0.244",
  startUniverse: 1,
  fps: 60,
  brightness: 1,
  speed: 0.35,
  patternSeconds: 75,
  crossfade: 18,
  mode: "schrankwand_onlyu",
  wiring: "straight",
  pixelCount: 1183,
  mappingFile: path.join("Mappings", "schrankwand -onlyu.txt"),
  portPixels: [509, 272, 402],
  portUniverses: [3, 2, 3],
  playlist: ["test_solid_red"],
} : {
  host: "10.0.0.246",
  fps: 60,
  brightness: 1,
  speed: 0.35,
  patternSeconds: 75,
  crossfade: 18,
  mode: "fit16",
  wiring: "serpentine",
  playlist: ["noise_noise2", "rotating_blob", "waves_animation"],
};

const patternCatalog = [
  { id: "test_black", label: "TEST Aus / Schwarz" },
  { id: "test_solid_red", label: "TEST Voll Rot" },
  { id: "test_solid_green", label: "TEST Voll Gruen" },
  { id: "test_solid_blue", label: "TEST Voll Blau" },
  { id: "test_solid_white", label: "TEST Voll Weiss" },
  { id: "noise_noise2", label: "FastLED Noise Rot/Blau" },
  { id: "rotating_blob", label: "FastLED Rotating Blob" },
  { id: "waves_animation", label: "FastLED Waves Blau" },
];

let player = null;
let playerStartedAt = null;
let lastExit = null;
let currentSettings = loadSettings();

function loadSettings() {
  try {
    return Object.assign({}, defaultSettings, JSON.parse(fs.readFileSync(settingsPath, "utf8")));
  } catch (_error) {
    return Object.assign({}, defaultSettings);
  }
}

function saveSettings(settings) {
  fs.mkdirSync(reportsDir, { recursive: true });
  fs.writeFileSync(settingsPath, JSON.stringify(settings, null, 2));
}

function saveLiveSettings(settings) {
  fs.mkdirSync(reportsDir, { recursive: true });
  fs.writeFileSync(liveSettingsPath, JSON.stringify(settings, null, 2));
}

function clampNumber(value, fallback, min, max) {
  const parsed = Number(value);
  if (!Number.isFinite(parsed)) return fallback;
  return Math.max(min, Math.min(max, parsed));
}

function sanitizeIntList(value, fallback) {
  const source = Array.isArray(value) ? value : String(value || "").split(",");
  const parsed = source
    .map(item => Math.max(0, Math.floor(Number(item))))
    .filter(item => Number.isFinite(item) && item > 0);
  return parsed.length ? parsed : fallback;
}

function sanitizeSettings(input) {
  const allowedPatterns = new Set(patternCatalog.map(pattern => pattern.id));
  const playlist = Array.isArray(input.playlist)
    ? input.playlist.filter(id => allowedPatterns.has(id))
    : defaultSettings.playlist;
  const allowedModes = new Set(["fit16", "native24x32", "schrankwand_onlyu"]);
  const mode = allowedModes.has(input.mode) ? input.mode : defaultSettings.mode;
  return {
    host: String(input.host || defaultSettings.host).trim() || defaultSettings.host,
    startUniverse: clampNumber(input.startUniverse, defaultSettings.startUniverse || 0, 0, 32767),
    fps: clampNumber(input.fps, defaultSettings.fps, 1, 120),
    brightness: clampNumber(input.brightness, defaultSettings.brightness, 0, 1),
    speed: clampNumber(input.speed, defaultSettings.speed, 0.01, 3),
    patternSeconds: clampNumber(input.patternSeconds, defaultSettings.patternSeconds, 5, 600),
    crossfade: clampNumber(input.crossfade, defaultSettings.crossfade, 0, 120),
    mode,
    wiring: input.wiring === "straight" ? "straight" : "serpentine",
    pixelCount: clampNumber(input.pixelCount, defaultSettings.pixelCount || 0, 0, 10000),
    mappingFile: String(input.mappingFile || defaultSettings.mappingFile || ""),
    portPixels: sanitizeIntList(input.portPixels, defaultSettings.portPixels || []),
    portUniverses: sanitizeIntList(input.portUniverses, defaultSettings.portUniverses || []),
    playlist: playlist.length ? playlist : defaultSettings.playlist,
  };
}

function playerArgs(settings) {
  const args = [
    playerPath,
    "--host", settings.host,
    "--start-universe", String(settings.startUniverse || 0),
    "--mode", settings.mode,
    "--wiring", settings.wiring,
    "--fps", String(settings.fps),
    "--brightness", String(settings.brightness),
    "--speed", String(settings.speed),
    "--loop",
    "--pattern-seconds", String(settings.patternSeconds),
    "--crossfade", String(settings.crossfade),
    "--playlist", settings.playlist.join(","),
    "--control-file", liveSettingsPath,
  ];
  if (settings.pixelCount) args.push("--pixel-count", String(settings.pixelCount));
  if (settings.mappingFile) args.push("--mapping-file", settings.mappingFile);
  if (settings.portPixels && settings.portPixels.length) args.push("--port-pixels", settings.portPixels.join(","));
  if (settings.portUniverses && settings.portUniverses.length) args.push("--port-universes", settings.portUniverses.join(","));
  return args;
}

function appendLog(line) {
  fs.mkdirSync(reportsDir, { recursive: true });
  fs.appendFileSync(path.join(reportsDir, "player-control-server.log"), line);
}

function isRunning() {
  return Boolean(player && player.exitCode === null && !player.killed);
}

function startPlayer(settings) {
  currentSettings = sanitizeSettings(settings || currentSettings);
  saveSettings(currentSettings);
  saveLiveSettings(currentSettings);
  if (isRunning()) {
    return { started: false, message: "Player laeuft bereits", state: state() };
  }
  lastExit = null;
  playerStartedAt = Date.now();
  player = spawn(process.execPath, playerArgs(currentSettings), {
    cwd: rootDir,
    windowsHide: false,
    stdio: ["pipe", "pipe", "pipe"],
  });
  appendLog(`[${new Date().toISOString()}] start pid=${player.pid} ${JSON.stringify(currentSettings)}\n`);
  player.stdout.on("data", chunk => appendLog(chunk.toString()));
  player.stderr.on("data", chunk => appendLog(chunk.toString()));
  player.on("exit", (code, signal) => {
    lastExit = { code, signal, at: new Date().toISOString() };
    appendLog(`[${new Date().toISOString()}] exit code=${code} signal=${signal || ""}\n`);
  });
  return { started: true, message: `Player gestartet PID ${player.pid}`, state: state() };
}

function stopPlayer() {
  if (!isRunning()) {
    return { stopped: false, message: "Player laeuft nicht", state: state() };
  }
  const pid = player.pid;
  player.kill();
  appendLog(`[${new Date().toISOString()}] stop pid=${pid}\n`);
  return { stopped: true, message: `Player gestoppt PID ${pid}`, state: state() };
}

function sendLiveSettings() {
  saveLiveSettings(currentSettings);
  if (!isRunning() || !player.stdin || player.stdin.destroyed) return false;
  player.stdin.write(`${JSON.stringify(currentSettings)}\n`);
  return true;
}

function applySettings(settings) {
  const wasRunning = isRunning();
  if (wasRunning) stopPlayer();
  currentSettings = sanitizeSettings(settings || currentSettings);
  saveSettings(currentSettings);
  if (wasRunning) {
    return Object.assign({ restarted: true }, startPlayer(currentSettings));
  }
  return { restarted: false, message: "Einstellungen gespeichert", state: state() };
}

function state() {
  return {
    running: isRunning(),
    profile,
    pid: isRunning() ? player.pid : null,
    startedAt: playerStartedAt,
    uptimeSeconds: isRunning() ? Math.floor((Date.now() - playerStartedAt) / 1000) : 0,
    lastExit,
    settings: currentSettings,
  };
}

function sendJson(res, status, body) {
  const data = JSON.stringify(body);
  res.writeHead(status, {
    "Content-Type": "application/json; charset=utf-8",
    "Content-Length": Buffer.byteLength(data),
    "Cache-Control": "no-store",
  });
  res.end(data);
}

function readJson(req) {
  return new Promise((resolve, reject) => {
    let body = "";
    req.on("data", chunk => {
      body += chunk.toString();
      if (body.length > 1024 * 1024) reject(new Error("request too large"));
    });
    req.on("end", () => {
      if (!body) return resolve({});
      try {
        resolve(JSON.parse(body));
      } catch (error) {
        reject(error);
      }
    });
  });
}

function contentType(filePath) {
  if (filePath.endsWith(".html")) return "text/html; charset=utf-8";
  if (filePath.endsWith(".css")) return "text/css; charset=utf-8";
  if (filePath.endsWith(".js")) return "application/javascript; charset=utf-8";
  return "application/octet-stream";
}

function serveFile(res, filePath) {
  fs.readFile(filePath, (error, data) => {
    if (error) {
      res.writeHead(404, { "Content-Type": "text/plain; charset=utf-8" });
      res.end("Not found");
      return;
    }
    res.writeHead(200, { "Content-Type": contentType(filePath), "Cache-Control": "no-store" });
    res.end(data);
  });
}

async function requestHandler(req, res) {
  const url = new URL(req.url, "http://127.0.0.1");
  try {
    if (req.method === "GET" && url.pathname === "/") {
      serveFile(res, path.join(webDir, "player-control.html"));
      return;
    }
    if (req.method === "GET" && url.pathname === "/api/state") {
      sendJson(res, 200, state());
      return;
    }
    if (req.method === "GET" && url.pathname === "/api/patterns") {
      sendJson(res, 200, { patterns: patternCatalog });
      return;
    }
    if (req.method === "GET" && url.pathname === "/api/esp-status") {
      const status = await fetchJson(`http://${currentSettings.host}/api/status`, 2500);
      sendJson(res, 200, status);
      return;
    }
    if (req.method === "GET" && url.pathname === "/api/log") {
      const logPath = path.join(reportsDir, "player-control-server.log");
      const data = fs.existsSync(logPath) ? fs.readFileSync(logPath, "utf8") : "";
      sendJson(res, 200, { log: data.slice(-12000) });
      return;
    }
    if (req.method === "POST" && url.pathname === "/api/start") {
      sendJson(res, 200, startPlayer(await readJson(req)));
      return;
    }
    if (req.method === "POST" && url.pathname === "/api/stop") {
      sendJson(res, 200, stopPlayer());
      return;
    }
    if (req.method === "POST" && url.pathname === "/api/settings") {
      currentSettings = sanitizeSettings(await readJson(req));
      saveSettings(currentSettings);
      sendLiveSettings();
      sendJson(res, 200, state());
      return;
    }
    if (req.method === "POST" && url.pathname === "/api/apply") {
      sendJson(res, 200, applySettings(await readJson(req)));
      return;
    }
    const staticPath = path.normalize(path.join(webDir, url.pathname.replace(/^\/+/, "")));
    if (staticPath.startsWith(webDir)) {
      serveFile(res, staticPath);
      return;
    }
    sendJson(res, 404, { error: "not found" });
  } catch (error) {
    sendJson(res, 500, { error: error.message });
  }
}

function fetchJson(target, timeoutMs) {
  return new Promise((resolve, reject) => {
    const req = http.get(target, response => {
      let body = "";
      response.on("data", chunk => { body += chunk.toString(); });
      response.on("end", () => {
        try {
          resolve({ ok: response.statusCode >= 200 && response.statusCode < 300, statusCode: response.statusCode, data: JSON.parse(body) });
        } catch (error) {
          reject(error);
        }
      });
    });
    req.setTimeout(timeoutMs, () => {
      req.destroy(new Error("ESP status timeout"));
    });
    req.on("error", reject);
  });
}

function runSelfTest() {
  const settings = sanitizeSettings({ brightness: "2", fps: "60", speed: "0.35", mode: "schrankwand_onlyu", portPixels: [0], portUniverses: [0], playlist: ["rotating_blob", "bogus"] });
  if (settings.brightness !== 1 || settings.fps !== 60) throw new Error("settings clamp failed");
  if (settings.playlist.length !== 1 || settings.playlist[0] !== "rotating_blob") throw new Error("playlist sanitize failed");
  if (!playerArgs(settings).includes("--loop")) throw new Error("loop arg missing");
  if (!playerArgs(settings).includes("--playlist")) throw new Error("playlist arg missing");
  if (settings.mode !== "schrankwand_onlyu") throw new Error("schrankwand mode sanitize failed");
  if (isSchrankwandProfile && settings.portPixels.join(",") !== "509,272,402") throw new Error("port pixel fallback failed");
  if (isSchrankwandProfile && settings.portUniverses.join(",") !== "3,2,3") throw new Error("port universe fallback failed");
  console.log("player-control-server self-test ok");
}

if (process.argv.includes("--self-test")) {
  runSelfTest();
} else {
  const port = Number(process.argv[process.argv.indexOf("--port") + 1]) || defaultPort;
  const server = http.createServer(requestHandler);
  server.listen(port, "127.0.0.1", () => {
    console.log(`Schrank LED Player GUI: http://127.0.0.1:${port}`);
  });
  process.on("SIGINT", () => {
    stopPlayer();
    process.exit(0);
  });
}
