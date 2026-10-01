const assert = require("assert");
const calc = require("../web/calculations");

assert.strictEqual(calc.universesForPixels(170), 1);
assert.strictEqual(calc.universesForPixels(171), 2);
assert.strictEqual(calc.ws2812FrameTimeUs(500), 15300);

const apa = calc.estimateOutput({ id: 1, type: "APA102", pixelCount: 500, spiHz: 4000000, targetFps: 60 });
assert(apa.theoreticalFps > 200);
assert.strictEqual(apa.warning, false);

const ws = calc.estimateOutput({ id: 2, type: "WS2812B", pixelCount: 750, targetFps: 60 });
assert.strictEqual(ws.warning, true);

console.log("Web calculation tests passed");
