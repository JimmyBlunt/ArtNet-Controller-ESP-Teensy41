#pragma once

#include "UniverseAssembler.h"
#ifdef ARDUINO
#include "ArtNetRxQueue.h"
#include <freertos/task.h>
#endif

namespace led {

constexpr uint16_t kArtNetPort = 6454;

class ArtNetReceiver {
 public:
  static bool parseArtDmx(const uint8_t* bytes, size_t length, UniversePacket& packet);

#ifdef ARDUINO
  bool begin(uint16_t port = kArtNetPort);
  // True means one raw datagram was consumed, even if it was not ArtDmx.
  bool poll(UniverseAssembler& assembler, bool* accepted = nullptr);
  uint32_t rxPackets() const { return queue_.packets(); }
  uint32_t rxQueueDrops() const { return queue_.drops(); }
  uint32_t rxOversizedPackets() const { return queue_.oversized(); }
  uint32_t rxSocketErrors() const { return socketErrors_.load(std::memory_order_relaxed); }

 private:
  static void receiveTask(void* context);
  void receiveLoop();
  int openSocket();
  ArtNetRxQueue queue_;
  TaskHandle_t task_ = nullptr;
  uint16_t port_ = kArtNetPort;
  std::atomic<uint32_t> socketErrors_{0};
#endif
};

}  // namespace led
