#include "UniverseAssembler.h"
#include <algorithm>

namespace led {
UniverseAssembler::UniverseAssembler(uint16_t startUniverse, uint16_t universeCount,
                                     uint32_t pixelCount)
    : startUniverse_(startUniverse), universeCount_(universeCount), pixelCount_(pixelCount) {
  for (uint16_t slot = 0; slot < universeCount; ++slot) {
    const uint16_t universe = static_cast<uint16_t>(startUniverse + slot);
    universeIds_.push_back(universe);
    const uint32_t targetPixel = static_cast<uint32_t>(slot) * 170;
    const uint32_t remaining = targetPixel < pixelCount ? pixelCount - targetPixel : 0;
    routes_.push_back(Route{universe, targetPixel,
                            static_cast<uint16_t>(std::min<uint32_t>(170, remaining))});
  }
  initializeStorage();
}

UniverseAssembler::UniverseAssembler(const ControllerConfig& config)
    : startUniverse_(config.startUniverse), universeCount_(0), pixelCount_(config.pixelCount) {
  for (const auto& output : config.outputs) {
    if (!output.enabled || output.pixelCount == 0) continue;
    const uint32_t count = (output.pixelCount + 169) / 170;
    for (uint32_t offset = 0; offset < count; ++offset) {
      const uint16_t universe = static_cast<uint16_t>(output.startUniverse + offset);
      universeIds_.push_back(universe);
      const uint32_t consumed = offset * 170;
      routes_.push_back(Route{universe, output.startPixel + consumed,
                              static_cast<uint16_t>(std::min<uint32_t>(170, output.pixelCount - consumed))});
    }
  }
  std::sort(universeIds_.begin(), universeIds_.end());
  universeIds_.erase(std::unique(universeIds_.begin(), universeIds_.end()), universeIds_.end());
  universeCount_ = static_cast<uint16_t>(universeIds_.size());
  if (!universeIds_.empty()) startUniverse_ = universeIds_.front();
  initializeStorage();
}

void UniverseAssembler::initializeStorage() {
  lastSequences_.assign(universeCount_, 0);
  lastPacketMs_.assign(universeCount_, 0);
  universes_.resize(universeCount_);
  for (auto& universe : universes_) universe.reserve(512);
}

int UniverseAssembler::slotForUniverse(uint16_t universe) const {
  const auto found = std::lower_bound(universeIds_.begin(), universeIds_.end(), universe);
  if (found == universeIds_.end() || *found != universe) return -1;
  return static_cast<int>(found - universeIds_.begin());
}

bool UniverseAssembler::accept(const UniversePacket& packet, uint32_t nowMs) {
  stats_.packets++;
  expire(nowMs);
  const int resolvedSlot = slotForUniverse(packet.universe);
  if (resolvedSlot < 0 || packet.data.size() < 2 || packet.data.size() > 512 ||
      packet.data.size() % 2 != 0) {
    stats_.droppedPackets++;
    return false;
  }
  const auto slot = static_cast<uint16_t>(resolvedSlot);
  // ArtDmx sequence orders packets per universe, not global frame IDs.
  const uint8_t previous = lastSequences_[slot];
  if (packet.sequence != 0 && previous != 0 && nowMs - lastPacketMs_[slot] < 2000) {
    const int distance = (static_cast<int>(packet.sequence) - previous + 255) % 255;
    if (distance == 0 || distance > 127) {
      stats_.sequenceErrors++;
      stats_.droppedPackets++;
      return false;
    }
  }
  lastSequences_[slot] = packet.sequence;
  lastPacketMs_[slot] = nowMs;
  // A new packet for an already collected universe abandons the incomplete collection.
  // Without ArtSync these boundaries are best-effort, not guaranteed sender frame IDs.
  if (received_.count(slot)) {
    if (!frameReady()) stats_.framesIncomplete++;
    received_.clear();
  }
  if (received_.empty()) frameStartMs_ = nowMs;
  universes_[slot] = packet.data;
  received_.insert(slot);
  return true;
}

bool UniverseAssembler::frameReady() const {
  return universeCount_ > 0 && received_.size() == universeCount_;
}

void UniverseAssembler::compose(LogicalPixelBuffer& target) {
  if (target.size() != pixelCount_) target.resize(pixelCount_);
  target.clear();
  for (const auto& route : routes_) {
    const int resolvedSlot = slotForUniverse(route.universe);
    if (resolvedSlot < 0 || !received_.count(static_cast<uint16_t>(resolvedSlot))) continue;
    const auto& universe = universes_[static_cast<size_t>(resolvedSlot)];
    for (uint32_t localPixel = 0; localPixel < route.pixelCount; ++localPixel) {
      const size_t channel = static_cast<size_t>(localPixel) * 3;
      if (channel + 2 >= universe.size() || channel >= 510) break;
      const uint32_t pixel = route.targetPixel + localPixel;
      if (pixel >= pixelCount_) break;
      target.set(pixel, Rgb{universe[channel], universe[channel + 1], universe[channel + 2]});
    }
  }
  if (frameReady()) stats_.framesComplete++;
  else stats_.framesIncomplete++;
  received_.clear();
}

bool UniverseAssembler::expire(uint32_t nowMs, uint32_t timeoutMs) {
  if (!received_.empty() && nowMs - frameStartMs_ >= timeoutMs) {
    markTimeout();
    return true;
  }
  return false;
}
void UniverseAssembler::discard() { received_.clear(); }
void UniverseAssembler::markTimeout() {
  stats_.timeouts++;
  if (!received_.empty() && !frameReady()) stats_.framesIncomplete++;
  received_.clear();
}
const AssemblerStats& UniverseAssembler::stats() const { return stats_; }
uint16_t UniverseAssembler::startUniverse() const { return startUniverse_; }
uint16_t UniverseAssembler::universeCount() const { return universeCount_; }
}  // namespace led
