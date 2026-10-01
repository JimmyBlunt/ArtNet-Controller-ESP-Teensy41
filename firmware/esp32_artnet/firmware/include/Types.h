#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace led {

enum class OutputType { APA102, WS2812B };
enum class ColorOrder { RGB, RBG, GRB, GBR, BRG, BGR };
enum class LossMode { Hold, Blackout, FadeToBlack, TestPattern };
enum class MappingMode { Linear, Points1D, Points2D, Points3D };

struct Rgb {
  Rgb() = default;
  Rgb(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}

  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
};

struct Point3 {
  Point3() = default;
  Point3(float xValue, float yValue, float zValue) : x(xValue), y(yValue), z(zValue) {}

  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

struct OutputConfig {
  OutputConfig() = default;
  OutputConfig(int idValue, OutputType typeValue, bool enabledValue, uint32_t startPixelValue,
               uint32_t pixelCountValue, int dataPinValue, int clockPinValue,
               ColorOrder colorOrderValue, bool reverseValue, uint32_t spiHzValue,
               uint16_t startUniverseValue, uint16_t targetFpsValue)
      : id(idValue),
        type(typeValue),
        enabled(enabledValue),
        startPixel(startPixelValue),
        pixelCount(pixelCountValue),
        dataPin(dataPinValue),
        clockPin(clockPinValue),
        colorOrder(colorOrderValue),
        reverse(reverseValue),
        spiHz(spiHzValue),
        startUniverse(startUniverseValue),
        targetFps(targetFpsValue) {}

  int id = 0;
  OutputType type = OutputType::APA102;
  bool enabled = true;
  uint32_t startPixel = 0;
  uint32_t pixelCount = 0;
  int dataPin = -1;
  int clockPin = -1;
  ColorOrder colorOrder = ColorOrder::RGB;
  bool reverse = false;
  uint32_t spiHz = 4000000;
  uint16_t startUniverse = 0;
  uint16_t targetFps = 60;
};

struct ControllerConfig {
  uint32_t pixelCount = 0;
  uint16_t startUniverse = 0;
  uint16_t universeCount = 27;
  uint16_t targetFps = 60;
  LossMode signalLossMode = LossMode::Hold;
  std::vector<OutputConfig> outputs;
};

struct UniversePacket {
  uint16_t universe = 0;
  uint8_t sequence = 0;
  std::vector<uint8_t> data;
};

struct OutputEstimate {
  uint32_t outputId = 0;
  double frameTimeUs = 0.0;
  double theoreticalFps = 0.0;
  bool chainTooLongForTarget = false;
  uint16_t firstUniverse = 0;
  uint16_t lastUniverse = 0;
  double dataRateMbps = 0.0;
};

}  // namespace led
