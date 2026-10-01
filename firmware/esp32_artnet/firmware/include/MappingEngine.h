#pragma once

#include "LogicalPixelBuffer.h"

namespace led {

struct Mapping {
  std::string id = "linear";
  MappingMode mode = MappingMode::Linear;
  uint32_t pixelCount = 0;
  std::vector<uint32_t> indexMap;
  std::vector<Point3> points;
};

class MappingEngine {
 public:
  void setMapping(Mapping mapping);
  const Mapping& mapping() const;
  bool validate(std::string* error = nullptr) const;
  LogicalPixelBuffer apply(const LogicalPixelBuffer& source) const;

 private:
  Mapping mapping_;
};

std::vector<Point3> normalizeContain(const std::vector<Point3>& points);
std::vector<Point3> normalizeFill(const std::vector<Point3>& points);

}  // namespace led
