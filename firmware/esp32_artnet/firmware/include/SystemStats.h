#pragma once

#include "UniverseAssembler.h"

namespace led {

struct SystemStats {
  AssemblerStats artnet;
  uint32_t rxPackets = 0;
  uint32_t rxQueueDrops = 0;
  uint32_t rxOversizedPackets = 0;
  uint32_t rxSocketErrors = 0;
  uint64_t previewFrames = 0;
  uint64_t outputFrames = 0;
  uint32_t heapFree = 0;
  uint32_t heapMinFree = 0;
  double lastFrameLatencyMs = 0.0;
};

}  // namespace led
