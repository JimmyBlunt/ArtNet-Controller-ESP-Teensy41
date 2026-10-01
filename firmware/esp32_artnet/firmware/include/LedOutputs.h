#pragma once

#include "ConfigManager.h"

namespace led {

struct OutputRuntimeStats {
  OutputRuntimeStats() = default;
  OutputRuntimeStats(int idValue, OutputType typeValue, uint32_t pixelCountValue,
                     uint32_t totalUpdatesValue)
      : id(idValue),
        type(typeValue),
        pixelCount(pixelCountValue),
        totalUpdates(totalUpdatesValue) {}

  int id = 0;
  OutputType type = OutputType::APA102;
  uint32_t pixelCount = 0;
  uint32_t totalUpdates = 0;
};

class Apa102Output {
 public:
  bool begin(const OutputConfig& config);
  void show(const LogicalPixelBuffer& pixels);
  bool ready() const;
  int id() const;

 private:
  OutputConfig config_;
  bool ready_ = false;
};

class Ws2812Output {
 public:
  bool begin(const OutputConfig& config);
  void show(const LogicalPixelBuffer& pixels);
  bool ready() const;
  int id() const;

 private:
  OutputConfig config_;
  bool ready_ = false;
};

class LedOutputManager {
 public:
  bool begin(const ControllerConfig& config);
  void show(const LogicalPixelBuffer& finalFrame);
  uint32_t outputCount() const;
  const std::vector<OutputRuntimeStats>& stats() const;

 private:
  void countUpdate(int outputId);

  OutputRouter router_;
  std::vector<Apa102Output> apa102_;
  std::vector<Ws2812Output> ws2812_;
  std::vector<OutputRuntimeStats> stats_;
};

}  // namespace led
