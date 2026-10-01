function universesForPixels(pixelCount) {
  return Math.ceil(pixelCount / 170);
}

function ws2812FrameTimeUs(pixelCount, resetTimeUs = 300) {
  return pixelCount * 30 + resetTimeUs;
}

function apa102FrameTimeUs(pixelCount, spiHz) {
  const totalBits = 32 + pixelCount * 32 + Math.ceil(pixelCount / 2);
  return (totalBits / spiHz) * 1000000;
}

function estimateOutput(output) {
  const frameTimeUs = output.type === "WS2812B"
    ? ws2812FrameTimeUs(output.pixelCount)
    : apa102FrameTimeUs(output.pixelCount, output.spiHz || 4000000);
  const theoreticalFps = frameTimeUs > 0 ? 1000000 / frameTimeUs : 0;
  const universeCount = universesForPixels(output.pixelCount);
  return {
    ...output,
    frameTimeUs,
    theoreticalFps,
    universeCount,
    warning: theoreticalFps < (output.targetFps || 60),
    dataRateMbps: ((18 + 512) * universeCount * (output.targetFps || 60) * 8) / 1000000
  };
}

function outputUniverseRange(output) {
  const first = output.startUniverse || 0;
  return [first, first + universesForPixels(output.pixelCount) - 1];
}

if (typeof module !== "undefined") {
  module.exports = {
    universesForPixels,
    ws2812FrameTimeUs,
    apa102FrameTimeUs,
    estimateOutput,
    outputUniverseRange
  };
}
