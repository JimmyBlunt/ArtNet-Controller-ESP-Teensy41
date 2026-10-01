const fs = require("fs");

function writePlayback(filePath, frames) {
  const normalized = frames.map(frame => ({
    frameId: frame.frameId,
    timestampUs: frame.timestampUs,
    mappingId: frame.mappingId || "unknown",
    pixels: frame.pixels
  }));
  fs.writeFileSync(filePath, JSON.stringify({ version: 1, frames: normalized }, null, 2));
}

function readPlayback(filePath) {
  const playback = JSON.parse(fs.readFileSync(filePath, "utf8"));
  if (playback.version !== 1) throw new Error("unsupported playback version");
  if (!Array.isArray(playback.frames)) throw new Error("frames must be an array");
  playback.frames.forEach((frame, index) => {
    if (!Number.isInteger(frame.frameId)) throw new Error(`frame ${index} missing frameId`);
    if (!Number.isFinite(frame.timestampUs)) throw new Error(`frame ${index} missing timestampUs`);
    if (!Array.isArray(frame.pixels)) throw new Error(`frame ${index} missing pixels`);
  });
  return playback;
}

class HotReloadFile {
  constructor(filePath, { stat = fs.statSync, load }) {
    this.filePath = filePath;
    this.stat = stat;
    this.load = load;
    this.lastMtimeMs = null;
    this.value = null;
  }

  poll() {
    const mtimeMs = this.stat(this.filePath).mtimeMs;
    if (this.lastMtimeMs === mtimeMs) return null;
    this.lastMtimeMs = mtimeMs;
    this.value = this.load(this.filePath);
    return this.value;
  }
}

module.exports = { writePlayback, readPlayback, HotReloadFile };
