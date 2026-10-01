#pragma once

#include <set>

#include "LogicalPixelBuffer.h"

namespace led {

struct AssemblerStats {
  uint64_t packets = 0;
  uint64_t framesComplete = 0;
  uint64_t framesIncomplete = 0;
  uint64_t sequenceErrors = 0;
  uint64_t timeouts = 0;
  uint64_t droppedPackets = 0;
};

class UniverseAssembler {
 public:
  UniverseAssembler(uint16_t startUniverse, uint16_t universeCount,
                    uint32_t pixelCount);
  explicit UniverseAssembler(const ControllerConfig& config);

  bool accept(const UniversePacket& packet, uint32_t nowMs = 0);
  bool expire(uint32_t nowMs, uint32_t timeoutMs = 100);
  void discard();
  bool frameReady() const;
  void compose(LogicalPixelBuffer& target);
  void markTimeout();
  const AssemblerStats& stats() const;
  uint16_t startUniverse() const;
  uint16_t universeCount() const;

 private:
  struct Route {
    Route() = default;
    Route(uint16_t universeValue, uint32_t targetPixelValue, uint16_t pixelCountValue)
        : universe(universeValue), targetPixel(targetPixelValue), pixelCount(pixelCountValue) {}
    uint16_t universe = 0;
    uint32_t targetPixel = 0;
    uint16_t pixelCount = 0;
  };
  void initializeStorage();
  int slotForUniverse(uint16_t universe) const;

  uint16_t startUniverse_;
  uint16_t universeCount_;
  uint32_t pixelCount_;
  std::vector<uint16_t> universeIds_;
  std::vector<Route> routes_;
  std::vector<uint8_t> lastSequences_;
  std::vector<uint32_t> lastPacketMs_;
  uint32_t frameStartMs_ = 0;
  std::vector<std::vector<uint8_t>> universes_;
  std::set<uint16_t> received_;
  AssemblerStats stats_;
};

}  // namespace led
