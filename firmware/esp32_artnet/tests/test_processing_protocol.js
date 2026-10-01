const assert = require("assert");
const { encodePreviewFrame, decodePreviewChunk, FrameAssembler } = require("../processing/src/previewProtocol");

const pixels = Array.from({ length: 1000 }, (_, i) => [i % 256, (i * 2) % 256, (i * 3) % 256]);
const chunks = encodePreviewFrame({ frameId: 7, pixels, maxPixelsPerChunk: 333 });
assert.strictEqual(chunks.length, 4);

const assembler = new FrameAssembler();
assert.strictEqual(assembler.accept(decodePreviewChunk(chunks[0])), null);
assert.strictEqual(assembler.accept(decodePreviewChunk(chunks[2])), null);
assert.strictEqual(assembler.accept(decodePreviewChunk(chunks[1])), null);
const frame = assembler.accept(decodePreviewChunk(chunks[3]));
assert(frame);
assert.strictEqual(frame.pixels.length, 1000);
assert.deepStrictEqual(frame.pixels[999], pixels[999]);

assert.throws(() => decodePreviewChunk(Buffer.from("BAD")));
console.log("Processing preview protocol tests passed");
