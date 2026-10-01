const assert = require("assert");
const { artDmxPacket, derive, run } = require("../tools/hardware-benchmark");

const packet = artDmxPacket(7, 3, 2);
assert.strictEqual(packet.toString("ascii", 0, 8), "Art-Net\u0000");
assert.strictEqual(packet.readUInt16LE(8), 0x5000);
assert.strictEqual(packet.readUInt16LE(14), 7);
assert.strictEqual(packet.readUInt16BE(16), 512);

assert.deepStrictEqual(
  derive({ packets: 10, framesComplete: 2, framesIncomplete: 1 }, { packets: 37, framesComplete: 3, framesIncomplete: 1 }, { sentPackets: 30, sentFrames: 1 }),
  { receivedPackets: 27, packetLossEstimated: 3, framesCompleteDelta: 1, framesIncompleteDelta: 0, completionRatio: 1 }
);

run({
  dryRun: true,
  host: "127.0.0.1",
  scenario: "mixed_4500_30",
  durationSeconds: 2,
  settleMs: 0
}).then(result => {
  assert.strictEqual(result.measured, false);
  assert.strictEqual(result.plannedPackets, 1620);
  console.log("Hardware benchmark tool tests passed");
}).catch(error => {
  console.error(error);
  process.exit(1);
});
