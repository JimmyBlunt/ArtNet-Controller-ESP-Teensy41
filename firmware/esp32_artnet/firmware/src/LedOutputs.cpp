#include "LedOutputs.h"
#include "LedColorOrder.h"

#ifdef ARDUINO
#include <Arduino.h>
#include <FastLED.h>
#include <new>
#endif

namespace led {

#ifdef ARDUINO
namespace {
constexpr uint16_t kApa102Output0MaxPixels = 500;
constexpr uint16_t kApa102Output1MaxPixels = 1200;
constexpr uint16_t kWs2812Output0MaxPixels = 1024;
CRGB apa102Output0Leds[kApa102Output0MaxPixels];
CRGB apa102Output1Leds[kApa102Output1MaxPixels];
CRGB ws2812Output0Leds[kWs2812Output0MaxPixels];
#if defined(LED_PROFILE_FLEX8)
constexpr uint16_t kWs8MaxPixels = 1200;
#if defined(LED_PROFILE_EXTENSION_BOARD)
constexpr int kWs8Pins[] = {25, 26, 27, 14, 19, 18, 5, 17};
constexpr int kApa8Pairs[][2] = {{18, 5}, {26, 27}};
#else
constexpr int kWs8Pins[] = {32, 33, 25, 26, 27, 14, 13, 16, 17, 18, 19, 21, 22, 23};
constexpr int kApa8Pairs[][2] = {{18, 19}, {23, 18}, {13, 14}, {25, 26},
                                 {32, 33}, {16, 17}, {21, 22}};
#endif
constexpr size_t kWs8PinCount = sizeof(kWs8Pins) / sizeof(kWs8Pins[0]);
CRGB* ws8Leds[kWs8PinCount] = {};
CLEDController* ws8Controllers[kWs8PinCount] = {};
constexpr size_t kApa8PairCount = sizeof(kApa8Pairs) / sizeof(kApa8Pairs[0]);
CRGB* apa8Leds[kApa8PairCount] = {};
CLEDController* apa8Controllers[kApa8PairCount] = {};

int ws8SlotForPin(int pin) {
  for (size_t i = 0; i < kWs8PinCount; ++i) {
    if (kWs8Pins[i] == pin) return static_cast<int>(i);
  }
  return -1;
}

CLEDController* registerWs8Controller(size_t slot, uint16_t count) {
  if (ws8Leds[slot] == nullptr) ws8Leds[slot] = new (std::nothrow) CRGB[kWs8MaxPixels]();
  if (ws8Leds[slot] == nullptr) return nullptr;
  switch (kWs8Pins[slot]) {
    case 32: return &FastLED.addLeds<WS2812B, 32, GRB>(ws8Leds[slot], count);
    case 33: return &FastLED.addLeds<WS2812B, 33, GRB>(ws8Leds[slot], count);
    case 25: return &FastLED.addLeds<WS2812B, 25, GRB>(ws8Leds[slot], count);
    case 26: return &FastLED.addLeds<WS2812B, 26, GRB>(ws8Leds[slot], count);
    case 27: return &FastLED.addLeds<WS2812B, 27, GRB>(ws8Leds[slot], count);
    case 14: return &FastLED.addLeds<WS2812B, 14, GRB>(ws8Leds[slot], count);
    case 13: return &FastLED.addLeds<WS2812B, 13, GRB>(ws8Leds[slot], count);
    case 16: return &FastLED.addLeds<WS2812B, 16, GRB>(ws8Leds[slot], count);
    case 17: return &FastLED.addLeds<WS2812B, 17, GRB>(ws8Leds[slot], count);
    case 18: return &FastLED.addLeds<WS2812B, 18, GRB>(ws8Leds[slot], count);
    case 19: return &FastLED.addLeds<WS2812B, 19, GRB>(ws8Leds[slot], count);
    case 21: return &FastLED.addLeds<WS2812B, 21, GRB>(ws8Leds[slot], count);
    case 22: return &FastLED.addLeds<WS2812B, 22, GRB>(ws8Leds[slot], count);
    case 23: return &FastLED.addLeds<WS2812B, 23, GRB>(ws8Leds[slot], count);
    default: return nullptr;
  }
}

int apa8SlotForPins(int dataPin, int clockPin) {
  for (size_t i = 0; i < kApa8PairCount; ++i) {
    if (kApa8Pairs[i][0] == dataPin && kApa8Pairs[i][1] == clockPin) return static_cast<int>(i);
  }
  return -1;
}

CLEDController* registerApa8Controller(size_t slot, uint16_t count) {
  if (apa8Leds[slot] == nullptr) apa8Leds[slot] = new (std::nothrow) CRGB[kWs8MaxPixels]();
  if (apa8Leds[slot] == nullptr) return nullptr;
  const int dataPin = kApa8Pairs[slot][0];
  const int clockPin = kApa8Pairs[slot][1];
#if defined(LED_PROFILE_EXTENSION_BOARD)
  if (dataPin == 18 && clockPin == 5)
    return &FastLED.addLeds<APA102, 18, 5, BGR, DATA_RATE_MHZ(4)>(apa8Leds[slot], count);
  if (dataPin == 26 && clockPin == 27)
    return &FastLED.addLeds<APA102, 26, 27, BGR, DATA_RATE_MHZ(4)>(apa8Leds[slot], count);
#else
  if (dataPin == 18 && clockPin == 19)
    return &FastLED.addLeds<APA102, 18, 19, BGR, DATA_RATE_MHZ(4)>(apa8Leds[slot], count);
  if (dataPin == 23 && clockPin == 18)
    return &FastLED.addLeds<APA102, 23, 18, BGR, DATA_RATE_MHZ(4)>(apa8Leds[slot], count);
  if (dataPin == 13 && clockPin == 14)
    return &FastLED.addLeds<APA102, 13, 14, BGR, DATA_RATE_MHZ(4)>(apa8Leds[slot], count);
  if (dataPin == 25 && clockPin == 26)
    return &FastLED.addLeds<APA102, 25, 26, BGR, DATA_RATE_MHZ(4)>(apa8Leds[slot], count);
  if (dataPin == 32 && clockPin == 33)
    return &FastLED.addLeds<APA102, 32, 33, BGR, DATA_RATE_MHZ(4)>(apa8Leds[slot], count);
  if (dataPin == 16 && clockPin == 17)
    return &FastLED.addLeds<APA102, 16, 17, BGR, DATA_RATE_MHZ(4)>(apa8Leds[slot], count);
  if (dataPin == 21 && clockPin == 22)
    return &FastLED.addLeds<APA102, 21, 22, BGR, DATA_RATE_MHZ(4)>(apa8Leds[slot], count);
#endif
  return nullptr;
}
#endif
bool apa102Output0Registered = false;
bool apa102Output1Registered = false;
bool ws2812Output0Registered = false;
CLEDController* apa0 = nullptr;
CLEDController* apa1 = nullptr;
CLEDController* ws0 = nullptr;

void copyPixelsToCrgb(const LogicalPixelBuffer& pixels, const OutputConfig& config, CRGB* target) {
  for (uint32_t i = 0; i < config.pixelCount; ++i) {
    const uint32_t source = config.startPixel + (config.reverse ? config.pixelCount - 1 - i : i);
    const auto color = colorForFixedDriver(pixels.get(source), config.colorOrder, config.type);
    target[i] = CRGB(color.r, color.g, color.b);
  }
}
}
#endif

bool Apa102Output::begin(const OutputConfig& config) {
  config_ = config;
  ready_ = config.enabled && config.type == OutputType::APA102 && config.pixelCount > 0;
#ifdef ARDUINO
  if (!ready_) return false;
  ready_ = false;
  Serial.print("APA102 output configured id=");
  Serial.print(config.id);
  Serial.print(" pixels=");
  Serial.print(config.pixelCount);
  Serial.print(" data=");
  Serial.print(config.dataPin);
  Serial.print(" clock=");
  Serial.println(config.clockPin);
#if defined(LED_PROFILE_FLEX8)
  const int slot = apa8SlotForPins(config.dataPin, config.clockPin);
  if (slot >= 0 && config.pixelCount <= kWs8MaxPixels) {
    if (apa8Controllers[slot] == nullptr) {
      apa8Controllers[slot] = registerApa8Controller(static_cast<size_t>(slot), config.pixelCount);
      FastLED.setBrightness(160);
      Serial.print("APA102 hardware enabled on data/clock=");
      Serial.print(config.dataPin);
      Serial.print("/");
      Serial.println(config.clockPin);
    }
    if (apa8Controllers[slot] != nullptr) {
      apa8Controllers[slot]->setLeds(apa8Leds[slot], config.pixelCount);
      apa8Controllers[slot]->setEnabled(true);
      ready_ = true;
    }
  } else {
    Serial.println("APA102 hardware disabled: unsupported pin pair or more than 1200 pixels");
  }
#else
  if (config.id == 0 && config.dataPin == 23 && config.clockPin == 18 &&
      config.pixelCount <= kApa102Output0MaxPixels) {
    if (!apa102Output0Registered) {
      apa0 = &FastLED.addLeds<APA102, 23, 18, BGR, DATA_RATE_MHZ(4)>(
          apa102Output0Leds, config.pixelCount);
      FastLED.setBrightness(96);
      apa102Output0Registered = true;
      Serial.println("APA102 output 0 hardware enabled on data=23 clock=18");
    }
    apa0->setLeds(apa102Output0Leds, config.pixelCount);
    apa0->setEnabled(true);
    ready_ = true;
  } else if (config.id == 1 && config.dataPin == 18 && config.clockPin == 19 &&
             config.pixelCount <= kApa102Output1MaxPixels) {
    if (!apa102Output1Registered) {
      apa1 = &FastLED.addLeds<APA102, 18, 19, BGR, DATA_RATE_MHZ(4)>(
          apa102Output1Leds, config.pixelCount);
      FastLED.setBrightness(160);
      FastLED.clear(true);
      apa102Output1Registered = true;
      Serial.println("APA102 output 1 hardware enabled on data=18 clock=19");
    }
    apa1->setLeds(apa102Output1Leds, config.pixelCount);
    apa1->setEnabled(true);
    ready_ = true;
  } else if (config.id == 0) {
    Serial.println("APA102 output 0 hardware disabled: expected data=23 clock=18 pixels<=500");
  } else if (config.id == 1) {
    Serial.println("APA102 output 1 hardware disabled: expected data=18 clock=19 pixels<=1200");
  }
#endif
#endif
  return ready_;
}

void Apa102Output::show(const LogicalPixelBuffer& pixels) {
#ifdef ARDUINO
#if defined(LED_PROFILE_FLEX8)
  const int slot = apa8SlotForPins(config_.dataPin, config_.clockPin);
  if (slot >= 0 && apa8Controllers[slot] != nullptr) {
    copyPixelsToCrgb(pixels, config_, apa8Leds[slot]);
  }
#else
  if (config_.id == 0 && apa102Output0Registered) {
    copyPixelsToCrgb(pixels, config_, apa102Output0Leds);
  } else if (config_.id == 1 && apa102Output1Registered) {
    copyPixelsToCrgb(pixels, config_, apa102Output1Leds);
  }
#endif
#else
  (void)pixels;
#endif
}

bool Apa102Output::ready() const { return ready_; }
int Apa102Output::id() const { return config_.id; }

bool Ws2812Output::begin(const OutputConfig& config) {
  config_ = config;
  ready_ = config.enabled && config.type == OutputType::WS2812B && config.pixelCount > 0;
#ifdef ARDUINO
  if (!ready_) return false;
  ready_ = false;
  Serial.print("WS2812B output configured id=");
  Serial.print(config.id);
  Serial.print(" pixels=");
  Serial.print(config.pixelCount);
  Serial.print(" data=");
  Serial.println(config.dataPin);
#if defined(LED_PROFILE_FLEX8)
  const int slot = ws8SlotForPin(config.dataPin);
  if (slot >= 0 && config.pixelCount <= kWs8MaxPixels) {
    if (ws8Controllers[slot] == nullptr) {
      ws8Controllers[slot] = registerWs8Controller(static_cast<size_t>(slot), config.pixelCount);
      FastLED.setBrightness(160);
      Serial.print("WS2812B hardware enabled on data=");
      Serial.println(config.dataPin);
    }
    if (ws8Controllers[slot] != nullptr) {
      ws8Controllers[slot]->setLeds(ws8Leds[slot], config.pixelCount);
      ws8Controllers[slot]->setEnabled(true);
      ready_ = true;
    }
  } else {
    Serial.println("WS2812B hardware disabled: unsupported GPIO or more than 1200 pixels");
  }
#else
  if (config.id == 0 && config.dataPin == 23 && config.pixelCount <= kWs2812Output0MaxPixels) {
    if (!ws2812Output0Registered) {
      ws0 = &FastLED.addLeds<WS2812B, 23, GRB>(ws2812Output0Leds, config.pixelCount);
      FastLED.setBrightness(160);
      FastLED.clear(true);
      ws2812Output0Registered = true;
      Serial.println("WS2812B output 0 hardware enabled on data=23 (GRB driver, configurable color order)");
    }
    ws0->setLeds(ws2812Output0Leds, config.pixelCount);
    ws0->setEnabled(true);
    ready_ = true;
  } else if (config.id == 0) {
    Serial.println("WS2812B output 0 hardware disabled: expected data=23 pixels<=1024");
  }
#endif
#endif
  return ready_;
}

void Ws2812Output::show(const LogicalPixelBuffer& pixels) {
#ifdef ARDUINO
#if defined(LED_PROFILE_FLEX8)
  const int slot = ws8SlotForPin(config_.dataPin);
  if (slot >= 0 && ws8Controllers[slot] != nullptr) {
    copyPixelsToCrgb(pixels, config_, ws8Leds[slot]);
  }
#else
  if (config_.id == 0 && ws2812Output0Registered) {
    copyPixelsToCrgb(pixels, config_, ws2812Output0Leds);
  }
#endif
#else
  (void)pixels;
#endif
}

bool Ws2812Output::ready() const { return ready_; }
int Ws2812Output::id() const { return config_.id; }

bool LedOutputManager::begin(const ControllerConfig& config) {
#ifdef ARDUINO
  for (auto* controller : {apa0, apa1, ws0}) {
    if (controller) {
      controller->clearLeds();
      controller->showLeds(160);
      controller->setEnabled(false);
    }
  }
#if defined(LED_PROFILE_FLEX8)
  for (auto* controller : ws8Controllers) {
    if (controller) {
      controller->clearLeds();
      controller->showLeds(160);
      controller->setEnabled(false);
    }
  }
  for (auto* controller : apa8Controllers) {
    if (controller) {
      controller->clearLeds();
      controller->showLeds(160);
      controller->setEnabled(false);
    }
  }
#endif
#endif
  router_.setOutputs(config.outputs);
  apa102_.clear();
  ws2812_.clear();
  stats_.clear();
  for (const auto& output : config.outputs) {
    if (!output.enabled) continue;
    stats_.push_back(OutputRuntimeStats{output.id, output.type, output.pixelCount, 0});
    if (output.type == OutputType::APA102) {
      Apa102Output adapter;
      adapter.begin(output);
      apa102_.push_back(adapter);
    } else {
      Ws2812Output adapter;
      adapter.begin(output);
      ws2812_.push_back(adapter);
    }
  }
  return !apa102_.empty() || !ws2812_.empty();
}

void LedOutputManager::countUpdate(int outputId) {
  for (auto& stat : stats_) {
    if (stat.id == outputId) {
      stat.totalUpdates++;
      return;
    }
  }
}

void LedOutputManager::show(const LogicalPixelBuffer& finalFrame) {
  for (const auto& output : router_.outputs()) {
    if (!output.enabled) continue;
    bool updated = false;
    if (output.type == OutputType::APA102) {
      for (auto& adapter : apa102_) {
        if (adapter.ready() && adapter.id() == output.id) {
          adapter.show(finalFrame);
          updated = true;
          break;
        }
      }
    } else {
      for (auto& adapter : ws2812_) {
        if (adapter.ready() && adapter.id() == output.id) {
          adapter.show(finalFrame);
          updated = true;
          break;
        }
      }
    }
    if (updated) countUpdate(output.id);
  }
#ifdef ARDUINO
  FastLED.show();
#endif
}

uint32_t LedOutputManager::outputCount() const {
  return static_cast<uint32_t>(apa102_.size() + ws2812_.size());
}

const std::vector<OutputRuntimeStats>& LedOutputManager::stats() const { return stats_; }

}  // namespace led
