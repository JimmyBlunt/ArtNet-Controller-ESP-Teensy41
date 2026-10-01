#include "LogicalPixelBuffer.h"

namespace led {

LogicalPixelBuffer::LogicalPixelBuffer(uint32_t pixelCount) : pixels_(pixelCount) {}

void LogicalPixelBuffer::resize(uint32_t pixelCount) { pixels_.assign(pixelCount, {}); }
uint32_t LogicalPixelBuffer::size() const { return static_cast<uint32_t>(pixels_.size()); }
void LogicalPixelBuffer::clear() { std::fill(pixels_.begin(), pixels_.end(), Rgb{}); }

bool LogicalPixelBuffer::set(uint32_t index, Rgb color) {
  if (index >= pixels_.size()) return false;
  pixels_[index] = color;
  return true;
}

Rgb LogicalPixelBuffer::get(uint32_t index) const {
  if (index >= pixels_.size()) return {};
  return pixels_[index];
}

const std::vector<Rgb>& LogicalPixelBuffer::pixels() const { return pixels_; }
std::vector<Rgb>& LogicalPixelBuffer::pixels() { return pixels_; }

}  // namespace led
