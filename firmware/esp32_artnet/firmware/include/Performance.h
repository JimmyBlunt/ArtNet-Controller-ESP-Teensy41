#pragma once

#include "Types.h"

namespace led {

double ws2812FrameTimeUs(uint32_t pixels, double resetTimeUs = 300.0);
double apa102FrameTimeUs(uint32_t pixels, uint32_t spiHz);
uint16_t universesForPixels(uint32_t pixels);
double artNetDataRateMbps(uint32_t pixels, uint16_t fps);
OutputEstimate estimateOutput(const OutputConfig& output);

}  // namespace led
