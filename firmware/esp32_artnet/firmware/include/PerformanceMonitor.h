#pragma once

#include <cstdint>

namespace led {

struct PerformanceSnapshot {
  uint32_t fps = 0;
  uint32_t frameTimeUs = 0;
  uint32_t outputTimeUs = 0;
  uint32_t packetsPerSecond = 0;
  uint32_t heapFree = 0;
  uint32_t heapMinFree = 0;
};

class PerformanceMonitor {
 public:
  void begin(uint32_t nowMs);
  void markPacket(uint32_t nowMs);
  void markFrameStart(uint32_t nowUs);
  void markOutputDone(uint32_t nowUs);
  void update(uint32_t nowMs, uint32_t heapFree = 0, uint32_t heapMinFree = 0);
  const PerformanceSnapshot& snapshot() const;

 private:
  uint32_t windowStartMs_ = 0;
  uint32_t packetsInWindow_ = 0;
  uint32_t framesInWindow_ = 0;
  uint32_t frameStartUs_ = 0;
  PerformanceSnapshot snapshot_;
};

}  // namespace led
