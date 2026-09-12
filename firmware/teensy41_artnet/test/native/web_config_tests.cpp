#include <cassert>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <set>
#include <vector>
#include "../../include/web_config_store.h"

struct MemoryEEPROM {
    std::vector<uint8_t> bytes = std::vector<uint8_t>(256, 0xff);
    int remaining = -1;
    unsigned writes = 0;
    int length() const { return int(bytes.size()); }
    uint8_t read(int index) const { return bytes.at(size_t(index)); }
    void update(int index, uint8_t value) {
        if (remaining == 0) throw std::runtime_error("power cut");
        if (remaining > 0) --remaining;
        ++writes;
        bytes.at(size_t(index)) = value;
    }
};

static void configurationValidation() {
    auto config = webcfg::defaults();
    char error[160];
    assert(webcfg::validate(config, error, sizeof(error)));
    assert(webcfg::totalPixels(config) == 4031 && webcfg::universeCount(config) == 29);
    assert(config.brightness == 8 && config.targetFps == 30 && config.dhcp);
    assert(config.outputs[5].pixelCount == 536 && config.outputs[6].pixelCount == 512);
    assert(config.outputs[5].startUniverse == 142 && config.outputs[6].startUniverse == 146);
    assert(!config.outputs[7].enabled);
    auto copy = config;
    assert(webcfg::equal(config, copy)); copy.outputs[0].reverse = true;
    assert(!webcfg::equal(config, copy));
    copy = config; copy.targetFps = 0; assert(!webcfg::validate(copy, error, sizeof(error)));
    copy.targetFps = 61; assert(!webcfg::validate(copy, error, sizeof(error)));
    copy = config; copy.outputs[0].pixelCount = 1201; assert(!webcfg::validate(copy, error, sizeof(error)));
    copy = config; copy.outputs[0].pixelCount = 0; assert(!webcfg::validate(copy, error, sizeof(error)));
    copy = config; copy.outputs[0].colorOrder = static_cast<webcfg::Order>(6);
    assert(!webcfg::validate(copy, error, sizeof(error)));
    copy = config; copy.outputs[6].startUniverse = 145;
    assert(!webcfg::validate(copy, error, sizeof(error))); // The 26-pixel tail still occupies U145.
    for (auto& output : copy.outputs) output.enabled = false;
    assert(webcfg::validate(copy, error, sizeof(error)));
    assert(webcfg::totalPixels(copy) == 0 && webcfg::universeCount(copy) == 0);
    copy.outputs[0].enabled = true; copy.outputs[0].pixelCount = 170; copy.outputs[0].startUniverse = 32767;
    assert(webcfg::validate(copy, error, sizeof(error)));
    copy.outputs[0].pixelCount = 171; assert(!webcfg::validate(copy, error, sizeof(error)));
    copy.outputs[0].pixelCount = 1; copy.outputs[0].startUniverse = 32768;
    assert(!webcfg::validate(copy, error, sizeof(error)));
    char bounded[3] = {'x','x','x'};
    assert(!webcfg::validate(copy, bounded, 2) && bounded[1] == 0 && bounded[2] == 'x');
    assert(!webcfg::validate(copy, nullptr, 0));

    for (const char* invalid : {"", "1.2.3", "1.2.3.4.5", "256.1.1.1", "1.-2.3.4", "1.2.3.4 ",
                                " 1.2.3.4", "1..3.4", "9999999999999.1.1.1", "1e2.1.1.1"}) {
        uint8_t ip[4] = {9,8,7,6};
        assert(!webcfg::parseIp(invalid, ip)); assert(ip[0] == 9 && ip[3] == 6);
    }
    copy = config; copy.dhcp = false;
    assert(webcfg::parseIp("192.168.1.50", copy.ip));
    assert(webcfg::parseIp("255.255.255.0", copy.netmask));
    assert(webcfg::parseIp("192.168.1.1", copy.gateway));
    assert(webcfg::parseIp("1.1.1.1", copy.dns));
    assert(webcfg::validate(copy, error, sizeof(error)));
    const auto staticConfig = copy;
    for (const char* invalid : {"0.0.0.0", "127.0.0.1", "224.0.0.1", "192.168.1.0", "192.168.1.255"}) {
        copy = staticConfig; assert(webcfg::parseIp(invalid, copy.ip));
        assert(!webcfg::validate(copy, error, sizeof(error)));
    }
    for (const char* invalid : {"0.0.0.0", "255.0.255.0", "255.255.255.254", "255.255.255.255"}) {
        copy = staticConfig; assert(webcfg::parseIp(invalid, copy.netmask));
        assert(!webcfg::validate(copy, error, sizeof(error)));
    }
    for (const char* invalid : {"192.168.2.1", "192.168.1.0", "192.168.1.255", "192.168.1.50"}) {
        copy = staticConfig; assert(webcfg::parseIp(invalid, copy.gateway));
        assert(!webcfg::validate(copy, error, sizeof(error)));
    }
    copy = staticConfig; memset(copy.gateway, 0, 4); memset(copy.dns, 0, 4);
    assert(webcfg::validate(copy, error, sizeof(error)));
}

static void rewriteGeneration(MemoryEEPROM& storage, unsigned slot, uint32_t generation) {
    auto* bytes = storage.bytes.data() + slot * webcfg::journal::kSlotSize;
    webcfg::journal::put32(bytes + 8, generation);
    webcfg::journal::put32(bytes + webcfg::journal::kCrcAt,
                         webcfg::journal::crc32(bytes, webcfg::journal::kCrcAt));
}
static void persistenceJournal() {
    using webcfg::LoadResult;
    const auto original = webcfg::defaults();
    auto second = original; second.brightness = 17; second.outputs[3].reverse = true;
    auto third = second; third.brightness = 33;
    webcfg::Config loaded;
    MemoryEEPROM empty;
    std::set<unsigned> firstSectors, secondSectors;
    for (unsigned address = 0; address < webcfg::journal::kSlotSize; ++address) {
        firstSectors.insert((address >> 2) % 63);
        secondSectors.insert(((address + webcfg::journal::kSlotSize) >> 2) % 63);
    }
    assert(firstSectors.size() == 31 && secondSectors.size() == 31);
    for (unsigned sector : firstSectors) assert(!secondSectors.count(sector));
    assert(webcfg::loadFrom(empty, loaded) == LoadResult::Defaults && webcfg::equal(original, loaded));
    assert(empty.writes == 0);
    assert(webcfg::journal::crc32(reinterpret_cast<const uint8_t*>("123456789"), 9) == 0xcbf43926UL);
    assert(webcfg::saveTo(empty, original));
    assert(webcfg::loadFrom(empty, loaded) == LoadResult::Loaded && webcfg::equal(original, loaded));
    const unsigned savedWrites = empty.writes;
    assert(webcfg::saveTo(empty, original) && empty.writes == savedWrites);
    auto everyField = original;
    everyField.brightness = 255; everyField.targetFps = 60; everyField.dhcp = false;
    assert(webcfg::parseIp("192.168.50.42", everyField.ip));
    assert(webcfg::parseIp("255.255.255.0", everyField.netmask));
    assert(webcfg::parseIp("192.168.50.1", everyField.gateway));
    assert(webcfg::parseIp("9.9.9.9", everyField.dns));
    for (unsigned i = 0; i < 8; ++i) {
        everyField.outputs[i].colorOrder = static_cast<webcfg::Order>(i % 6);
        everyField.outputs[i].reverse = (i % 2) != 0;
    }
    everyField.outputs[7].enabled = true;
    everyField.outputs[7].pixelCount = 1200; everyField.outputs[7].startUniverse = 32000;
    MemoryEEPROM fullRoundTrip;
    assert(webcfg::saveTo(fullRoundTrip, everyField));
    assert(webcfg::loadFrom(fullRoundTrip, loaded) == LoadResult::Loaded && webcfg::equal(everyField, loaded));
    assert(webcfg::saveTo(empty, second));
    assert(webcfg::loadFrom(empty, loaded) == LoadResult::Loaded && webcfg::equal(second, loaded));
    const auto twoSlots = empty;
    // Cut power at every EEPROM update boundary, including marker invalidation and final commit.
    for (int cut = 0; cut < 100; ++cut) {
        auto torn = twoSlots; torn.remaining = cut;
        try { webcfg::saveTo(torn, third); } catch (const std::runtime_error&) {}
        assert(webcfg::loadFrom(torn, loaded) == LoadResult::Loaded);
        assert(webcfg::equal(loaded, second) || webcfg::equal(loaded, third));
    }
    auto corrupt = twoSlots;
    corrupt.bytes[webcfg::journal::kSlotSize + webcfg::journal::kPayloadAt] ^= 0x10;
    assert(webcfg::loadFrom(corrupt, loaded) == LoadResult::Loaded && webcfg::equal(original, loaded));
    corrupt.bytes[webcfg::journal::kPayloadAt] ^= 0x10;
    assert(webcfg::loadFrom(corrupt, loaded) == LoadResult::Corrupt && webcfg::equal(original, loaded));
    auto wrongVersion = twoSlots;
    for (unsigned slot = 0; slot < 2; ++slot) {
        auto* bytes = wrongVersion.bytes.data() + slot * webcfg::journal::kSlotSize;
        webcfg::journal::put16(bytes + 4, 2);
        webcfg::journal::put32(bytes + webcfg::journal::kCrcAt,
                             webcfg::journal::crc32(bytes, webcfg::journal::kCrcAt));
    }
    assert(webcfg::loadFrom(wrongVersion, loaded) == LoadResult::Corrupt);
    auto invalidPayload = twoSlots;
    auto* bytes = invalidPayload.bytes.data() + webcfg::journal::kSlotSize;
    bytes[webcfg::journal::kPayloadAt + 2] = 2; // Invalid boolean even with matching CRC.
    webcfg::journal::put32(bytes + webcfg::journal::kCrcAt,
                         webcfg::journal::crc32(bytes, webcfg::journal::kCrcAt));
    assert(webcfg::loadFrom(invalidPayload, loaded) == LoadResult::Loaded && webcfg::equal(original, loaded));
    auto wrap = twoSlots;
    rewriteGeneration(wrap, 0, UINT32_MAX);
    rewriteGeneration(wrap, 1, 0);
    assert(webcfg::loadFrom(wrap, loaded) == LoadResult::Loaded && webcfg::equal(second, loaded));
    assert(webcfg::saveTo(wrap, third));
    assert(webcfg::journal::readSlot(wrap, 0).generation == 1);
    assert(webcfg::loadFrom(wrap, loaded) == LoadResult::Loaded && webcfg::equal(third, loaded));
    auto invalid = second; invalid.targetFps = 0;
    const auto snapshot = twoSlots.bytes;
    assert(!webcfg::saveTo(empty, invalid) && empty.bytes == snapshot);
    MemoryEEPROM tooSmall; tooSmall.bytes.resize(webcfg::journal::kSlots * webcfg::journal::kSlotSize - 1);
    assert(!webcfg::saveTo(tooSmall, original));
    assert(webcfg::loadFrom(tooSmall, loaded) == LoadResult::Corrupt);
    MemoryEEPROM tornFirst; tornFirst.remaining = 30;
    try { webcfg::saveTo(tornFirst, original); } catch (const std::runtime_error&) {}
    assert(webcfg::loadFrom(tornFirst, loaded) == LoadResult::Corrupt && webcfg::equal(original, loaded));
}
int main() {
    configurationValidation();
    persistenceJournal();
    std::cout << "Web config validation and CRC journal passed (100 interrupted-write boundaries); no hardware writes\n";
}
