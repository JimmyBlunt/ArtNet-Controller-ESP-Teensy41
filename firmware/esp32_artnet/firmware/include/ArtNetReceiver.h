#pragma once

#include "UniverseAssembler.h"

namespace led {

constexpr uint16_t kArtNetPort = 6454;

class ArtNetReceiver {
 public:
  static bool parseArtDmx(const uint8_t* bytes, size_t length, UniversePacket& packet);

#ifdef ARDUINO
  bool begin(uint16_t port = kArtNetPort);
  bool poll(UniverseAssembler& assembler);
#endif
};

}  // namespace led
