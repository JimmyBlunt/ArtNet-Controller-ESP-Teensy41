#include "OutputRouter.h"

#include <algorithm>

#include "Performance.h"

namespace led {

OutputRouter::OutputRouter(std::vector<OutputConfig> outputs) : outputs_(std::move(outputs)) {}
void OutputRouter::setOutputs(std::vector<OutputConfig> outputs) { outputs_ = std::move(outputs); }
const std::vector<OutputConfig>& OutputRouter::outputs() const { return outputs_; }

bool OutputRouter::validate(uint32_t logicalPixelCount, std::string* error) const {
  std::vector<uint8_t> occupied(logicalPixelCount, 0);
  for (const auto& output : outputs_) {
    if (!output.enabled) continue;
    if (output.pixelCount == 0) {
      if (error) *error = "enabled output has zero pixels";
      return false;
    }
    if (output.startPixel > logicalPixelCount ||
        output.pixelCount > logicalPixelCount - output.startPixel) {
      if (error) *error = "output exceeds logical pixel count";
      return false;
    }
    for (uint32_t i = 0; i < output.pixelCount; ++i) {
      auto& slot = occupied[output.startPixel + i];
      if (slot != 0) {
        if (error) *error = "outputs overlap";
        return false;
      }
      slot = 1;
    }
  }
  return true;
}

std::vector<Rgb> OutputRouter::sliceForOutput(uint32_t outputId,
                                              const LogicalPixelBuffer& finalFrame) const {
  for (const auto& output : outputs_) {
    if (static_cast<uint32_t>(output.id) != outputId || !output.enabled) continue;
    std::vector<Rgb> slice(output.pixelCount);
    for (uint32_t i = 0; i < output.pixelCount; ++i) {
      const uint32_t physical = output.reverse ? (output.pixelCount - 1 - i) : i;
      slice[physical] = finalFrame.get(output.startPixel + i);
    }
    return slice;
  }
  return {};
}

std::vector<OutputEstimate> OutputRouter::estimates() const {
  std::vector<OutputEstimate> out;
  for (const auto& output : outputs_) out.push_back(estimateOutput(output));
  return out;
}

}  // namespace led
