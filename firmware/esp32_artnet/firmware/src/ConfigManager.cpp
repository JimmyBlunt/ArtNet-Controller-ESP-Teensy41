#include "ConfigManager.h"
#include "LedColorOrder.h"

#include "Performance.h"

#include <set>

namespace led {

ControllerConfig defaultMixedConfig() {
  ControllerConfig config;
#ifdef LED_PROFILE_MATRIX16_TEST
  config.pixelCount = 256;
  config.startUniverse = 0;
  config.universeCount = universesForPixels(config.pixelCount);
  config.targetFps = 60;
  config.outputs = {
      {0, OutputType::APA102, true, 0, 256, 23, 18, ColorOrder::BGR, false, 4000000, 0, 60},
  };
#elif defined(LED_PROFILE_WS2812_OUTPUT0)
  config.pixelCount = 256;
  config.startUniverse = 0;
  config.universeCount = universesForPixels(config.pixelCount);
  config.targetFps = 30;
  config.outputs = {
      {0, OutputType::WS2812B, true, 0, 256, 23, -1, ColorOrder::GRB, false, 0, 0, 30},
  };
#elif defined(LED_PROFILE_ESP251)
  config.pixelCount = 1309;
  config.startUniverse = 0;
  config.targetFps = 30;
  config.outputs = {
      {0, OutputType::WS2812B, true, 0, 285, 32, -1, ColorOrder::GRB, false, 0, 0, 30},
      {1, OutputType::APA102, true, 285, 1024, 18, 19, ColorOrder::BGR, false, 4000000, 2, 30},
  };
  config.universeCount = mappedUniverseCount(config);
#elif defined(LED_PROFILE_FLEX8)
  config.pixelCount = 256;
  config.startUniverse = 0;
  config.universeCount = universesForPixels(config.pixelCount);
  config.targetFps = 30;
  // Start with one output so the user can add the remaining outputs in the web menu.
  config.outputs = {
      {0, OutputType::WS2812B, true, 0, 256,
#if defined(LED_PROFILE_EXTENSION_BOARD)
       25,
#else
       32,
#endif
       -1, ColorOrder::GRB, false, 0, 0, 30},
  };
#elif defined(LED_PROFILE_WS2812_APA102_1679)
  config.pixelCount = 1679;
  config.startUniverse = 0;
  config.universeCount = universesForPixels(config.pixelCount);
  config.targetFps = 30;
  config.outputs = {
      {0, OutputType::WS2812B, true, 0, 512, 23, -1, ColorOrder::GRB, false, 0, 0, 30},
      {1, OutputType::APA102, true, 512, 1167, 18, 19, ColorOrder::BGR, false, 4000000, 4, 30},
  };
#else
  config.pixelCount = 4500;
  config.startUniverse = 0;
  config.universeCount = universesForPixels(config.pixelCount);
  config.targetFps = 60;
  config.outputs = {
      {0, OutputType::APA102, true, 0, 500, 23, 18, ColorOrder::BGR, false, 4000000, 0, 60},
      {1, OutputType::APA102, true, 500, 500, 19, 5, ColorOrder::BGR, false, 4000000, 3, 60},
      {2, OutputType::APA102, true, 1000, 500, 21, 22, ColorOrder::BGR, false, 4000000, 6, 60},
      {4, OutputType::WS2812B, true, 1500, 500, 13, -1, ColorOrder::GRB, false, 0, 9, 60},
      {5, OutputType::WS2812B, true, 2000, 500, 14, -1, ColorOrder::GRB, false, 0, 12, 60},
      {6, OutputType::WS2812B, true, 2500, 500, 25, -1, ColorOrder::GRB, false, 0, 15, 60},
      {7, OutputType::WS2812B, true, 3000, 500, 26, -1, ColorOrder::GRB, false, 0, 18, 60},
      {8, OutputType::WS2812B, true, 3500, 500, 27, -1, ColorOrder::GRB, false, 0, 21, 60},
      {9, OutputType::WS2812B, true, 4000, 500, 32, -1, ColorOrder::GRB, false, 0, 24, 60},
  };
#endif
#if !defined(LED_PROFILE_FLEX8)
  for (auto& output : config.outputs) output.startUniverse = config.startUniverse + output.startPixel / 170;
#endif
  return config;
}

uint16_t mappedUniverseCount(const ControllerConfig& config) {
  std::set<uint16_t> universes;
  for (const auto& output : config.outputs) {
    if (!output.enabled || output.pixelCount == 0) continue;
    const uint32_t count = universesForPixels(output.pixelCount);
    for (uint32_t offset = 0; offset < count; ++offset) {
      const uint32_t universe = static_cast<uint32_t>(output.startUniverse) + offset;
      if (universe < 32768) universes.insert(static_cast<uint16_t>(universe));
    }
  }
  return static_cast<uint16_t>(universes.size());
}

bool validateConfig(const ControllerConfig& config, std::string* error) {
  if (config.pixelCount == 0 || config.pixelCount > 8192) {
    if (error) *error = "controller pixelCount must be 1..8192";
    return false;
  }
#if defined(LED_PROFILE_FLEX8)
  if (config.startUniverse > 32767) {
    if (error) *error = "auto-layout start universe exceeds Art-Net address space";
    return false;
  }
  for (const auto& output : config.outputs) {
    if (!output.enabled) continue;
    if (static_cast<uint32_t>(output.startUniverse) + universesForPixels(output.pixelCount) > 32768) {
      if (error) *error = "output universe range exceeds Art-Net address space";
      return false;
    }
  }
#else
  if (config.startUniverse > 32767 ||
      static_cast<uint32_t>(config.startUniverse) + universesForPixels(config.pixelCount) > 32768) {
    if (error) *error = "universe range exceeds Art-Net address space";
    return false;
  }
#endif
  for (size_t i = 0; i < config.outputs.size(); ++i) {
    for (size_t j = 0; j < i; ++j) {
      if (config.outputs[i].id == config.outputs[j].id) {
        if (error) *error = "output IDs must be unique";
        return false;
      }
    }
  }
  OutputRouter router(config.outputs);
  return router.validate(config.pixelCount, error);
}

bool validateRuntimeHardwareConfig(const ControllerConfig& config, std::string* error) {
#if defined(LED_PROFILE_FLEX8)
  constexpr uint32_t kRuntimePixelLimit = 8192;
#else
  constexpr uint32_t kRuntimePixelLimit = 2400;
#endif
  if (config.pixelCount > kRuntimePixelLimit) {
    if (error) *error = "pixelCount exceeds runtime limit";
    return false;
  }
  if (!validateConfig(config, error)) return false;
  if (config.targetFps < 1 || config.targetFps > 60) {
    if (error) *error = "targetFps must be 1..60";
    return false;
  }
  for (size_t outputIndex = 0; outputIndex < config.outputs.size(); ++outputIndex) {
    const auto& output = config.outputs[outputIndex];
    if (!validColorOrder(output.colorOrder)) {
      if (error) *error = "colorOrder must be RGB, RBG, GRB, GBR, BRG or BGR";
      return false;
    }
#if defined(LED_PROFILE_FLEX8)
    #if defined(LED_PROFILE_EXTENSION_BOARD)
    // Two physical four-pin groups on the photographed ESP32-WROOM-32 board.
    static constexpr int kAllowedPins[] = {25, 26, 27, 14, 19, 18, 5, 17};
    // GPIO26/27 may be used separately for WS2812 or together for APA102.
    // Duplicate-pin validation below prevents both uses at the same time.
    static constexpr int kApaPairs[][2] = {{18, 5}, {26, 27}};
    constexpr size_t kMaximumOutputs = 8;
    #else
    static constexpr int kAllowedPins[] = {
        32, 33, 25, 26, 27, 14, 13, 16, 17, 18, 19, 21, 22, 23,
    };
    static constexpr int kApaPairs[][2] = {
        {18, 19}, {23, 18}, {13, 14}, {25, 26}, {32, 33}, {16, 17}, {21, 22},
    };
    constexpr size_t kMaximumOutputs = 8;
    #endif
    if (config.outputs.size() > kMaximumOutputs) {
      if (error) *error = "hardware profile has too many outputs";
      return false;
    }
    bool hardwareValid = false;
    if (output.type == OutputType::WS2812B) {
      bool pinAllowed = false;
      for (int pin : kAllowedPins) pinAllowed = pinAllowed || output.dataPin == pin;
      hardwareValid = pinAllowed && output.clockPin == -1 && output.pixelCount > 0 &&
                      output.pixelCount <= 1200 &&
                      output.spiHz == 0;
    } else {
      bool pairAllowed = false;
      for (const auto& pair : kApaPairs) {
        pairAllowed = pairAllowed ||
                      (output.dataPin == pair[0] && output.clockPin == pair[1]);
      }
      hardwareValid = pairAllowed && output.pixelCount > 0 && output.pixelCount <= 1200 &&
                      output.spiHz == 4000000;
    }
    if (!hardwareValid) {
      if (error) *error = "output type, pins or pixel count are not supported by this profile";
      return false;
    }
    for (size_t previous = 0; previous < outputIndex; ++previous) {
      const auto& earlier = config.outputs[previous];
      const bool pinsOverlap =
          output.dataPin == earlier.dataPin || output.dataPin == earlier.clockPin ||
          (output.clockPin >= 0 &&
           (output.clockPin == earlier.dataPin || output.clockPin == earlier.clockPin));
      if (pinsOverlap) {
        if (error) *error = "each GPIO can be reserved by only one output";
        return false;
      }
    }
#else
    if (!output.enabled) continue;
    if (output.type == OutputType::WS2812B) {
      if (output.id != 0 || output.dataPin != 23 || output.clockPin != -1 ||
          output.pixelCount > 1024 ||
          output.spiHz != 0) {
        if (error) {
          *error =
              "WS2812B runtime output must be id=0 dataPin=23 spiHz=0 pixels<=1024";
        }
        return false;
      }
    } else if (output.type == OutputType::APA102) {
      if (output.id != 1 || output.dataPin != 18 || output.clockPin != 19 ||
          output.pixelCount > 1200 ||
          output.spiHz != 4000000) {
        if (error) {
          *error =
              "APA102 runtime output must be id=1 dataPin=18 clockPin=19 spiHz=4000000 pixels<=1200";
        }
        return false;
      }
    }
#endif
  }
  return true;
}

}  // namespace led
