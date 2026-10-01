#include "MappingEngine.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace led {

void MappingEngine::setMapping(Mapping mapping) { mapping_ = std::move(mapping); }
const Mapping& MappingEngine::mapping() const { return mapping_; }

bool MappingEngine::validate(std::string* error) const {
  if (mapping_.pixelCount == 0) {
    if (error) *error = "mapping pixelCount must be greater than zero";
    return false;
  }
  for (const auto& p : mapping_.points) {
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) {
      if (error) *error = "mapping contains NaN or Infinity";
      return false;
    }
  }
  for (auto index : mapping_.indexMap) {
    if (index >= mapping_.pixelCount) {
      if (error) *error = "indexMap references a pixel outside pixelCount";
      return false;
    }
  }
  return true;
}

LogicalPixelBuffer MappingEngine::apply(const LogicalPixelBuffer& source) const {
  LogicalPixelBuffer out(source.size());
  if (mapping_.indexMap.empty()) {
    out.pixels() = source.pixels();
    return out;
  }
  for (uint32_t dst = 0; dst < mapping_.indexMap.size() && dst < out.size(); ++dst) {
    out.set(dst, source.get(mapping_.indexMap[dst]));
  }
  return out;
}

static std::vector<Point3> normalize(const std::vector<Point3>& points, bool fill) {
  if (points.empty()) return {};
  Point3 minP{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
              std::numeric_limits<float>::max()};
  Point3 maxP{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
              std::numeric_limits<float>::lowest()};
  for (const auto& p : points) {
    minP.x = std::min(minP.x, p.x); minP.y = std::min(minP.y, p.y); minP.z = std::min(minP.z, p.z);
    maxP.x = std::max(maxP.x, p.x); maxP.y = std::max(maxP.y, p.y); maxP.z = std::max(maxP.z, p.z);
  }
  const float sx = std::max(0.000001f, maxP.x - minP.x);
  const float sy = std::max(0.000001f, maxP.y - minP.y);
  const float sz = std::max(0.000001f, maxP.z - minP.z);
  const float s = fill ? std::min({sx, sy, sz}) : std::max({sx, sy, sz});
  std::vector<Point3> out;
  out.reserve(points.size());
  for (const auto& p : points) {
    out.push_back(Point3{(p.x - minP.x) / s, (p.y - minP.y) / s, (p.z - minP.z) / s});
  }
  return out;
}

std::vector<Point3> normalizeContain(const std::vector<Point3>& points) {
  return normalize(points, false);
}

std::vector<Point3> normalizeFill(const std::vector<Point3>& points) {
  return normalize(points, true);
}

}  // namespace led
