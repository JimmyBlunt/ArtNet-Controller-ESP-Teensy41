const ARTNET_ID = "Art-Net\0";
const OP_OUTPUT = 0x5000;

function parseArtDmx(buffer) {
  if (!Buffer.isBuffer(buffer)) buffer = Buffer.from(buffer);
  if (buffer.length < 18 || buffer.toString("ascii", 0, 8) !== ARTNET_ID) {
    throw new Error("invalid Art-Net packet");
  }
  const opcode = buffer.readUInt16LE(8);
  if (opcode !== OP_OUTPUT) throw new Error("unsupported Art-Net opcode");
  const sequence = buffer[12];
  const universe = buffer.readUInt16LE(14);
  const length = buffer.readUInt16BE(16);
  if (length <= 0 || length > 512 || buffer.length < 18 + length) {
    throw new Error("invalid ArtDMX length");
  }
  return {
    sequence,
    universe,
    data: buffer.subarray(18, 18 + length)
  };
}

function artDmxPacket({ universe, sequence = 1, data }) {
  const payload = Buffer.from(data);
  const packet = Buffer.alloc(18 + payload.length);
  packet.write(ARTNET_ID, 0, "ascii");
  packet.writeUInt16LE(OP_OUTPUT, 8);
  packet.writeUInt16BE(14, 10);
  packet[12] = sequence;
  packet.writeUInt16LE(universe, 14);
  packet.writeUInt16BE(payload.length, 16);
  payload.copy(packet, 18);
  return packet;
}

class DirectArtNetAssembler {
  constructor({ startUniverse = 0, universeCount, pixelCount }) {
    this.startUniverse = startUniverse;
    this.universeCount = universeCount;
    this.pixelCount = pixelCount;
    this.universes = new Map();
    this.completeFrames = 0;
    this.incompleteFrames = 0;
  }

  accept(packet) {
    const parsed = Buffer.isBuffer(packet) ? parseArtDmx(packet) : packet;
    if (parsed.universe < this.startUniverse || parsed.universe >= this.startUniverse + this.universeCount) {
      return null;
    }
    this.universes.set(parsed.universe, parsed.data);
    if (this.universes.size !== this.universeCount) return null;

    const bytes = Buffer.concat(
      Array.from({ length: this.universeCount }, (_, i) => this.universes.get(this.startUniverse + i) || Buffer.alloc(0))
    );
    this.universes.clear();
    this.completeFrames++;
    const pixels = [];
    for (let offset = 0; offset + 2 < bytes.length && pixels.length < this.pixelCount; offset += 3) {
      pixels.push([bytes[offset], bytes[offset + 1], bytes[offset + 2]]);
    }
    return { pixels, frameId: this.completeFrames };
  }
}

module.exports = { parseArtDmx, artDmxPacket, DirectArtNetAssembler };
