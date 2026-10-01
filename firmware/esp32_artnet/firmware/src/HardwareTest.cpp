#include "HardwareTest.h"

#include <cstring>
#include <sstream>

#ifdef ARDUINO
#include <Arduino.h>
#endif

namespace led {

namespace {
constexpr uint8_t kLow = 18;
constexpr uint8_t kRedDiagnostic = 36;
constexpr uint32_t kFrameIntervalMs = 50;
constexpr uint32_t kCycleMs = 20000;
}

void HardwareTest::begin(const ControllerConfig* config) { config_ = config; }

void HardwareTest::logEvent(const char* event) const {
#ifdef ARDUINO
  Serial.print("[hardware-test] ");
  Serial.println(event);
#else
  (void)event;
#endif
}

void HardwareTest::setPhase(const char* phase) {
  if (std::strcmp(phaseName_, phase) == 0) return;
  phaseName_ = phase;
  logEvent(phaseName_);
}

void HardwareTest::start(uint32_t nowMs, int outputId) {
  outputId_ = outputId;
  blackout_ = false;
  clearPending_ = false;
  active_ = true;
  loop_ = false;
  startMs_ = nowMs;
  lastFrameMs_ = 0;
  frameIndex_ = 0;
  setPhase("starting");
  logEvent("single sequence started");
}

void HardwareTest::startLoop(uint32_t nowMs, int outputId) {
  outputId_ = outputId;
  blackout_ = false;
  clearPending_ = false;
  active_ = true;
  loop_ = true;
  startMs_ = nowMs;
  lastFrameMs_ = 0;
  frameIndex_ = 0;
  setPhase("loop-starting");
  logEvent("loop started");
}

void HardwareTest::stop(LogicalPixelBuffer* frame) {
  active_ = false;
  loop_ = false;
  blackout_ = false;
  clearPending_ = true;
  setPhase("stopped");
  logEvent("stopped");
  if (frame != nullptr) frame->clear();
}

void HardwareTest::blackout() {
  stop();
  blackout_ = true;
  setPhase("blackout-held");
}

bool HardwareTest::active() const { return active_ || blackout_ || clearPending_; }

const OutputConfig* HardwareTest::outputById(int id) const {
  if (config_ == nullptr) return nullptr;
  for (const auto& output : config_->outputs) {
    if (output.enabled && output.id == id && (outputId_ < 0 || outputId_ == id)) return &output;
  }
  return nullptr;
}

void HardwareTest::fillRange(LogicalPixelBuffer* frame, uint32_t start, uint32_t count,
                             Rgb color, uint32_t every, uint32_t phase) {
  if (frame == nullptr || every == 0) return;
  const uint32_t end = start + count;
  for (uint32_t i = start; i < end && i < frame->size(); ++i) {
    if (((i - start + phase) % every) < 4) {
      frame->set(i, color);
    }
  }
}

void HardwareTest::fillOutputs(LogicalPixelBuffer* frame, OutputType type, Rgb color,
                               uint32_t every, uint32_t phase) {
  if (config_ == nullptr) return;
  for (const auto& output : config_->outputs) {
    if (!output.enabled || output.type != type ||
        (outputId_ >= 0 && output.id != outputId_)) continue;
    fillRange(frame, output.startPixel, output.pixelCount, color, every, phase);
  }
}

bool HardwareTest::update(uint32_t nowMs, LogicalPixelBuffer* frame) {
  if (frame != nullptr && clearPending_) {
    frame->clear();
    clearPending_ = false;
    return true;
  }
  if (!active_ || frame == nullptr || config_ == nullptr) return false;
  if (lastFrameMs_ != 0 && nowMs - lastFrameMs_ < kFrameIntervalMs) return false;
  lastFrameMs_ = nowMs;
  frameIndex_++;
  frame->clear();

  uint32_t elapsed = nowMs >= startMs_ ? nowMs - startMs_ : 0;
  if (loop_) elapsed %= kCycleMs;
  if (elapsed < 1000) {
    setPhase("blackout");
  } else if (elapsed < 7000) {
    setPhase("all-red-chase");
    fillOutputs(frame, OutputType::WS2812B, Rgb{kRedDiagnostic, 0, 0}, 32, frameIndex_);
    fillOutputs(frame, OutputType::APA102, Rgb{kRedDiagnostic, 0, 0}, 40, frameIndex_);
  } else if (elapsed < 13000) {
    setPhase("all-green-chase");
    fillOutputs(frame, OutputType::WS2812B, Rgb{0, kLow, 0}, 32, frameIndex_);
    fillOutputs(frame, OutputType::APA102, Rgb{0, kLow, 0}, 40, frameIndex_);
  } else if (elapsed < 19000) {
    setPhase("all-blue-chase");
    fillOutputs(frame, OutputType::WS2812B, Rgb{0, 0, kLow}, 32, frameIndex_);
    fillOutputs(frame, OutputType::APA102, Rgb{0, 0, kLow}, 40, frameIndex_);
  } else if (elapsed < 20000) {
    setPhase("final-blackout");
  } else {
    stop(frame);
  }

  return true;
}

std::string HardwareTest::statusJson() const {
  std::ostringstream out;
  out << "{\"active\":" << (active_ ? "true" : "false") << ",\"phase\":\""
      << phaseName_ << "\",\"loop\":" << (loop_ ? "true" : "false")
      << ",\"blackout\":" << (blackout_ ? "true" : "false")
      << ",\"outputId\":" << outputId_ << "}";
  return out.str();
}

}  // namespace led
