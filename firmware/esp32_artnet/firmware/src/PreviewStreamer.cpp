#include "PreviewStreamer.h"

#ifdef ARDUINO
#include <WiFiUdp.h>
namespace {
WiFiUDP previewUdp;
}
#endif

namespace led {

bool PreviewStreamer::begin(uint16_t port) {
  localPort_ = port;
#ifdef ARDUINO
  return previewUdp.begin(localPort_) == 1;
#else
  return true;
#endif
}

void PreviewStreamer::setTarget(const char* host, uint16_t port) {
  targetHost_ = host ? host : "";
  targetPort_ = port;
  targetConfigured_ = true;
}

bool PreviewStreamer::sendFrame(uint32_t frameId, const LogicalPixelBuffer& frame,
                                uint32_t mappingHash) {
  if (!targetConfigured_) return false;
  PreviewHeader header;
  header.frameId = frameId;
  header.pixelCount = frame.size();
  header.mappingHash = mappingHash;
  const uint32_t maxPixels = 480;
  header.chunkCount = static_cast<uint16_t>((frame.size() + maxPixels - 1) / maxPixels);
  for (uint16_t chunk = 0; chunk < header.chunkCount; ++chunk) {
    header.chunkIndex = chunk;
    auto bytes = encodePreviewChunk(header, frame.pixels(), chunk * maxPixels, maxPixels);
#ifdef ARDUINO
    if (!previewUdp.beginPacket(targetHost_.c_str(), targetPort_)) return false;
    const bool written = previewUdp.write(bytes.data(), bytes.size()) == bytes.size();
    if (!previewUdp.endPacket() || !written) return false;
#endif
  }
  return true;
}

}  // namespace led
