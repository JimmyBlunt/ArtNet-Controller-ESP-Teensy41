const MAGIC = "SLPV";
const HEADER_BYTES = 25;

function encodePreviewFrame({ frameId, timestampUs = Date.now() * 1000, mappingHash = 0, pixels, maxPixelsPerChunk = 480 }) {
  const chunks = [];
  const chunkCount = Math.ceil(pixels.length / maxPixelsPerChunk);
  for (let chunkIndex = 0; chunkIndex < chunkCount; chunkIndex++) {
    const first = chunkIndex * maxPixelsPerChunk;
    const part = pixels.slice(first, first + maxPixelsPerChunk);
    const buffer = Buffer.alloc(HEADER_BYTES + part.length * 3);
    buffer.write(MAGIC, 0, "ascii");
    buffer.writeUInt8(1, 4);
    buffer.writeUInt32LE(frameId, 5);
    buffer.writeUInt32LE(Math.floor(timestampUs % 0x100000000), 9);
    buffer.writeUInt32LE(pixels.length, 13);
    buffer.writeUInt32LE((chunkIndex << 16) | chunkCount, 17);
    buffer.writeUInt32LE(mappingHash, 21);
    part.forEach((p, i) => {
      buffer[HEADER_BYTES + i * 3] = p[0];
      buffer[HEADER_BYTES + i * 3 + 1] = p[1];
      buffer[HEADER_BYTES + i * 3 + 2] = p[2];
    });
    chunks.push(buffer);
  }
  return chunks;
}

function decodePreviewChunk(buffer) {
  if (buffer.length < HEADER_BYTES || buffer.toString("ascii", 0, 4) !== MAGIC) {
    throw new Error("invalid preview packet magic");
  }
  const pixels = [];
  for (let offset = HEADER_BYTES; offset + 2 < buffer.length; offset += 3) {
    pixels.push([buffer[offset], buffer[offset + 1], buffer[offset + 2]]);
  }
  return {
    version: buffer.readUInt8(4),
    frameId: buffer.readUInt32LE(5),
    timestampUs: buffer.readUInt32LE(9),
    pixelCount: buffer.readUInt32LE(13),
    chunkIndex: buffer.readUInt32LE(17) >>> 16,
    chunkCount: buffer.readUInt32LE(17) & 0xffff,
    mappingHash: buffer.readUInt32LE(21),
    pixels
  };
}

class FrameAssembler {
  constructor() {
    this.frames = new Map();
  }

  accept(chunk) {
    const key = `${chunk.frameId}:${chunk.mappingHash}`;
    if (!this.frames.has(key)) {
      this.frames.set(key, { chunkCount: chunk.chunkCount, chunks: new Map(), pixelCount: chunk.pixelCount });
    }
    const state = this.frames.get(key);
    state.chunks.set(chunk.chunkIndex, chunk.pixels);
    if (state.chunks.size !== state.chunkCount) return null;
    const pixels = [];
    for (let i = 0; i < state.chunkCount; i++) {
      if (!state.chunks.has(i)) return null;
      pixels.push(...state.chunks.get(i));
    }
    this.frames.delete(key);
    return { frameId: chunk.frameId, mappingHash: chunk.mappingHash, pixels: pixels.slice(0, state.pixelCount) };
  }
}

module.exports = { encodePreviewFrame, decodePreviewChunk, FrameAssembler };
