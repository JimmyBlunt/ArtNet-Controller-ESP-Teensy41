#include "ArtNetReceiver.h"

#include <cstring>

#ifdef ARDUINO
#include <Arduino.h>
#include <cerrno>
#include <lwip/sockets.h>
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
bool ArtNetReceiver::begin(uint16_t port) {
  if (task_) return true;
  if (!queue_.begin()) return false;
  port_ = port;
  // loopTask runs at priority 1 on core 1. Only this task owns the UDP socket;
  // parsing, configuration, the assembler and LED output stay on loopTask.
  return xTaskCreatePinnedToCore(receiveTask, "artnet-rx", 4096, this, 2,
                                &task_, 0) == pdPASS;
}

int ArtNetReceiver::openSocket() {
  const int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (fd < 0) return -1;
  timeval timeout{};
  timeout.tv_usec = 20000;
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(port_);
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0 ||
      bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
    close(fd);
    return -1;
  }
#if LWIP_SO_RCVBUF
  // This byte limit does not enlarge lwIP's compiled UDP mailbox. The RX task
  // must still drain that mailbox while core 1 is inside FastLED.show().
  const int receiveBytes = ArtNetRxQueue::kCapacity * sizeof(ArtNetDatagram);
  if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &receiveBytes, sizeof(receiveBytes)) != 0) {
    socketErrors_.fetch_add(1, std::memory_order_relaxed);
  }
#endif
  return fd;
}

void ArtNetReceiver::receiveTask(void* context) {
  static_cast<ArtNetReceiver*>(context)->receiveLoop();
}

void ArtNetReceiver::receiveLoop() {
  int fd = -1;
  // One extra byte detects oversized datagrams, even when recvfrom truncates.
  // Never accept a valid-looking 530-byte prefix of a larger UDP datagram.
  uint8_t buffer[sizeof(ArtNetDatagram::bytes) + 1];
  for (;;) {
    if (fd < 0) {
      fd = openSocket();
      if (fd < 0) {
        socketErrors_.fetch_add(1, std::memory_order_relaxed);
        vTaskDelay(pdMS_TO_TICKS(250));
        continue;
      }
    }
    const int length = recvfrom(fd, buffer, sizeof(buffer), 0, nullptr, nullptr);
    if (length >= 0) {
      queue_.push(buffer, static_cast<size_t>(length));
      continue;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) continue;
    socketErrors_.fetch_add(1, std::memory_order_relaxed);
    close(fd);
    fd = -1;
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

bool ArtNetReceiver::poll(UniverseAssembler& assembler, bool* accepted) {
  if (accepted) *accepted = false;
  ArtNetDatagram raw;
  if (!queue_.pop(raw)) return false;
  UniversePacket packet;
  if (parseArtDmx(raw.bytes, raw.length, packet)) {
    assembler.accept(packet, millis());
    if (accepted) *accepted = true;
  }
  return true;
}
#endif

}  // namespace led
