#include "PreviewProtocol.h"

#include <cstring>

namespace led {

static void append32(std::vector<uint8_t>& out, uint32_t v) {
  out.push_back(v & 0xff); out.push_back((v >> 8) & 0xff);
  out.push_back((v >> 16) & 0xff); out.push_back((v >> 24) & 0xff);
}

static uint32_t read32(const std::vector<uint8_t>& b, size_t off) {
  return static_cast<uint32_t>(b[off]) | (static_cast<uint32_t>(b[off + 1]) << 8) |
         (static_cast<uint32_t>(b[off + 2]) << 16) | (static_cast<uint32_t>(b[off + 3]) << 24);
}

std::vector<uint8_t> encodePreviewChunk(const PreviewHeader& header,
                                        const std::vector<Rgb>& pixels,
                                        uint32_t firstPixel,
                                        uint32_t maxPixels) {
  std::vector<uint8_t> out;
  out.insert(out.end(), header.magic, header.magic + 4);
  out.push_back(header.version);
  append32(out, header.frameId);
  append32(out, static_cast<uint32_t>(header.timestampUs & 0xffffffffu));
  append32(out, header.pixelCount);
  append32(out, (static_cast<uint32_t>(header.chunkIndex) << 16) | header.chunkCount);
  append32(out, header.mappingHash);
  const uint32_t end = std::min<uint32_t>(pixels.size(), firstPixel + maxPixels);
  for (uint32_t i = firstPixel; i < end; ++i) {
    out.push_back(pixels[i].r); out.push_back(pixels[i].g); out.push_back(pixels[i].b);
  }
  return out;
}

bool decodePreviewChunk(const std::vector<uint8_t>& bytes, PreviewHeader& header,
                        std::vector<Rgb>& pixels) {
  if (bytes.size() < 25 || std::memcmp(bytes.data(), "SLPV", 4) != 0) return false;
  header.version = bytes[4];
  header.frameId = read32(bytes, 5);
  header.timestampUs = read32(bytes, 9);
  header.pixelCount = read32(bytes, 13);
  const uint32_t chunk = read32(bytes, 17);
  header.chunkIndex = static_cast<uint16_t>(chunk >> 16);
  header.chunkCount = static_cast<uint16_t>(chunk & 0xffff);
  header.mappingHash = read32(bytes, 21);
  pixels.clear();
  for (size_t i = 25; i + 2 < bytes.size(); i += 3) {
    pixels.push_back(Rgb{bytes[i], bytes[i + 1], bytes[i + 2]});
  }
  return true;
}

}  // namespace led
