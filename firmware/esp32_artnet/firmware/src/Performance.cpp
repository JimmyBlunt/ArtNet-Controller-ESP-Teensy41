#include "Performance.h"

#include <algorithm>
#include <cmath>

namespace led {

double ws2812FrameTimeUs(uint32_t pixels, double resetTimeUs) {
  return static_cast<double>(pixels) * 30.0 + resetTimeUs;
}

double apa102FrameTimeUs(uint32_t pixels, uint32_t spiHz) {
  if (spiHz == 0) return 0.0;
  const double startBits = 32.0;
  const double ledBits = static_cast<double>(pixels) * 32.0;
  const double endBits = std::ceil(static_cast<double>(pixels) / 2.0);
  return ((startBits + ledBits + endBits) / static_cast<double>(spiHz)) * 1000000.0;
}

uint16_t universesForPixels(uint32_t pixels) {
  return static_cast<uint16_t>((pixels + 169) / 170);
}

double artNetDataRateMbps(uint32_t pixels, uint16_t fps) {
  const uint32_t universes = universesForPixels(pixels);
  const double bytesPerPacket = 18.0 + 512.0;
  return (bytesPerPacket * universes * fps * 8.0) / 1000000.0;
}

OutputEstimate estimateOutput(const OutputConfig& output) {
  OutputEstimate estimate;
  estimate.outputId = output.id;
  estimate.firstUniverse = output.startUniverse;
  estimate.lastUniverse = output.startUniverse + universesForPixels(output.pixelCount) - 1;
  if (output.type == OutputType::WS2812B) {
    estimate.frameTimeUs = ws2812FrameTimeUs(output.pixelCount);
    estimate.chainTooLongForTarget =
        estimate.frameTimeUs > (1000000.0 / std::max<uint16_t>(1, output.targetFps));
  } else {
    estimate.frameTimeUs = apa102FrameTimeUs(output.pixelCount, output.spiHz);
    estimate.chainTooLongForTarget =
        output.spiHz == 0 ||
        estimate.frameTimeUs > (1000000.0 / std::max<uint16_t>(1, output.targetFps));
  }
  estimate.theoreticalFps = estimate.frameTimeUs > 0.0 ? 1000000.0 / estimate.frameTimeUs : 0.0;
  estimate.dataRateMbps = artNetDataRateMbps(output.pixelCount, output.targetFps);
  return estimate;
}

}  // namespace led
