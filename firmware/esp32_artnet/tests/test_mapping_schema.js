const assert = require("assert");
const {
  exportMariMapperCsv,
  exportPixelblazeArray,
  importMariMapperCsv,
  importPixelblaze,
  importPixelblazeArray,
  loadMapping,
  normalize,
  validateMapping
} = require("../tools/mapping-utils");

const linear = loadMapping("config/examples/linear-10.json");
assert.deepStrictEqual(validateMapping(linear), []);

const invalidCount = { ...linear, pixelCount: 11 };
assert(validateMapping(invalidCount).includes("points length must equal pixelCount"));

const invalidNan = { ...linear, points: [[NaN], ...linear.points.slice(1)] };
assert(validateMapping(invalidNan).some(e => e.includes("invalid coordinates")));

const invalidDim = { ...linear, dimensions: 2, points: [[0], [1]] };
assert(validateMapping(invalidDim).some(e => e.includes("misses required dimension")));

const contained = normalize([[10, 0, 0], [20, 10, 0]], "contain");
assert(contained[1][0] <= 1 && contained[1][1] <= 1);

const imported = importPixelblazeArray("[[0,0,0],[1,0,0],[1,1,0]]");
assert.strictEqual(imported.pixelCount, 3);
assert.deepStrictEqual(validateMapping(imported), []);

const importedWithComments = importPixelblazeArray(`
  [
    [0, 0, 0], // first LED
    [1, 0, 0],
    [1, 1, 0],
  ]
`, "pixelblaze-comments");
assert.strictEqual(importedWithComments.id, "pixelblaze-comments");
assert.strictEqual(importedWithComments.pixelCount, 3);
assert.deepStrictEqual(validateMapping(importedWithComments), []);

const preservedGenerator = importPixelblaze(`
  export function beforeRender(delta) {}
  export function render2D(index, x, y) { hsv(x, 1, y) }
`, "pb-generator");
assert.strictEqual(preservedGenerator.kind, "generator");
assert.strictEqual(preservedGenerator.generator.executable, false);
assert(preservedGenerator.generator.generatorSource.includes("render2D"));

const mariMapper = importMariMapperCsv(`index,tx,ty,tz,fixture
0,0,0,0,left
2,1,0,0,right
`, "mari-gap");
assert.strictEqual(mariMapper.pixelCount, 3);
assert.deepStrictEqual(mariMapper.points[1], [0, 0, 0]);
assert.deepStrictEqual(mariMapper.compatibility.missingIndices, [1]);
assert.deepStrictEqual(mariMapper.compatibility.metadataFields, ["fixture"]);
assert.strictEqual(mariMapper.rowMetadata[2].fixture, "right");
assert.deepStrictEqual(validateMapping(mariMapper), []);

const mariMapperXyz = importMariMapperCsv(`index,x,y,z
0,0.1,0.2,0.3
1,0.4,0.5,0.6
`);
assert.strictEqual(mariMapperXyz.dimensions, 3);
assert.deepStrictEqual(mariMapperXyz.points[1], [0.4, 0.5, 0.6]);

assert.throws(() => importMariMapperCsv(`index,x,y,z
0,0,0,0
0,1,0,0
`), /duplicate index/);
assert.throws(() => importMariMapperCsv(`index,x,y,z
-1,0,0,0
`), /invalid index/);

const pixelblazeExport = exportPixelblazeArray(importedWithComments);
assert(JSON.parse(pixelblazeExport).length === 3);

const csvExport = exportMariMapperCsv(mariMapper);
assert(csvExport.startsWith("index,x,y,z,fixture"));
assert(csvExport.includes("2,1,0,0,right"));

console.log("Mapping schema tests passed");
