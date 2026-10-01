#pragma once

#include "UniverseAssembler.h"

namespace led {

struct SystemStats {
  AssemblerStats artnet;
  uint64_t previewFrames = 0;
  uint64_t outputFrames = 0;
  uint32_t heapFree = 0;
  uint32_t heapMinFree = 0;
  double lastFrameLatencyMs = 0.0;
};

}  // namespace led
