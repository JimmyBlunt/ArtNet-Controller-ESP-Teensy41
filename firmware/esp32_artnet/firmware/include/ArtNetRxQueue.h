#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

namespace led {

struct ArtNetDatagram {
  uint16_t length = 0;
  uint8_t bytes[530] = {};
};

// One producer (the socket task), one consumer (loop). FreeRTOS copies whole
// datagrams; neither side keeps pointers into the other side's buffers.
class ArtNetRxQueue {
 public:
  static constexpr size_t kCapacity = 64;
  ArtNetRxQueue() = default;
  ArtNetRxQueue(const ArtNetRxQueue&) = delete;
  ArtNetRxQueue& operator=(const ArtNetRxQueue&) = delete;

  bool begin() {
    if (!queue_) {
      queue_ = xQueueCreateStatic(kCapacity, sizeof(ArtNetDatagram), storage_, &control_);
    }
    return queue_ != nullptr;
  }

  bool push(const uint8_t* bytes, size_t length) {
    packets_.fetch_add(1, std::memory_order_relaxed);
    if (length > sizeof(ArtNetDatagram::bytes)) {
      oversized_.fetch_add(1, std::memory_order_relaxed);
      return false;
    }
    ArtNetDatagram packet;
    packet.length = static_cast<uint16_t>(length);
    if (length) std::memcpy(packet.bytes, bytes, length);
    if (!queue_ || xQueueSend(queue_, &packet, 0) != pdTRUE) {
      drops_.fetch_add(1, std::memory_order_relaxed);
      return false;
    }
    return true;
  }

  bool pop(ArtNetDatagram& packet) {
    return queue_ && xQueueReceive(queue_, &packet, 0) == pdTRUE;
  }
  uint32_t packets() const { return packets_.load(std::memory_order_relaxed); }
  uint32_t drops() const { return drops_.load(std::memory_order_relaxed); }
  uint32_t oversized() const { return oversized_.load(std::memory_order_relaxed); }

 private:
  StaticQueue_t control_{};
  alignas(4) uint8_t storage_[kCapacity * sizeof(ArtNetDatagram)]{};
  QueueHandle_t queue_ = nullptr;
  std::atomic<uint32_t> packets_{0};
  std::atomic<uint32_t> drops_{0};
  std::atomic<uint32_t> oversized_{0};
};

}  // namespace led
