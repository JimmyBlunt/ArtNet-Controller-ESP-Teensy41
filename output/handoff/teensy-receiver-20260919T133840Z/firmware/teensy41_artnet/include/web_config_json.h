#pragma once
#include <ArduinoJson.h>
#include "web_config.h"

namespace webcfg {
// Capacity for the fixed eight-output document, including copied network strings.
constexpr size_t kJsonCapacity = 8192;
inline void toJson(const Config& config, JsonObject object) {
    object["schemaVersion"] = kSchemaVersion;
    object["hardwareProfile"] = board::kId;
    object["brightness"] = config.brightness;
    object["targetFps"] = config.targetFps;
    object["pixelCount"] = totalPixels(config);
    object["universeCount"] = universeCount(config);
    JsonObject network = object.createNestedObject("network");
    network["dhcp"] = config.dhcp;
    char address[16];
    formatIp(config.ip, address); network["ip"] = address;
    formatIp(config.netmask, address); network["netmask"] = address;
    formatIp(config.gateway, address); network["gateway"] = address;
    formatIp(config.dns, address); network["dns"] = address;
    JsonArray outputs = object.createNestedArray("outputs");
    unsigned startPixel = 0;
    for (unsigned id = 0; id < kOutputs; ++id) {
        const auto& output = config.outputs[id];
        JsonObject item = outputs.createNestedObject();
        item["id"] = id;
        item["type"] = "WS2812B";
        item["enabled"] = output.enabled;
        item["pixelCount"] = output.pixelCount;
        item["startUniverse"] = output.startUniverse;
        item["dataPin"] = board::kOutputs[id].teensyPin;
        item["clockPin"] = -1;
        item["colorOrder"] = orderName(output.colorOrder);
        item["reverse"] = output.reverse;
        item["startPixel"] = startPixel;
        item["targetFps"] = config.targetFps;
        if (output.enabled) startPixel += output.pixelCount;
    }
}
inline bool jsonUnsigned(JsonVariantConst value, uint32_t maximum, uint32_t& result) {
    // Integer JSON values only: no numeric strings, booleans, floats, NaN or infinity.
    if (!value.is<uint32_t>()) return false;
    result = value.as<uint32_t>();
    return result <= maximum;
}
inline const char* jsonText(JsonVariantConst value) {
    if (!value.is<const char*>()) return nullptr;
    const JsonString text = value.as<JsonString>();
    // Embedded JSON \u0000 must not turn a mismatched profile/type/IP into a valid prefix.
    return text.size() == strlen(text.c_str()) ? text.c_str() : nullptr;
}
inline bool fromJson(JsonObjectConst object, Config& destination, char* error, size_t cap) {
    if (error && cap) *error = 0;
    if (object.isNull()) return fail(error, cap, "Config must be a JSON object");
    uint32_t number;
    if (!jsonUnsigned(object["schemaVersion"], kSchemaVersion, number) || number != kSchemaVersion)
        return fail(error, cap, "Unsupported or missing schemaVersion");
    const char* profile = jsonText(object["hardwareProfile"]);
    if (!profile || strcmp(profile, board::kId))
        return fail(error, cap, "hardwareProfile must be PJRC_OCTO_ADAPTER_T41");
    Config candidate;
    if (!jsonUnsigned(object["brightness"], 255, number)) return fail(error, cap, "brightness must be an integer 0..255");
    candidate.brightness = uint8_t(number);
    if (!jsonUnsigned(object["targetFps"], 60, number) || !number) return fail(error, cap, "targetFps must be an integer 1..60");
    candidate.targetFps = uint8_t(number);
    if (!object["network"].is<JsonObjectConst>()) return fail(error, cap, "network object is required");
    const JsonObjectConst network = object["network"];
    if (!network["dhcp"].is<bool>()) return fail(error, cap, "network.dhcp must be boolean");
    candidate.dhcp = network["dhcp"].as<bool>();
    const char* keys[] = {"ip", "netmask", "gateway", "dns"};
    uint8_t* addresses[] = {candidate.ip, candidate.netmask, candidate.gateway, candidate.dns};
    for (unsigned i = 0; i < 4; ++i)
        if (!parseIp(jsonText(network[keys[i]]), addresses[i]))
            return fail(error, cap, "Network addresses must be dotted-decimal IPv4 strings");
    if (!object["outputs"].is<JsonArrayConst>() || object["outputs"].size() != kOutputs)
        return fail(error, cap, "Exactly eight explicit outputs are required");
    unsigned seen = 0;
    for (JsonVariantConst entry : object["outputs"].as<JsonArrayConst>()) {
        if (!entry.is<JsonObjectConst>()) return fail(error, cap, "Each output must be an object");
        const JsonObjectConst item = entry;
        if (!jsonUnsigned(item["id"], kOutputs - 1, number)) return fail(error, cap, "Output id must be 0..7");
        const unsigned id = number;
        if (seen & (1U << id)) return fail(error, cap, "Duplicate output id");
        seen |= 1U << id;
        auto& output = candidate.outputs[id];
        const char* type = jsonText(item["type"]);
        if (!type || strcmp(type, "WS2812B"))
            return fail(error, cap, "Only WS2812B output type is supported");
        if (!item["enabled"].is<bool>() || !item["reverse"].is<bool>())
            return fail(error, cap, "Output enabled/reverse must be booleans");
        output.enabled = item["enabled"].as<bool>(); output.reverse = item["reverse"].as<bool>();
        if (!jsonUnsigned(item["pixelCount"], kMaxLength, number)) return fail(error, cap, "pixelCount must be 0..1200");
        output.pixelCount = uint16_t(number);
        if (!jsonUnsigned(item["startUniverse"], 32767, number)) return fail(error, cap, "startUniverse must be 0..32767");
        output.startUniverse = uint16_t(number);
        if (!jsonUnsigned(item["dataPin"], 255, number) || number != board::kOutputs[id].teensyPin ||
            !item["clockPin"].is<int>() || item["clockPin"].as<int>() != -1)
            return fail(error, cap, "Output GPIOs must match the fixed PJRC physical output");
        if (!parseOrder(jsonText(item["colorOrder"]), output.colorOrder))
            return fail(error, cap, "Invalid output colorOrder");
        if (item.containsKey("targetFps") &&
            (!jsonUnsigned(item["targetFps"], 60, number) || number != candidate.targetFps))
            return fail(error, cap, "Per-output targetFps must match global targetFps");
    }
    if (!validate(candidate, error, cap)) return false;
    // Derived ESP-compatible fields are emitted for clients, never trusted for routing.
    unsigned offsets[kOutputs] = {}, cursor = 0;
    for (unsigned id = 0; id < kOutputs; ++id) {
        offsets[id] = cursor;
        if (candidate.outputs[id].enabled) cursor += candidate.outputs[id].pixelCount;
    }
    for (JsonObjectConst item : object["outputs"].as<JsonArrayConst>())
        if (item.containsKey("startPixel") &&
            (!jsonUnsigned(item["startPixel"], kOutputs * kMaxLength, number) ||
             number != offsets[item["id"].as<unsigned>()]))
            return fail(error, cap, "startPixel does not match the computed physical output layout");
    if (object.containsKey("pixelCount") &&
        (!jsonUnsigned(object["pixelCount"], kOutputs * kMaxLength, number) || number != totalPixels(candidate)))
        return fail(error, cap, "Total pixelCount does not match enabled outputs");
    if (object.containsKey("universeCount") &&
        (!jsonUnsigned(object["universeCount"], 64, number) || number != universeCount(candidate)))
        return fail(error, cap, "universeCount does not match enabled output ranges");
    destination = candidate;  // No partial mutation after a malformed import.
    return true;
}
}  // namespace webcfg
