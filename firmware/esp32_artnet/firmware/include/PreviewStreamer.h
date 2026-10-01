#pragma once

#include "PreviewProtocol.h"

#include <string>

namespace led {

class PreviewStreamer {
 public:
  bool begin(uint16_t port = 6455);
  void setTarget(const char* host, uint16_t port);
  bool sendFrame(uint32_t frameId, const LogicalPixelBuffer& frame, uint32_t mappingHash = 0);

 private:
  uint16_t localPort_ = 6455;
  uint16_t targetPort_ = 6455;
  std::string targetHost_ = "255.255.255.255";
  bool targetConfigured_ = false;
};

}  // namespace led
