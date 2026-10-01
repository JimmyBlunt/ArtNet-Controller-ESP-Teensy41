#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "esp_output_profile.h"
#include "../../include/board_profiles/PjrcOctoAdapterT41.h"

namespace webcfg {
constexpr unsigned kOutputs = 8;
constexpr unsigned kMaxLength = 1200;
constexpr unsigned kSchemaVersion = 1;
namespace board = artnet::boards::pjrc_octo_adapter_t41;

enum class Order : uint8_t { RGB, RBG, GRB, GBR, BRG, BGR };
struct Output {
    bool enabled = false;
    uint16_t pixelCount = 0, startUniverse = 0;
    Order colorOrder = Order::GRB;
    bool reverse = false;
};
struct Config {
    uint8_t brightness = 8, targetFps = 30;
    bool dhcp = true;
    uint8_t ip[4] = {}, netmask[4] = {255,255,255,0}, gateway[4] = {}, dns[4] = {};
    Output outputs[kOutputs];
};
inline bool fail(char* error, size_t capacity, const char* message) {
    if (error && capacity) snprintf(error, capacity, "%s", message);
    return false;
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
inline bool parseOrder(const char* value, Order& output) {
    if (!value) return false;
    for (unsigned i = 0; i < 6; ++i) {
        const auto order = static_cast<Order>(i);
        if (!strcmp(value, orderName(order))) { output = order; return true; }
    }
    return false;
}
inline Config defaults() {
    Config result;
    for (unsigned i = 0; i < kOutputs; ++i) {
        result.outputs[i].pixelCount = esp_profile::outputs[i].pixels;
        result.outputs[i].startUniverse = esp_profile::outputs[i].startUniverse;
        result.outputs[i].enabled = result.outputs[i].pixelCount != 0;
    }
    return result;
}
inline unsigned totalPixels(const Config& config) {
    unsigned total = 0;
    for (const auto& output : config.outputs) if (output.enabled) total += output.pixelCount;
    return total;
}
inline unsigned universeCount(const Config& config) {
    unsigned total = 0;
    for (const auto& output : config.outputs)
        if (output.enabled) total += (unsigned(output.pixelCount) + 169U) / 170U;
    return total;  // Validation rejects overlapping active ranges.
}
inline uint32_t ipv4(const uint8_t value[4]) {
    return uint32_t(value[0]) << 24 | uint32_t(value[1]) << 16 |
           uint32_t(value[2]) << 8 | value[3];
}
inline bool unicast(uint32_t ip) {
    return (ip >> 24) != 0 && (ip >> 24) != 127 && (ip >> 24) < 224;
}
inline bool parseIp(const char* text, uint8_t output[4]) {
    if (!text) return false;
    uint8_t parsed[4];
    const char* cursor = text;
    for (unsigned octet = 0; octet < 4; ++octet) {
        unsigned value = 0, digits = 0;
        while (*cursor >= '0' && *cursor <= '9') {
            if (++digits > 3) return false;
            value = value * 10 + unsigned(*cursor++ - '0');
            if (value > 255) return false;
        }
        if (!digits) return false;
        parsed[octet] = uint8_t(value);
        if (octet != 3) { if (*cursor++ != '.') return false; }
        else if (*cursor) return false;
    }
    memcpy(output, parsed, 4);
    return true;
}
inline void formatIp(const uint8_t ip[4], char output[16]) {
    snprintf(output, 16, "%u.%u.%u.%u", unsigned(ip[0]), unsigned(ip[1]), unsigned(ip[2]), unsigned(ip[3]));
}
inline bool validate(const Config& config, char* error, size_t cap) {
    if (error && cap) *error = 0;
    if (config.targetFps < 1 || config.targetFps > 60)
        return fail(error, cap, "targetFps must be 1..60");
    for (unsigned i = 0; i < kOutputs; ++i) {
        const auto& output = config.outputs[i];
        if (unsigned(output.colorOrder) >= 6) return fail(error, cap, "Invalid colorOrder");
        if (output.pixelCount > kMaxLength || (output.enabled && !output.pixelCount))
            return fail(error, cap, "Enabled outputs need 1..1200 pixels; disabled outputs allow zero");
        const unsigned end = unsigned(output.startUniverse) + (unsigned(output.pixelCount) + 169U) / 170U;
        if (output.startUniverse > 32767 || end > 32768)
            return fail(error, cap, "Output universe range exceeds 0..32767");
        if (!output.enabled) continue;
        for (unsigned previous = 0; previous < i; ++previous) {
            const auto& other = config.outputs[previous];
            if (!other.enabled) continue;
            const unsigned otherEnd = unsigned(other.startUniverse) + (unsigned(other.pixelCount) + 169U) / 170U;
            if (output.startUniverse < otherEnd && other.startUniverse < end)
                return fail(error, cap, "Active output universe ranges overlap");
        }
    }
    if (!config.dhcp) {
        const uint32_t ip = ipv4(config.ip), mask = ipv4(config.netmask);
        const uint32_t hostMask = ~mask, gateway = ipv4(config.gateway), dns = ipv4(config.dns);
        // A normal Ethernet subnet needs network and broadcast addresses plus hosts.
        if (!mask || hostMask < 3 || (hostMask & (hostMask + 1U)))
            return fail(error, cap, "Static netmask must be contiguous /1../30");
        if (!unicast(ip) || !(ip & hostMask) || (ip & hostMask) == hostMask)
            return fail(error, cap, "Static IP must be a unicast host address");
        if (gateway && (!unicast(gateway) || (gateway & mask) != (ip & mask) ||
                        !(gateway & hostMask) || (gateway & hostMask) == hostMask || gateway == ip))
            return fail(error, cap, "Gateway must be zero or another host in the same subnet");
        if (dns && !unicast(dns)) return fail(error, cap, "DNS must be zero or a unicast address");
    }
    return true;
}
inline bool equal(const Config& a, const Config& b) {
    if (a.brightness != b.brightness || a.targetFps != b.targetFps || a.dhcp != b.dhcp ||
        memcmp(a.ip, b.ip, 4) || memcmp(a.netmask, b.netmask, 4) ||
        memcmp(a.gateway, b.gateway, 4) || memcmp(a.dns, b.dns, 4)) return false;
    for (unsigned i = 0; i < kOutputs; ++i) {
        const auto& x = a.outputs[i]; const auto& y = b.outputs[i];
        if (x.enabled != y.enabled || x.pixelCount != y.pixelCount || x.startUniverse != y.startUniverse ||
            x.colorOrder != y.colorOrder || x.reverse != y.reverse) return false;
    }
    return true;
}
}  // namespace webcfg
