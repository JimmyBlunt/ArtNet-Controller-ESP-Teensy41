#pragma once

#include "LogicalPixelBuffer.h"

namespace led {

class OutputRouter {
 public:
  explicit OutputRouter(std::vector<OutputConfig> outputs = {});

  void setOutputs(std::vector<OutputConfig> outputs);
  bool validate(uint32_t logicalPixelCount, std::string* error = nullptr) const;
  std::vector<Rgb> sliceForOutput(uint32_t outputId,
                                  const LogicalPixelBuffer& finalFrame) const;
  std::vector<OutputEstimate> estimates() const;
  const std::vector<OutputConfig>& outputs() const;

 private:
  std::vector<OutputConfig> outputs_;
};

}  // namespace led
