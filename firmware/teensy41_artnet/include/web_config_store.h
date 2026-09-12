#pragma once
#include "web_config.h"
#ifdef ARDUINO
#include <EEPROM.h>
#endif

namespace webcfg {
enum class LoadResult { Defaults, Loaded, Corrupt };
namespace journal {
// Dedicated EEPROM bytes 0..247. On the pinned Teensy 4.1 core each group of four
// EEPROM bytes maps to sector (address >> 2) % 63. These 31-sector slots are
// physically disjoint, so erasing a destination sector cannot erase the old slot.
// Explicit encoding also avoids struct padding/ABI changes.
constexpr size_t kSlotSize = 124, kSlots = 2, kPayloadSize = 75;
constexpr size_t kPayloadAt = 12, kCrcAt = kPayloadAt + kPayloadSize, kCommitAt = kSlotSize - 1;
constexpr uint8_t kCommitted = 0xa5;
constexpr uint32_t kMagic = 0x3143544fUL;  // "OTC1": fixed PJRC Octo config journal.
constexpr uint16_t kVersion = 1;
inline void put16(uint8_t* p, uint16_t value) { p[0] = uint8_t(value); p[1] = uint8_t(value >> 8); }
inline uint16_t get16(const uint8_t* p) { return uint16_t(p[0]) | uint16_t(p[1]) << 8; }
inline void put32(uint8_t* p, uint32_t value) { for (unsigned i = 0; i < 4; ++i) p[i] = uint8_t(value >> (8 * i)); }
inline uint32_t get32(const uint8_t* p) {
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}
inline uint32_t crc32(const uint8_t* bytes, size_t length) {
    uint32_t crc = 0xffffffffUL;
    for (size_t i = 0; i < length; ++i) {
        crc ^= bytes[i];
        for (unsigned bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ (0xedb88320UL & (0U - (crc & 1U)));
    }
    return ~crc;
}
inline void encode(const Config& config, uint8_t* p) {
    *p++ = config.brightness; *p++ = config.targetFps; *p++ = config.dhcp ? 1 : 0;
    const uint8_t* addresses[] = {config.ip, config.netmask, config.gateway, config.dns};
    for (const auto* ip : addresses) {
        memcpy(p, ip, 4); p += 4;
    }
    for (const auto& output : config.outputs) {
        *p++ = output.enabled ? 1 : 0;
        put16(p, output.pixelCount); p += 2; put16(p, output.startUniverse); p += 2;
        *p++ = uint8_t(output.colorOrder); *p++ = output.reverse ? 1 : 0;
    }
}
inline bool decode(const uint8_t* p, Config& config) {
    config.brightness = *p++; config.targetFps = *p++;
    if (*p > 1) return false;
    config.dhcp = *p++ != 0;
    uint8_t* addresses[] = {config.ip, config.netmask, config.gateway, config.dns};
    for (auto* ip : addresses) { memcpy(ip, p, 4); p += 4; }
    for (auto& output : config.outputs) {
        if (*p > 1) return false;
        output.enabled = *p++ != 0;
        output.pixelCount = get16(p); p += 2; output.startUniverse = get16(p); p += 2;
        if (*p > 5) return false;
        output.colorOrder = static_cast<Order>(*p++);
        if (*p > 1) return false;
        output.reverse = *p++ != 0;
    }
    return validate(config, nullptr, 0);
}
struct Slot { bool valid = false, blank = true; uint32_t generation = 0; Config config; };
template<class Backend> Slot readSlot(Backend& eeprom, unsigned slot) {
    Slot result;
    uint8_t bytes[kSlotSize];
    for (size_t i = 0; i < kSlotSize; ++i) {
        bytes[i] = eeprom.read(int(slot * kSlotSize + i));
        if (bytes[i] != 0xff) result.blank = false;
    }
    if (bytes[kCommitAt] != kCommitted || get32(bytes) != kMagic ||
        get16(bytes + 4) != kVersion || get16(bytes + 6) != kPayloadSize ||
        get32(bytes + kCrcAt) != crc32(bytes, kCrcAt)) return result;
    result.generation = get32(bytes + 8);
    result.valid = decode(bytes + kPayloadAt, result.config);
    return result;
}
inline bool newer(uint32_t a, uint32_t b) { const uint32_t delta = a - b; return delta && delta < 0x80000000UL; }
inline int latest(const Slot& a, const Slot& b) {
    if (!a.valid) return b.valid ? 1 : -1;
    if (!b.valid) return 0;
    return newer(b.generation, a.generation) ? 1 : 0;
}
}  // namespace journal

// Backends supply length(), read(index), update(index,byte); tests use an in-memory EEPROM.
template<class Backend> LoadResult loadFrom(Backend& eeprom, Config& config) {
    config = defaults();
    if (size_t(eeprom.length()) < journal::kSlots * journal::kSlotSize) return LoadResult::Corrupt;
    const auto a = journal::readSlot(eeprom, 0), b = journal::readSlot(eeprom, 1);
    const int slot = journal::latest(a, b);
    if (slot < 0) return a.blank && b.blank ? LoadResult::Defaults : LoadResult::Corrupt;
    config = slot ? b.config : a.config;
    return LoadResult::Loaded;
}
template<class Backend> bool saveTo(Backend& eeprom, const Config& config) {
    if (!validate(config, nullptr, 0) || size_t(eeprom.length()) < journal::kSlots * journal::kSlotSize) return false;
    const auto a = journal::readSlot(eeprom, 0), b = journal::readSlot(eeprom, 1);
    const int current = journal::latest(a, b);
    if (current >= 0 && equal(current ? b.config : a.config, config)) return true;
    const unsigned next = current == 0 ? 1 : 0;
    const uint32_t generation = current < 0 ? 1U : (current ? b.generation : a.generation) + 1U;
    const size_t base = next * journal::kSlotSize;
    // Invalidate the destination before changing its body; the previous slot stays intact.
    eeprom.update(int(base + journal::kCommitAt), 0);
    if (eeprom.read(int(base + journal::kCommitAt)) != 0) return false;
    uint8_t bytes[journal::kCrcAt + 4] = {};
    journal::put32(bytes, journal::kMagic); journal::put16(bytes + 4, journal::kVersion);
    journal::put16(bytes + 6, journal::kPayloadSize); journal::put32(bytes + 8, generation);
    journal::encode(config, bytes + journal::kPayloadAt);
    journal::put32(bytes + journal::kCrcAt, journal::crc32(bytes, journal::kCrcAt));
    for (size_t i = 0; i < sizeof(bytes); ++i) eeprom.update(int(base + i), bytes[i]);
    for (size_t i = 0; i < sizeof(bytes); ++i) if (eeprom.read(int(base + i)) != bytes[i]) return false;
    eeprom.update(int(base + journal::kCommitAt), journal::kCommitted);  // Commit last.
    const auto committed = journal::readSlot(eeprom, next);
    return committed.valid && committed.generation == generation && equal(committed.config, config);
}
#ifdef ARDUINO
inline LoadResult load(Config& config) { return loadFrom(EEPROM, config); }
// Caller must first finish STOP and the black DMA; EEPROM writes may stall execution.
inline bool save(const Config& config) { return saveTo(EEPROM, config); }
#endif
}  // namespace webcfg
