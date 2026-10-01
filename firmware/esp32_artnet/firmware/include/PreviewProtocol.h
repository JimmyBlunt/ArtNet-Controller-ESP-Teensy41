#pragma once

#include "LogicalPixelBuffer.h"

namespace led {

struct PreviewHeader {
  char magic[4] = {'S', 'L', 'P', 'V'};
  uint8_t version = 1;
  uint32_t frameId = 0;
  uint64_t timestampUs = 0;
  uint32_t pixelCount = 0;
  uint16_t chunkIndex = 0;
  uint16_t chunkCount = 1;
  uint32_t mappingHash = 0;
};

std::vector<uint8_t> encodePreviewChunk(const PreviewHeader& header,
                                        const std::vector<Rgb>& pixels,
                                        uint32_t firstPixel,
                                        uint32_t maxPixels);
bool decodePreviewChunk(const std::vector<uint8_t>& bytes,
                        PreviewHeader& header,
                        std::vector<Rgb>& pixels);

}  // namespace led
