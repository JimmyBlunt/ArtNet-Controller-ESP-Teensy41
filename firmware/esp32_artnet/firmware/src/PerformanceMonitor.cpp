#include "PerformanceMonitor.h"

namespace led {

void PerformanceMonitor::begin(uint32_t nowMs) { windowStartMs_ = nowMs; }

void PerformanceMonitor::markPacket(uint32_t) { packetsInWindow_++; }

void PerformanceMonitor::markFrameStart(uint32_t nowUs) { frameStartUs_ = nowUs; }

void PerformanceMonitor::markOutputDone(uint32_t nowUs) {
  if (frameStartUs_ != 0) {
    snapshot_.frameTimeUs = nowUs - frameStartUs_;
    snapshot_.outputTimeUs = snapshot_.frameTimeUs;
  }
  framesInWindow_++;
}

void PerformanceMonitor::update(uint32_t nowMs, uint32_t heapFree, uint32_t heapMinFree) {
  const uint32_t elapsed = nowMs - windowStartMs_;
  snapshot_.heapFree = heapFree;
  snapshot_.heapMinFree = heapMinFree;
  if (elapsed >= 1000) {
    snapshot_.fps = (framesInWindow_ * 1000u) / elapsed;
    snapshot_.packetsPerSecond = (packetsInWindow_ * 1000u) / elapsed;
    framesInWindow_ = 0;
    packetsInWindow_ = 0;
    windowStartMs_ = nowMs;
  }
}

const PerformanceSnapshot& PerformanceMonitor::snapshot() const { return snapshot_; }

}  // namespace led
