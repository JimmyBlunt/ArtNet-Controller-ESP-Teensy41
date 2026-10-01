#pragma once

#include "ConfigManager.h"
#include "LogicalPixelBuffer.h"

namespace led {

class HardwareTest {
 public:
  void begin(const ControllerConfig* config);
  void start(uint32_t nowMs, int outputId = -1);
  void startLoop(uint32_t nowMs, int outputId = -1);
  void stop(LogicalPixelBuffer* frame = nullptr);
  void blackout();
  bool active() const;
  bool update(uint32_t nowMs, LogicalPixelBuffer* frame);
  std::string statusJson() const;

 private:
  void setPhase(const char* phase);
  void logEvent(const char* event) const;
  void fillRange(LogicalPixelBuffer* frame, uint32_t start, uint32_t count, Rgb color,
                 uint32_t every, uint32_t phase);
  void fillOutputs(LogicalPixelBuffer* frame, OutputType type, Rgb color,
                   uint32_t every, uint32_t phase);
  const OutputConfig* outputById(int id) const;

  const ControllerConfig* config_ = nullptr;
  bool active_ = false;
  bool loop_ = false;
  bool blackout_ = false;
  bool clearPending_ = false;
  int outputId_ = -1;
  uint32_t startMs_ = 0;
  uint32_t lastFrameMs_ = 0;
  uint32_t frameIndex_ = 0;
  const char* phaseName_ = "idle";
};

}  // namespace led
