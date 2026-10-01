#include "ArtNetReceiver.h"

#include <cstring>

#ifdef ARDUINO
#include <WiFiUdp.h>
#include <Arduino.h>
namespace {
WiFiUDP artnetUdp;
}
#endif

namespace led {

bool ArtNetReceiver::parseArtDmx(const uint8_t* bytes, size_t length, UniversePacket& packet) {
  if (length < 18) return false;
  if (std::memcmp(bytes, "Art-Net\0", 8) != 0) return false;
  const uint16_t opcode = static_cast<uint16_t>(bytes[8] | (bytes[9] << 8));
  if (opcode != 0x5000) return false;
  const uint16_t dmxLength = static_cast<uint16_t>((bytes[16] << 8) | bytes[17]);
  if (dmxLength < 2 || dmxLength > 512 || dmxLength % 2 != 0 ||
      length < static_cast<size_t>(18 + dmxLength)) return false;
  packet.sequence = bytes[12];
  packet.universe = static_cast<uint16_t>(bytes[14] | (bytes[15] << 8));
  packet.data.assign(bytes + 18, bytes + 18 + dmxLength);
  return true;
}

#ifdef ARDUINO
bool ArtNetReceiver::begin(uint16_t port) { return artnetUdp.begin(port) == 1; }

bool ArtNetReceiver::poll(UniverseAssembler& assembler) {
  const int packetSize = artnetUdp.parsePacket();
  if (packetSize <= 0) return false;
  uint8_t buffer[530];
  const int read = artnetUdp.read(buffer, sizeof(buffer));
  UniversePacket packet;
  if (!parseArtDmx(buffer, read, packet)) return false;
  assembler.accept(packet, millis());
  return true;
}
#endif

}  // namespace led
