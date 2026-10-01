#include <cassert>
#include <iostream>
#include "ConfigManager.h"
#include "UniverseAssembler.h"

int main() {
  const auto config = led::defaultMixedConfig();
  std::string error;
  assert(led::validateRuntimeHardwareConfig(config, &error));
  assert(config.pixelCount == 1309 && config.universeCount == 9);
  assert(config.outputs[0].dataPin == 32 && config.outputs[0].pixelCount == 285);
  assert(config.outputs[1].dataPin == 18 && config.outputs[1].clockPin == 19);
  led::UniverseAssembler assembler(config);
  for (uint16_t universe = 0; universe < 9; ++universe) {
    led::UniversePacket packet;
    packet.universe = universe;
    packet.sequence = 1;
    packet.data.assign(510, static_cast<uint8_t>(universe + 1));
    assert(assembler.accept(packet, 1));
  }
  assert(assembler.frameReady());
  led::LogicalPixelBuffer frame;
  assembler.compose(frame);
  // The short WS2812 tail must not spill into the first APA102 pixel.
  assert(frame.get(284).r == 2);
  assert(frame.get(285).r == 3);
  assert(frame.get(1308).r == 9);
  std::cout << "ESP251 configuration and universe boundary tests passed\n";
}
