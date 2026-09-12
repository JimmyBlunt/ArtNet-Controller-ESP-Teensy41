#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "../../include/octo_test_core.h"
#include "../../include/receiver_core.h"
#include "../../include/esp_output_profile.h"
#include "../../../include/board_profiles/PjrcOctoAdapterT41.h"

namespace test = octo_test;
namespace board = artnet::boards::pjrc_octo_adapter_t41;

static test::Command parse(const std::string& input) {
    std::vector<char> writable(input.begin(), input.end());
    writable.push_back(0);
    return test::parse(writable.data());
}

static void parserBoundaries() {
    using Kind = test::CommandKind;
    const std::vector<std::string> invalid = {
        "", " \t ", "status", "STOP NOW", "ARM 1", "TEST", "TEST 0",
        "TEST 9", "TEST -1", "TEST +1", "TEST 01", "TEST 1x", "TEST all",
        "TEST ALL 0", "TEST ALL 601", "TEST ALL -1", "TEST ALL +1",
        "TEST ALL 1.0", "TEST ALL 1e2", "TEST ALL 4294967297",
        "TEST ALL 18446744073709551616", "TEST 1 2 extra", "TEST ALL 1\nARM",
        "CONFIG WS2812B GRB 0 0 0 0 0 0 0 0",
        "CONFIG APA102 GRB 1 1 1 1 1 1 1 1",
        "CONFIG WS2812B INVALID 1 1 1 1 1 1 1 1",
        "CONFIG WS2812B GRB 1 1 1 1 1 1 1",
        "CONFIG WS2812B GRB 1 1 1 1 1 1 1 1 1",
        "CONFIG WS2812B GRB -1 0 0 0 0 0 0 0",
        "CONFIG WS2812B GRB +1 0 0 0 0 0 0 0",
        "CONFIG WS2812B GRB 1.0 0 0 0 0 0 0 0",
        "CONFIG WS2812B GRB 4294967297 0 0 0 0 0 0 0",
        "CONFIG WS2812B GRB 999999999999999999999999999 0 0 0 0 0 0 0",
        "a b c d e f g h i j k l m"
    };
    for (const auto& input : invalid) assert(parse(input).kind == Kind::Invalid);
    assert(parse("CONFIG WS2812B GRB " + std::to_string(test::kMaxLength + 1) +
                 " 0 0 0 0 0 0 0").kind == Kind::Invalid);
    assert(parse("\t STATUS  \t").kind == Kind::Status);
    assert(parse("STOP").kind == Kind::Stop);
    assert(parse("CRASH").kind == Kind::Crash);
    assert(parse("ARM").kind == Kind::Arm);
    for (unsigned number = 1; number <= 8; ++number) {
        const auto command = parse("TEST " + std::to_string(number));
        assert(command.kind == Kind::Test && command.output == number);
        assert(command.durationMs == 30000);
    }
    for (unsigned seconds = 1; seconds <= 600; ++seconds) {
        const auto command = parse("TEST ALL " + std::to_string(seconds));
        assert(command.kind == Kind::Test && command.output == 0);
        assert(command.durationMs == seconds * 1000U);
    }
    const std::vector<std::string> orders = {"RGB", "RBG", "GRB", "GBR", "BRG", "BGR"};
    for (const auto& order : orders) {
        auto command = parse("CONFIG WS2812B " + order + " 0 1 880 0 7 0 0 8");
        assert(command.kind == Kind::Configure);
        assert(test::orderName(command.config.order) == order);
        assert(command.config.lengths[0] == 0 && command.config.lengths[2] == 880);
        assert(command.config.lengths[7] == 8);
        auto identical = command.config;
        assert(test::sameConfig(command.config, identical));
        identical.lengths[0] = 1;
        assert(!test::sameConfig(command.config, identical));
    }
    auto first = parse("CONFIG WS2812B RGB 1 0 0 0 0 0 0 0").config;
    auto second = parse("CONFIG WS2812B GRB 1 0 0 0 0 0 0 0").config;
    assert(!test::sameConfig(first, second));
    std::string maximum = "CONFIG WS2812B GRB";
    for (unsigned output = 0; output < 8; ++output)
        maximum += " " + std::to_string(test::kMaxLength);
    const auto maxCommand = parse(maximum);
    assert(maxCommand.kind == Kind::Configure);
    for (auto length : maxCommand.config.lengths) assert(length == test::kMaxLength);
}

static void physicalIdentityAndPatterns() {
    const auto config = parse("CONFIG WS2812B GRB 0 1 0 0 2 0 0 8").config;
    const unsigned expectedPins[] = {2, 14, 7, 8, 6, 20, 21, 5};
    for (unsigned output = 1; output <= 8; ++output) {
        const auto* physical = board::findOutput(output);
        assert(physical && physical->number == output);
        assert(physical->teensyPin == expectedPins[output - 1]);
        assert(board::matchesOutputPin(output, expectedPins[output - 1]));
        const bool active = output == 2 || output == 5 || output == 8;
        assert(test::selected(0, output, config) == active);
        assert(test::selected(output, output, config) == active);
        assert(!test::selected(9, output, config));
        for (unsigned other = 1; other <= 8; ++other)
            assert(test::selected(other, output, config) == (active && other == output));
        unsigned pulses = 0;
        bool wasLit = false;
        for (unsigned time = 0; time < 2800; ++time) {
            const bool lit = test::pulseLit(output, time);
            if (lit && !wasLit) ++pulses;
            wasLit = lit;
        }
        assert(pulses == output);
        assert(!test::pulseLit(output, 2799));
        assert(test::pulseLit(output, 2800));
    }
    assert(!board::findOutput(0) && !board::findOutput(9));
    assert(!board::findOutput(UINT32_MAX));
    assert(!board::matchesOutputPin(2, 2));
    assert(!test::selected(0, 0, config) && !test::selected(0, 9, config));
    assert(!test::pulseLit(0, 0) && !test::pulseLit(9, 0));
    assert(test::colorIndex(0) == 0 && test::colorIndex(2799) == 0);
    assert(test::colorIndex(2800) == 1 && test::colorIndex(5600) == 2);
    assert(test::colorIndex(8400) == 0);
}

static void stopTransitions() {
    using State = test::State;
    assert(test::requestStop(State::Dormant) == State::Dormant);
    assert(test::requestStop(State::RunningArtNet) == State::WaitBlackout);
    assert(test::running(State::RunningArtNet) && test::running(State::Running));
    assert(!test::running(State::Stopped) && !test::running(State::WaitBlackout));
    auto state = test::requestStop(State::Running);
    assert(state == State::WaitBlackout && test::stopping(state));
    for (unsigned poll = 0; poll < 100; ++poll) {
        assert(test::requestStop(state) == state);
        state = test::advanceStop(state, false);
        assert(state == State::WaitBlackout);
    }
    state = test::advanceStop(state, true);
    assert(state == State::WaitBlackoutCompletion && test::stopping(state));
    for (unsigned poll = 0; poll < 100; ++poll) {
        assert(test::requestStop(state) == state);
        state = test::advanceStop(state, false);
        assert(state == State::WaitBlackoutCompletion);
    }
    state = test::advanceStop(state, true);
    assert(state == State::Stopped && !test::stopping(state));
    assert(test::requestStop(state) == State::Stopped);
    assert(test::advanceStop(state, false) == State::Stopped);
    assert(test::advanceStop(state, true) == State::Stopped);
    assert(test::advanceStop(State::Running, true) == State::Running);
}

#ifdef OCTO_ESP_PROFILE
static std::vector<uint8_t> datagram(unsigned universe, unsigned length = 510,
                                     unsigned sequence = 1) {
    std::vector<uint8_t> packet(18 + length, uint8_t(universe));
    memcpy(packet.data(), "Art-Net\0", 8);
    packet[8] = 0; packet[9] = 0x50; packet[10] = 0; packet[11] = 14;
    packet[12] = uint8_t(sequence); packet[13] = 0;
    packet[14] = uint8_t(universe); packet[15] = uint8_t(universe >> 8);
    packet[16] = uint8_t(length >> 8); packet[17] = uint8_t(length);
    return packet;
}

static void esp4031Routing() {
    static_assert(artnet::kPixels == 4031 && artnet::kPorts == 7, "Explicit ESP4031 split");
    static_assert(artnet::kBytes == 12093, "No historical 4105-pixel tail");
    const unsigned counts[] = {203, 738, 880, 810, 352, 536, 512, 0};
    const unsigned universes[] = {120, 122, 127, 133, 139, 142, 146};
    const unsigned offsets[] = {0, 203, 941, 1821, 2631, 2983, 3519};
    test::Config config;
    for (unsigned i = 0; i < 8; ++i) {
        assert(esp_profile::outputs[i].physicalOutput == i + 1);
        assert(esp_profile::outputs[i].pixels == counts[i]);
        config.lengths[i] = esp_profile::outputs[i].pixels;
        if (i < 7) {
            assert(artnet::ports[i].universe == universes[i]);
            assert(artnet::ports[i].pixels == counts[i]);
            assert(artnet::ports[i].offset == offsets[i]);
        }
    }
    assert(test::selected(0, 6, config));
    assert(test::selected(0, 7, config) && !test::selected(0, 8, config));
    assert(test::selected(7, 7, config) && !test::selected(8, 8, config));
    assert(parse("CONFIG WS2812B GRB 203 738 880 810 352 536 512 0").kind == test::CommandKind::Configure);

    // Distinct values in every universe expose incorrect output offsets and tail copies.
    artnet::Receiver receiver;
    for (unsigned u = 120; u <= 149; ++u) {
        if (u == 138) continue;
        if (u == 149) assert(!receiver.ready() && receiver.counts.complete == 0);
        auto packet = datagram(u, u == 145 ? 78 : (u == 149 ? 6 : 510));
        receiver.ingest(packet.data(), packet.size(), 100);
    }
    assert(receiver.counts.packets == 29 && receiver.counts.complete == 1 && receiver.counts.rejected == 0);
    std::vector<uint8_t> protectedOutput(artnet::kBytes + 2, 0xa5);
    assert(receiver.take(protectedOutput.data() + 1));
    assert(protectedOutput.front() == 0xa5 && protectedOutput.back() == 0xa5);
    for (auto port : artnet::ports) {
        for (unsigned pixel = 0; pixel < port.pixels; ++pixel) {
            for (unsigned channel = 0; channel < 3; ++channel)
                assert(protectedOutput[1 + (port.offset + pixel) * 3 + channel] ==
                       port.universe + pixel / 170);
        }
    }
    assert(protectedOutput[1 + 3492 * 3 + 2] == 144);
    assert(protectedOutput[1 + 3493 * 3] == 145);
    assert(protectedOutput[1 + 3518 * 3 + 2] == 145);
    assert(protectedOutput[1 + 3519 * 3] == 146);
    assert(protectedOutput[1 + 4028 * 3 + 2] == 148);
    assert(protectedOutput[1 + 4029 * 3] == 149);
    assert(protectedOutput[1 + 4030 * 3 + 2] == 149);
    assert(!receiver.take(protectedOutput.data() + 1));

    // OUT6's tail is 26 RGB pixels in U145; OUT7 adds two final RGB pixels in U149.
    artnet::Receiver tail;
    auto shortTail = datagram(145, 76);
    assert(!tail.ingest(shortTail.data(), shortTail.size(), 10));
    shortTail = datagram(149, 4);
    assert(!tail.ingest(shortTail.data(), shortTail.size(), 10));
    assert(tail.counts.rejected == 2 && !tail.ready());
    for (unsigned u = 120; u <= 148; ++u) {
        if (u == 138) continue;
        auto packet = datagram(u, u == 145 ? 78 : 510);
        tail.ingest(packet.data(), packet.size(), 20);
    }
    assert(!tail.ready());
    auto final = datagram(149, 6);
    assert(tail.ingest(final.data(), final.size(), 21));
    assert(tail.counts.complete == 1 && tail.take(protectedOutput.data() + 1));
    for (unsigned u : {0U, 119U, 138U, 150U, 32767U}) {
        auto ignored = datagram(u);
        assert(!tail.ingest(ignored.data(), ignored.size(), 22));
    }
    assert(tail.counts.ignored == 5);

    // Receiver expiry, which also services Ethernet loop loss handling, crosses millis rollover.
    artnet::Receiver rollover;
    auto partial = datagram(120);
    assert(!rollover.ingest(partial.data(), partial.size(), UINT32_MAX - 50));
    rollover.expire(51);
    assert(rollover.counts.incomplete == 1);
    for (unsigned u = 120; u <= 149; ++u) {
        if (u == 138) continue;
        auto packet = datagram(u, 510, 2);
        rollover.ingest(packet.data(), packet.size(), 60);
    }
    assert(rollover.counts.complete == 1 && rollover.ready());
    rollover.clear();
    assert(!rollover.ready());
}
#endif

int main() {
    parserBoundaries();
    physicalIdentityAndPatterns();
    stopTransitions();
#ifdef OCTO_ESP_PROFILE
    esp4031Routing();
#endif
    std::cout << "Octo parser, physical identity, pulse identification and STOP transition tests passed";
#ifdef OCTO_ESP_PROFILE
    std::cout << "; ESP4031 routing passed: 7 active outputs, 29 universes, 4031 pixels";
#endif
    std::cout << '\n';
}
