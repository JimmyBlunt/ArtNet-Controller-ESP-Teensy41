#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace octo_test {

constexpr unsigned kOutputs = 8;
constexpr unsigned kMaxLength = 1200;
constexpr uint32_t kPeriodUs = 33334;
constexpr uint32_t kTestDurationMs = 30000;
constexpr unsigned kMaxDurationSeconds = 600;
constexpr uint8_t kBrightness = 8;

enum class Order { RGB, RBG, GRB, GBR, BRG, BGR };
struct Config {
    Order order = Order::GRB;
    uint16_t lengths[kOutputs] = {};
};
inline bool sameConfig(const Config& a, const Config& b) {
    if (a.order != b.order) return false;
    for (unsigned i = 0; i < kOutputs; ++i)
        if (a.lengths[i] != b.lengths[i]) return false;
    return true;
}
inline const char* orderName(Order order) {
    switch (order) {
        case Order::RGB: return "RGB";
        case Order::RBG: return "RBG";
        case Order::GRB: return "GRB";
        case Order::GBR: return "GBR";
        case Order::BRG: return "BRG";
        case Order::BGR: return "BGR";
    }
    return "INVALID";
}

enum class CommandKind { Invalid, Status, Stop, Crash, Arm, Configure, Test };
struct Command {
    CommandKind kind = CommandKind::Invalid;
    Config config;
    unsigned output = 0;  // 0 = ALL; 1..8 remain physical IDs.
    uint32_t durationMs = kTestDurationMs;
};

// Mutates only the supplied command line. Strict token and decimal validation
// avoids accepting truncation, trailing arguments, negative lengths or overflow.
inline Command parse(char* line) {
    char* words[12] = {};
    unsigned count = 0;
    char* cursor = line;
    while (*cursor) {
        while (*cursor == ' ' || *cursor == '\t') ++cursor;
        if (!*cursor) break;
        if (count == 12) return {};
        words[count++] = cursor;
        while (*cursor && *cursor != ' ' && *cursor != '\t') ++cursor;
        if (*cursor) *cursor++ = 0;
    }
    Command result;
    if (!count) return result;
    if (count == 1) {
        if (!strcmp(words[0], "STATUS")) result.kind = CommandKind::Status;
        else if (!strcmp(words[0], "STOP")) result.kind = CommandKind::Stop;
        else if (!strcmp(words[0], "CRASH")) result.kind = CommandKind::Crash;
        else if (!strcmp(words[0], "ARM")) result.kind = CommandKind::Arm;
        return result;
    }
    if ((count == 2 || count == 3) && !strcmp(words[0], "TEST")) {
        if (!strcmp(words[1], "ALL")) result.kind = CommandKind::Test;
        else if (words[1][0] >= '1' && words[1][0] <= '8' && !words[1][1]) {
            result.kind = CommandKind::Test;
            result.output = words[1][0] - '0';
        }
        if (count == 3) {
            unsigned seconds = 0;
            for (const char* p = words[2]; *p; ++p) {
                if (*p < '0' || *p > '9') return {};
                seconds = seconds * 10 + unsigned(*p - '0');
                if (seconds > kMaxDurationSeconds) return {};
            }
            if (!seconds) return {};
            result.durationMs = seconds * 1000U;
        }
        return result;
    }
    if (count != 11 || strcmp(words[0], "CONFIG") || strcmp(words[1], "WS2812B"))
        return result;
    bool orderFound = false;
    const Order orders[] = {Order::RGB, Order::RBG, Order::GRB,
                            Order::GBR, Order::BRG, Order::BGR};
    for (Order order : orders) {
        if (!strcmp(words[2], orderName(order))) {
            result.config.order = order;
            orderFound = true;
        }
    }
    if (!orderFound) return result;
    unsigned total = 0;
    for (unsigned i = 0; i < kOutputs; ++i) {
        unsigned value = 0;
        for (const char* p = words[i + 3]; *p; ++p) {
            if (*p < '0' || *p > '9') return {};
            value = value * 10 + unsigned(*p - '0');
            if (value > kMaxLength) return {};
        }
        result.config.lengths[i] = uint16_t(value);
        total += value;
    }
    if (!total) return {};
    result.kind = CommandKind::Configure;
    return result;
}

inline bool selected(unsigned testOutput, unsigned physicalOutput, const Config& config) {
    return physicalOutput >= 1 && physicalOutput <= kOutputs &&
        config.lengths[physicalOutput - 1] != 0 &&
        (testOutput == 0 || testOutput == physicalOutput);
}
// 2.8 s per colour: physical output N has N pulses, then a common gap.
// Works for a one-pixel test lead, independently of the configured chain length.
inline bool pulseLit(unsigned physicalOutput, uint32_t elapsedMs) {
    const uint32_t phase = elapsedMs % 2800;
    return physicalOutput >= 1 && physicalOutput <= kOutputs &&
        phase < physicalOutput * 300 && phase % 300 < 150;
}
inline unsigned colorIndex(uint32_t elapsedMs) { return (elapsedMs / 2800) % 3; }

enum class State { Dormant, Running, RunningArtNet, WaitBlackout, WaitBlackoutCompletion, Stopped };
inline const char* stateName(State state) {
    switch (state) {
        case State::Dormant: return "DISARMED";
        case State::Running: return "TEST_RUNNING";
        case State::RunningArtNet: return "ARTNET_RUNNING";
        case State::WaitBlackout: return "STOP_WAIT_PREVIOUS_DMA";
        case State::WaitBlackoutCompletion: return "STOP_WAIT_BLACK_DMA";
        case State::Stopped: return "STOPPED";
    }
    return "INVALID";
}
inline bool stopping(State state) {
    return state == State::WaitBlackout || state == State::WaitBlackoutCompletion;
}
inline bool running(State state) {
    return state == State::Running || state == State::RunningArtNet;
}
inline State requestStop(State state) {
    return running(state) ? State::WaitBlackout : state;
}
// Blackout starts only after the prior transfer is safe; acknowledgement starts
// only after a separately submitted blackout finishes, including its latch guard.
inline State advanceStop(State state, bool transferReady) {
    if (!transferReady) return state;
    if (state == State::WaitBlackout) return State::WaitBlackoutCompletion;
    if (state == State::WaitBlackoutCompletion) return State::Stopped;
    return state;
}

}  // namespace octo_test
