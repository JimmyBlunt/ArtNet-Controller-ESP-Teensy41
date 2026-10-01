#include <cstdlib>
#include <iostream>
#include <string>

#include "ConfigManager.h"
#include "Performance.h"

using namespace led;

namespace {
int failures = 0;

void expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

ControllerConfig configWithOutputs(const std::vector<OutputConfig>& outputs) {
  ControllerConfig config;
  config.startUniverse = 0;
  config.targetFps = 30;
  config.outputs = outputs;
  config.pixelCount = 0;
  for (const auto& output : outputs) {
    if (output.enabled) config.pixelCount += output.pixelCount;
  }
  config.universeCount = mappedUniverseCount(config);
  return config;
}
}

int main() {
  std::string error;
  const int wsPins[] = {25, 26, 27, 14, 19, 18, 5, 17};
  std::vector<OutputConfig> wsOutputs;
  for (int id = 0; id < 8; ++id) {
    wsOutputs.push_back({id, OutputType::WS2812B, true,
                         static_cast<uint32_t>(id * 1024), 1024, wsPins[id], -1,
                         ColorOrder::GRB, false, 0,
                         static_cast<uint16_t>(id * 7), 30});
  }
  auto wsConfig = configWithOutputs(wsOutputs);
  expect(validateRuntimeHardwareConfig(wsConfig, &error),
         "all eight photographed-board WS2812 GPIO outputs are valid");

  auto longWs = configWithOutputs({
      {0, OutputType::WS2812B, true, 0, 1122, 25, -1, ColorOrder::GRB,
       false, 0, 0, 25},
  });
  expect(validateRuntimeHardwareConfig(longWs, &error),
         "1122 WS2812 pixels on one output are supported");
  longWs.outputs[0].pixelCount = 1201;
  longWs.pixelCount = 1201;
  longWs.universeCount = mappedUniverseCount(longWs);
  expect(!validateRuntimeHardwareConfig(longWs, &error),
         "more than 1200 pixels on one output remain rejected");

  for (int inputOnlyPin : {34, 35}) {
    auto invalid = wsConfig;
    invalid.outputs[0].dataPin = inputOnlyPin;
    expect(!validateRuntimeHardwareConfig(invalid, &error),
           "GPIO34 and GPIO35 remain input-only");
  }

  for (int reservedPin : {0, 2, 4, 12, 13, 15, 16, 21, 22, 23, 32, 33}) {
    auto invalid = wsConfig;
    invalid.outputs[0].dataPin = reservedPin;
    expect(!validateRuntimeHardwareConfig(invalid, &error),
           "non-extension WS2812 pins remain unavailable");
  }

  const int apaPairs[][2] = {{18, 5}, {26, 27}};
  std::vector<OutputConfig> apaOutputs;
  for (int id = 0; id < 2; ++id) {
    apaOutputs.push_back({id, OutputType::APA102, true,
                          static_cast<uint32_t>(id * 1024), 1024,
                          apaPairs[id][0], apaPairs[id][1], ColorOrder::BGR,
                          false, 4000000, static_cast<uint16_t>(id * 7), 30});
  }
  auto apaConfig = configWithOutputs(apaOutputs);
  expect(validateRuntimeHardwareConfig(apaConfig, &error),
         "two selectable APA102 pairs are valid");

  auto overlapping = apaConfig;
  overlapping.outputs.push_back({2, OutputType::WS2812B, true, 2048, 128, 26, -1,
                                  ColorOrder::GRB, false, 0, 14, 30});
  overlapping.pixelCount += 128;
  overlapping.universeCount = mappedUniverseCount(overlapping);
  expect(!validateRuntimeHardwareConfig(overlapping, &error),
         "an APA102 pin cannot be reused by another output");

  std::vector<OutputConfig> tooMany = wsOutputs;
  tooMany.push_back({8, OutputType::WS2812B, true, 8192, 1, 25, -1,
                     ColorOrder::GRB, false, 0, 49, 30});
  auto nineOutputs = configWithOutputs(tooMany);
  expect(!validateRuntimeHardwareConfig(nineOutputs, &error),
         "extension-board profile keeps the eight-output controller limit");

  if (failures) return EXIT_FAILURE;
  std::cout << "Extension-board profile tests passed\n";
  return EXIT_SUCCESS;
}
