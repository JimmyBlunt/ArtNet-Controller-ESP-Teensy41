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
}

int main() {
  auto config = defaultMixedConfig();
  std::string error;
  expect(config.outputs.size() == 1 && config.outputs[0].dataPin == 32,
         "flex profile starts with one user-expandable output");
  expect(validateRuntimeHardwareConfig(config, &error), "default flex profile is valid");

  config.pixelCount = 8192;
  config.outputs.clear();
  const int pins[] = {32, 33, 25, 26, 27, 14, 13, 23};
  for (int id = 0; id < 8; ++id) {
    config.outputs.push_back({id, OutputType::WS2812B, true,
                              static_cast<uint32_t>(id * 1024), 1024, pins[id], -1,
                              ColorOrder::GRB, false, 0,
                              static_cast<uint16_t>(id * 7), 30});
  }
  config.universeCount = mappedUniverseCount(config);
  expect(config.universeCount == 56, "aligned 1024-pixel outputs use seven universes each");
  expect(validateRuntimeHardwareConfig(config, &error), "eight unique WS2812 outputs are valid");

  config.outputs[7] = {7, OutputType::APA102, true, 7168, 1024, 23, 18,
                       ColorOrder::BGR, false, 4000000, 49, 30};
  expect(validateRuntimeHardwareConfig(config, &error), "APA102 reserves a supported data-clock pair");

  for (auto order : {ColorOrder::RGB, ColorOrder::RBG, ColorOrder::GRB,
                     ColorOrder::GBR, ColorOrder::BRG, ColorOrder::BGR}) {
    auto changed = config;
    for (auto& output : changed.outputs) output.colorOrder = order;
    expect(validateRuntimeHardwareConfig(changed, &error), "flex WS and APA accept all color orders");
  }
  auto invalidOrder = config;
  invalidOrder.outputs[7].colorOrder = static_cast<ColorOrder>(-1);
  expect(!validateRuntimeHardwareConfig(invalidOrder, &error), "invalid color order rejected");

  auto duplicate = config;
  duplicate.outputs[6].dataPin = 18;
  expect(!validateRuntimeHardwareConfig(duplicate, &error), "pin reuse across WS2812 and APA102 is rejected");

  auto bootPin = config;
  bootPin.outputs[0].dataPin = 12;
  expect(!validateRuntimeHardwareConfig(bootPin, &error), "boot strapping GPIO12 is rejected");

  auto invalidUniverse = config;
  invalidUniverse.outputs[0].startUniverse = 32767;
  expect(!validateRuntimeHardwareConfig(invalidUniverse, &error),
         "output universe range cannot exceed Art-Net address space");

  if (failures) return EXIT_FAILURE;
  std::cout << "Flexible output profile tests passed\n";
  return EXIT_SUCCESS;
}
