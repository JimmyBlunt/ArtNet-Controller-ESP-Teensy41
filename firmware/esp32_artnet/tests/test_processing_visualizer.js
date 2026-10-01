const assert = require("assert");
const path = require("path");
const { artDmxPacket, parseArtDmx, DirectArtNetAssembler } = require("../processing/src/artnet");
const { loadMapping, normalize } = require("../processing/src/mappingLoader");
const { writePlayback, readPlayback, HotReloadFile } = require("../processing/src/playback");
const { VisualizerStats } = require("../processing/src/statistics");

const packet = artDmxPacket({ universe: 4, sequence: 9, data: Buffer.from([1, 2, 3, 4, 5, 6]) });
const parsed = parseArtDmx(packet);
assert.strictEqual(parsed.universe, 4);
assert.strictEqual(parsed.sequence, 9);
assert.deepStrictEqual(Array.from(parsed.data), [1, 2, 3, 4, 5, 6]);

const assembler = new DirectArtNetAssembler({ startUniverse: 0, universeCount: 2, pixelCount: 3 });
assert.strictEqual(assembler.accept(artDmxPacket({ universe: 0, data: Buffer.from([1, 2, 3]) })), null);
const frame = assembler.accept(artDmxPacket({ universe: 1, data: Buffer.from([4, 5, 6, 7, 8, 9]) }));
assert.deepStrictEqual(frame.pixels, [[1, 2, 3], [4, 5, 6], [7, 8, 9]]);

const mapping = loadMapping(path.join("config", "examples", "zigzag-matrix-8x8.json"));
assert.strictEqual(mapping.pixelCount, 64);
assert.strictEqual(mapping.points[8][0], 1);
const contained = normalize([[10, 0, 0], [20, 10, 0]], "contain");
assert(contained[1][0] <= 1 && contained[1][1] <= 1);

let now = 0;
const stats = new VisualizerStats({ now: () => now });
stats.recordPacket(1000);
stats.recordFrame(1);
stats.recordFrame(3);
stats.recordIncomplete();
stats.recordRendered();
now = 1000;
const snapshot = stats.snapshot();
assert.strictEqual(snapshot.lostFrames, 1);
assert.strictEqual(snapshot.incompleteFrames, 1);
assert(snapshot.throughputMbps > 0);

const playbackPath = path.join("build", "processing-playback-test.json");
writePlayback(playbackPath, [
  { frameId: 1, timestampUs: 1000, mappingId: "test", pixels: [[1, 2, 3]] },
  { frameId: 2, timestampUs: 2000, mappingId: "test", pixels: [[4, 5, 6]] }
]);
const playback = readPlayback(playbackPath);
assert.strictEqual(playback.frames.length, 2);
assert.deepStrictEqual(playback.frames[1].pixels[0], [4, 5, 6]);

let mtimeMs = 1;
let loads = 0;
const hotReload = new HotReloadFile("mapping.json", {
  stat: () => ({ mtimeMs }),
  load: filePath => ({ filePath, loads: ++loads })
});
assert.strictEqual(hotReload.poll().loads, 1);
assert.strictEqual(hotReload.poll(), null);
mtimeMs = 2;
assert.strictEqual(hotReload.poll().loads, 2);

console.log("Processing visualizer tests passed");
