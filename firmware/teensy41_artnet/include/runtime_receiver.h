#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace runtime_artnet {
constexpr unsigned kOutputs = 8;
constexpr unsigned kMaxLength = 1200;
constexpr unsigned kMaxBytes = kOutputs * kMaxLength * 3;
constexpr unsigned kMaxUniverses = 64;
struct Port { uint16_t universe, pixels; }; // pixels=0 disables a physical slot.
struct Counters {
    uint32_t packets=0, rejected=0, ignored=0, complete=0, incomplete=0;
    uint32_t stale=0, duplicate=0, overwritten=0;
};

class Receiver {
 public:
    Counters counts;
    bool configure(const Port (&ports)[kOutputs]) {
        Route next[kMaxUniverses];
        unsigned offsets[kOutputs] = {}, routes = 0, bytes = 0;
        for (unsigned i = 0; i < kOutputs; ++i) {
            offsets[i] = bytes / 3;
            if (ports[i].pixels > kMaxLength) return false;
            const unsigned universes = (unsigned(ports[i].pixels) + 169) / 170;
            if (universes && unsigned(ports[i].universe) + universes > 32768U) return false;
            for (unsigned u = 0; u < universes; ++u) {
                if (routes == kMaxUniverses) return false;
                const unsigned universe = ports[i].universe + u;
                for (unsigned j = 0; j < routes; ++j)
                    if (next[j].universe == universe) return false;
                unsigned length = (ports[i].pixels - u * 170U) * 3U;
                if (length > 510) length = 510;
                next[routes++] = {uint16_t(universe), uint16_t(bytes + u * 510U), uint16_t(length)};
            }
            bytes += ports[i].pixels * 3U;
        }
        memcpy(routes_, next, routes * sizeof(Route));
        memcpy(offsets_, offsets, sizeof(offsets));
        routeCount_ = routes;
        bytes_ = bytes;
        expected_ = routes == 64 ? ~uint64_t(0) : (uint64_t(1) << routes) - 1;
        clear();
        counts = {};
        lastAccepted_ = lastComplete_ = 0;
        return true;
    }
    unsigned universeCount() const { return routeCount_; }
    unsigned pixelOffset(unsigned slot) const { return slot < kOutputs ? offsets_[slot] : 0; }
    unsigned bytes() const { return bytes_; }
    uint64_t expectedMask() const { return expected_; }
    bool ingest(const uint8_t* p, size_t n, uint32_t now) {
        ++counts.packets;
        if (n < 18 || memcmp(p, "Art-Net\0", 8) || p[8] != 0 || p[9] != 0x50 ||
            (unsigned(p[10]) * 256 + p[11]) < 14) { ++counts.rejected; return false; }
        const unsigned length = unsigned(p[16]) * 256 + p[17];
        if (length < 2 || length > 512 || (length & 1) || n != 18 + length || (p[15] & 0x80)) {
            ++counts.rejected; return false;
        }
        const unsigned universe = p[14] + unsigned(p[15]) * 256;
        unsigned slot = 0;
        for (; slot < routeCount_; ++slot) if (routes_[slot].universe == universe) break;
        if (slot == routeCount_) { ++counts.ignored; return false; }
        const Route& route = routes_[slot];
        if (length < route.length) { ++counts.rejected; return false; }
        expire(now);
        const uint8_t seq = p[12];
        if (seq) {
            if (!haveSequence_ && mask_) abandon();
            if (haveSequence_ && seq != sequence_) {
                const unsigned delta = (unsigned(seq) + 255 - sequence_) % 255;
                if (delta > 127) { ++counts.stale; return false; }
                abandon();
            } else if (haveSequence_ && seq == sequence_ && sealed_) {
                ++counts.duplicate; return false;
            }
            if (!haveSequence_ || seq != sequence_) {
                sequence_ = seq; haveSequence_ = true; sealed_ = false;
            }
        } else {
            if (haveSequence_) abandon();
            haveSequence_ = false; sealed_ = false;
        }
        const uint64_t bit = uint64_t(1) << slot;
        if (mask_ & bit) {
            ++counts.duplicate;
            if (seq) return false;
            abandon();
        }
        if (!mask_) started_ = now;
        memcpy(staging_ + route.offset, p + 18, route.length);
        mask_ |= bit;
        lastAccepted_ = now;
        if (mask_ != expected_) return false;
        if (ready_) ++counts.overwritten;
        memcpy(complete_, staging_, bytes_);
        mask_ = 0; ready_ = true; sealed_ = bool(seq);
        ++counts.complete; lastComplete_ = now;
        return true;
    }
    bool take(uint8_t* target) {
        if (!ready_) return false;
        memcpy(target, complete_, bytes_); ready_ = false; return true;
    }
    void expire(uint32_t now) {
        if (mask_ && uint32_t(now - started_) > 100) abandon();
        if (uint32_t(now - lastAccepted_) > 1000) { haveSequence_ = false; sealed_ = false; }
    }
    void clear() { mask_ = 0; ready_ = false; haveSequence_ = false; sealed_ = false; }
    bool ready() const { return ready_; }
    uint32_t lastComplete() const { return lastComplete_; }
 private:
    struct Route { uint16_t universe, offset, length; };
    Route routes_[kMaxUniverses] = {};
    unsigned offsets_[kOutputs] = {}, routeCount_ = 0, bytes_ = 0;
    uint64_t expected_ = 0, mask_ = 0;
    uint8_t staging_[kMaxBytes] = {}, complete_[kMaxBytes] = {};
    uint32_t started_ = 0, lastAccepted_ = 0, lastComplete_ = 0;
    uint8_t sequence_ = 0;
    bool ready_ = false, haveSequence_ = false, sealed_ = false;
    void abandon() { if (mask_) ++counts.incomplete; mask_ = 0; }
};
} // namespace runtime_artnet
