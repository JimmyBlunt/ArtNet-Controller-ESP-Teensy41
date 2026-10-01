#pragma once

#include "Types.h"

namespace led {

class LogicalPixelBuffer {
 public:
  explicit LogicalPixelBuffer(uint32_t pixelCount = 0);

  void resize(uint32_t pixelCount);
  uint32_t size() const;
  void clear();
  bool set(uint32_t index, Rgb color);
  Rgb get(uint32_t index) const;
  const std::vector<Rgb>& pixels() const;
  std::vector<Rgb>& pixels();

 private:
  std::vector<Rgb> pixels_;
};

}  // namespace led
