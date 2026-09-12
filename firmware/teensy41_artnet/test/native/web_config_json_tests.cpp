#include <cassert>
#include <functional>
#include <iostream>
#include <limits>
#include <string>
#include "../../include/web_config_json.h"

static void rejects(const std::function<void(JsonObject)>& mutate) {
    DynamicJsonDocument document(webcfg::kJsonCapacity);
    webcfg::toJson(webcfg::defaults(), document.to<JsonObject>());
    mutate(document.as<JsonObject>());
    assert(!document.overflowed());
    auto destination = webcfg::defaults(); destination.brightness = 177;
    const auto untouched = destination;
    char error[180] = {};
    assert(!webcfg::fromJson(document.as<JsonObjectConst>(), destination, error, sizeof(error)));
    assert(error[0] && webcfg::equal(destination, untouched));
}
int main() {
    DynamicJsonDocument document(webcfg::kJsonCapacity);
    const auto original = webcfg::defaults();
    webcfg::toJson(original, document.to<JsonObject>());
    assert(!document.overflowed());
    std::string serialized;
    serializeJson(document, serialized);
    assert(serialized.size() < 6144);
    DynamicJsonDocument parsed(webcfg::kJsonCapacity);
    assert(!deserializeJson(parsed, serialized));
    webcfg::Config restored;
    char error[180] = {};
    assert(webcfg::fromJson(parsed.as<JsonObjectConst>(), restored, error, sizeof(error)));
    assert(webcfg::equal(restored, original));
    assert(document["schemaVersion"].as<unsigned>() == 1);
    assert(document["pixelCount"].as<unsigned>() == 4031);
    assert(document["universeCount"].as<unsigned>() == 29);
    assert(document["outputs"][6]["id"].as<unsigned>() == 6);
    assert(document["outputs"][6]["startPixel"].as<unsigned>() == 3519);
    assert(document["outputs"][7]["startPixel"].as<unsigned>() == 4031);
    assert(document["outputs"][7]["dataPin"].as<unsigned>() == 5);
    assert(document["network"]["ip"].as<std::string>() == "0.0.0.0");
    assert(document["network"]["netmask"].as<std::string>() == "255.255.255.0");

    rejects([](JsonObject o) { o.remove("schemaVersion"); });
    rejects([](JsonObject o) { o["schemaVersion"] = 2; });
    rejects([](JsonObject o) { o["schemaVersion"] = "1"; });
    rejects([](JsonObject o) { o["hardwareProfile"] = "flex8-ws2812-apa102"; });
    rejects([](JsonObject o) { o["hardwareProfile"] = 1; });
    rejects([](JsonObject o) { o["hardwareProfile"] = std::string(webcfg::board::kId) + std::string("\0other", 6); });
    rejects([](JsonObject o) { o["brightness"] = -1; });
    rejects([](JsonObject o) { o["brightness"] = 256; });
    rejects([](JsonObject o) { o["brightness"] = true; });
    rejects([](JsonObject o) { o["brightness"] = 8.0; });
    rejects([](JsonObject o) { o["brightness"] = "8"; });
    rejects([](JsonObject o) { o["targetFps"] = std::numeric_limits<double>::infinity(); });
    rejects([](JsonObject o) { o["targetFps"] = std::numeric_limits<double>::quiet_NaN(); });
    rejects([](JsonObject o) { o["targetFps"] = 0; });
    rejects([](JsonObject o) { o["targetFps"] = 61; });
    rejects([](JsonObject o) { o["network"]["dhcp"] = "true"; });
    rejects([](JsonObject o) { o["network"]["dhcp"] = 1; });
    rejects([](JsonObject o) { o["network"]["ip"] = "999.0.0.1"; });
    rejects([](JsonObject o) { o["network"]["ip"] = std::string("1.2.3.4") + std::string("\0other", 6); });
    rejects([](JsonObject o) { o["network"]["dhcp"] = false; });
    rejects([](JsonObject o) { o["network"].remove("dns"); });
    rejects([](JsonObject o) { o["outputs"].as<JsonArray>().remove(7); });
    rejects([](JsonObject o) { o["outputs"].as<JsonArray>().add(1); });
    rejects([](JsonObject o) { o["outputs"][1]["id"] = 0; });
    rejects([](JsonObject o) { o["outputs"][0]["id"] = 8; });
    rejects([](JsonObject o) { o["outputs"][0]["id"] = 0.0; });
    rejects([](JsonObject o) { o["outputs"][0]["type"] = "APA102"; });
    rejects([](JsonObject o) { o["outputs"][0]["type"] = std::string("WS2812B") + std::string("\0other", 6); });
    rejects([](JsonObject o) { o["outputs"][0]["enabled"] = 1; });
    rejects([](JsonObject o) { o["outputs"][0]["reverse"] = "false"; });
    rejects([](JsonObject o) { o["outputs"][0]["pixelCount"] = -1; });
    rejects([](JsonObject o) { o["outputs"][0]["pixelCount"] = 4294967295UL; });
    rejects([](JsonObject o) { o["outputs"][0]["pixelCount"] = 203.0; });
    rejects([](JsonObject o) { o["outputs"][0]["startUniverse"] = 32768; });
    rejects([](JsonObject o) { o["outputs"][0]["dataPin"] = 14; });
    rejects([](JsonObject o) { o["outputs"][7]["dataPin"] = 8; }); // Disabled does not relax physical identity.
    rejects([](JsonObject o) { o["outputs"][0]["clockPin"] = 5; });
    rejects([](JsonObject o) { o["outputs"][0]["clockPin"] = -1.0; });
    rejects([](JsonObject o) { o["outputs"][0]["colorOrder"] = "grb"; });
    rejects([](JsonObject o) { o["outputs"][0]["startPixel"] = 1; });
    rejects([](JsonObject o) { o["outputs"][0]["targetFps"] = 29; });
    rejects([](JsonObject o) { o["pixelCount"] = 4030; });
    rejects([](JsonObject o) { o["universeCount"] = 28; });

    // Array order is irrelevant; explicit id owns the slot and its physical pin.
    DynamicJsonDocument reordered(webcfg::kJsonCapacity);
    reordered.set(document);
    JsonArray out = reordered["outputs"].to<JsonArray>();
    for (int id = 7; id >= 0; --id) out.add(document["outputs"][id]);
    assert(webcfg::fromJson(reordered.as<JsonObjectConst>(), restored, error, sizeof(error)));
    assert(webcfg::equal(restored, original));

    auto varied = original;
    for (unsigned i = 0; i < 8; ++i) {
        varied.outputs[i].colorOrder = static_cast<webcfg::Order>(i % 6);
        varied.outputs[i].reverse = i % 2 != 0;
        varied.outputs[i].enabled = false;
    }
    document.clear(); webcfg::toJson(varied, document.to<JsonObject>());
    assert(webcfg::fromJson(document.as<JsonObjectConst>(), restored, error, sizeof(error)));
    assert(webcfg::equal(restored, varied) && webcfg::totalPixels(restored) == 0);
    std::cout << "Strict ArduinoJson config imports passed; full export " << serialized.size()
              << " bytes; no partial mutations or free GPIOs\n";
}
