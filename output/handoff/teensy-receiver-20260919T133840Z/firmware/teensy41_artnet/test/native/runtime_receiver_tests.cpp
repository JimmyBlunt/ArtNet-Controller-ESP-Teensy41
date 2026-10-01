#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include "../../include/runtime_receiver.h"

using runtime_artnet::Receiver;
using runtime_artnet::Port;
static std::vector<uint8_t> packet(unsigned universe, unsigned pixels, uint8_t seq, uint8_t value) {
    const unsigned used = pixels * 3;
    const unsigned length = used + (used & 1);
    std::vector<uint8_t> result(18 + length, value);
    memcpy(result.data(), "Art-Net\0", 8);
    result[8] = 0; result[9] = 0x50; result[10] = 0; result[11] = 14;
    result[12] = seq; result[13] = 0;
    result[14] = uint8_t(universe); result[15] = uint8_t(universe >> 8);
    result[16] = uint8_t(length >> 8); result[17] = uint8_t(length);
    return result;
}
static bool feed(Receiver& receiver, unsigned universe, unsigned count, unsigned seq, unsigned value, uint32_t now) {
    const auto bytes = packet(universe, count, uint8_t(seq), uint8_t(value));
    return receiver.ingest(bytes.data(), bytes.size(), now);
}

static void all64UniversesAndBounds() {
    Receiver receiver;
    Port ports[8] = {};
    for (unsigned i = 0; i < 8; ++i) ports[i] = {uint16_t(i == 7 ? 32760 : i * 1000), 1200};
    assert(receiver.configure(ports));
    assert(receiver.universeCount() == 64);
    assert(receiver.expectedMask() == ~uint64_t(0));
    assert(receiver.bytes() == 9600 * 3);
    for (unsigned i = 0; i < 8; ++i) {
        assert(receiver.pixelOffset(i) == i * 1200);
        for (unsigned u = 0; u < 8; ++u) {
            const bool complete = feed(receiver, ports[i].universe + u, u == 7 ? 10 : 170,
                                       255, i * 8 + u + 1, 10);
            assert(complete == (i == 7 && u == 7));
        }
    }
    std::vector<uint8_t> result(receiver.bytes() + 2, 0xA5);
    assert(receiver.take(result.data() + 1));
    assert(result.front() == 0xA5 && result.back() == 0xA5);
    for (unsigned i = 0; i < 8; ++i) {
        assert(result[1 + i * 3600] == i * 8 + 1);
        assert(result[1 + i * 3600 + 3599] == i * 8 + 8);
    }
    assert(!receiver.take(result.data() + 1));
    // Sequence 255 -> 1 is newer; the final frame replaces the unread previous one.
    for (unsigned seq = 1; seq <= 2; ++seq)
        for (unsigned i = 0; i < 8; ++i)
            for (unsigned u = 0; u < 8; ++u)
                feed(receiver, ports[i].universe + u, u == 7 ? 10 : 170, seq, seq + 90, 20 + seq);
    assert(receiver.counts.complete == 3 && receiver.counts.overwritten == 1);
    assert(receiver.take(result.data() + 1) && result[1] == 92 && result[receiver.bytes()] == 92);
    assert(!feed(receiver, 0, 170, 2, 0, 30)); // Sealed duplicate.
    assert(receiver.counts.duplicate == 1);
    assert(!feed(receiver, 0, 170, 1, 0, 31)); // Stale sequence.
    assert(receiver.counts.stale == 1);
}

static void partialLegacyAndPhysicalSlots() {
    Receiver receiver;
    Port ports[8] = {};
    ports[1] = {32767, 1}; ports[6] = {42, 171};
    assert(receiver.configure(ports));
    assert(receiver.universeCount() == 3 && receiver.bytes() == 172 * 3);
    assert(receiver.pixelOffset(1) == 0 && receiver.pixelOffset(6) == 1);
    feed(receiver, 32767, 1, 1, 11, 10);
    feed(receiver, 42, 170, 1, 12, 11);
    assert(!receiver.ready());
    receiver.expire(112);
    assert(receiver.counts.incomplete == 1 && !receiver.ready());
    feed(receiver, 43, 1, 2, 13, 120);
    feed(receiver, 32767, 1, 2, 14, 121);
    assert(feed(receiver, 42, 170, 2, 15, 122)); // Shuffled ordering.
    std::vector<uint8_t> result(receiver.bytes());
    assert(receiver.take(result.data()));
    assert(result[0] == 14 && result[3] == 15 && result.back() == 13);
    feed(receiver, 32767, 1, 0, 21, 130);
    feed(receiver, 32767, 1, 0, 22, 131); // Legacy repeated universe abandons old candidate.
    assert(!receiver.ready());
    feed(receiver, 42, 170, 0, 23, 132);
    assert(feed(receiver, 43, 1, 0, 24, 133));
    assert(receiver.take(result.data()) && result[0] == 22 && result.back() == 24);
    assert(receiver.counts.incomplete == 2 && receiver.counts.duplicate == 1);
}

static void invalidConfigPacketsAndEmpty() {
    Receiver receiver;
    Port ports[8] = {}; ports[0] = {32767, 1};
    assert(receiver.configure(ports));
    ports[1] = {32767, 1};
    assert(!receiver.configure(ports)); // Duplicate universe, previous config intact.
    assert(receiver.universeCount() == 1);
    ports[1] = {0, 1201}; assert(!receiver.configure(ports));
    ports[1] = {32767, 171}; assert(!receiver.configure(ports));
    auto data = packet(32767, 1, 1, 3);
    data[15] |= 0x80;
    assert(!receiver.ingest(data.data(), data.size(), 0));
    data = packet(32767, 1, 1, 3); data.pop_back();
    assert(!receiver.ingest(data.data(), data.size(), 1));
    data = packet(32767, 1, 1, 3); data[11] = 13;
    assert(!receiver.ingest(data.data(), data.size(), 2));
    assert(receiver.counts.rejected == 3);
    assert(!feed(receiver, 10, 1, 1, 0, 3) && receiver.counts.ignored == 1);
    memset(ports, 0, sizeof(ports));
    assert(receiver.configure(ports));
    assert(receiver.universeCount() == 0 && receiver.expectedMask() == 0 && receiver.bytes() == 0);
    assert(!feed(receiver, 0, 1, 1, 0, 4) && !receiver.ready());
}

int main() {
    all64UniversesAndBounds();
    partialLegacyAndPhysicalSlots();
    invalidConfigPacketsAndEmpty();
    puts("runtime_receiver_tests: passed (64 universes, bounds, slots, sequences, latest frame, validation)");
}
