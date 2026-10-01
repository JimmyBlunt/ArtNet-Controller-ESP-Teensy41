#include "WebApi.h"

#include "Performance.h"
#include "FirmwareUpdate.h"
#include "WebUi.generated.h"

#include <sstream>

#ifdef ARDUINO
#include <ArduinoJson.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <nvs.h>
#include <WebServer.h>
#include <cstring>
#include <vector>
namespace {
WebServer server(80);

void sendCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void sendJson(int statusCode, const char* body) {
  sendCorsHeaders();
  server.send(statusCode, "application/json", body);
}

void sendJson(int statusCode, const std::string& body) {
  sendJson(statusCode, body.c_str());
}

void sendHtml(int statusCode, const char* body) {
  sendCorsHeaders();
  server.send(statusCode, "text/html; charset=utf-8", body);
}

void registerOptions(const char* path) {
  server.on(path, HTTP_OPTIONS, []() {
    sendCorsHeaders();
    server.send(204);
  });
}

void sendAccepted() {
  sendJson(202, "{\"accepted\":true}");
}

void sendError(int statusCode, const char* message) {
  JsonDocument doc;
  doc["error"] = message;
  String body;
  serializeJson(doc, body);
  sendJson(statusCode, body.c_str());
}

led::OutputType parseOutputType(const char* value) {
  return value && std::strcmp(value, "WS2812B") == 0 ? led::OutputType::WS2812B
                                                     : led::OutputType::APA102;
}

led::ColorOrder parseColorOrder(const char* value, led::OutputType type) {
  if (!value) return type == led::OutputType::APA102 ? led::ColorOrder::BGR : led::ColorOrder::GRB;
  if (std::strcmp(value, "RBG") == 0) return led::ColorOrder::RBG;
  if (std::strcmp(value, "GRB") == 0) return led::ColorOrder::GRB;
  if (std::strcmp(value, "GBR") == 0) return led::ColorOrder::GBR;
  if (std::strcmp(value, "BRG") == 0) return led::ColorOrder::BRG;
  if (std::strcmp(value, "BGR") == 0) return led::ColorOrder::BGR;
  if (std::strcmp(value, "RGB") == 0) return led::ColorOrder::RGB;
  return static_cast<led::ColorOrder>(-1);
}

bool parseOutputs(JsonVariantConst value, std::vector<led::OutputConfig>* outputs,
                  std::string* error) {
  if (!value.is<JsonArrayConst>()) {
    if (error) *error = "outputs must be an array";
    return false;
  }
  std::vector<led::OutputConfig> parsed;
  for (JsonObjectConst item : value.as<JsonArrayConst>()) {
    led::OutputConfig output;
    output.id = item["id"] | static_cast<int>(parsed.size());
    output.type = parseOutputType(item["type"] | "APA102");
    output.enabled = item["enabled"] | true;
    output.startPixel = item["startPixel"] | 0U;
    output.pixelCount = item["pixelCount"] | 0U;
    output.dataPin = item["dataPin"] | -1;
    output.clockPin = item["clockPin"] | (output.type == led::OutputType::APA102 ? 18 : -1);
    const char* colorOrder =
        item["colorOrder"].is<const char*>() ? item["colorOrder"].as<const char*>() : nullptr;
    if (item.containsKey("colorOrder") && colorOrder == nullptr) {
      if (error) *error = "colorOrder must be a string";
      return false;
    }
    output.colorOrder = parseColorOrder(colorOrder, output.type);
    output.reverse = item["reverse"] | false;
    output.spiHz = item["spiHz"] | (output.type == led::OutputType::APA102 ? 4000000U : 0U);
    output.startUniverse = item["startUniverse"] | 0;
    output.targetFps = item["targetFps"] | 60;
    parsed.push_back(output);
  }
  *outputs = parsed;
  return true;
}

bool parseRequest(JsonDocument* doc, std::string* error) {
  DeserializationError parseError = deserializeJson(*doc, server.arg("plain"));
  if (parseError) {
    if (error) *error = parseError.c_str();
    return false;
  }
  return true;
}

std::string jsonErrorBody(const std::string& error) {
  JsonDocument doc;
  doc["error"] = error.c_str();
  String body;
  serializeJson(doc, body);
  return body.c_str();
}
}
#endif

namespace led {

static const char* outputTypeName(OutputType type) {
  return type == OutputType::APA102 ? "APA102" : "WS2812B";
}

std::string WebApi::statusJson(const ControllerConfig& config, const SystemStats& stats,
                               const PerformanceSnapshot& performance) {
  std::ostringstream out;
  out << "{\"pixelCount\":" << config.pixelCount
      << ",\"universeCount\":" << config.universeCount
      << ",\"packets\":" << stats.artnet.packets
      << ",\"framesComplete\":" << stats.artnet.framesComplete
      << ",\"framesIncomplete\":" << stats.artnet.framesIncomplete
      << ",\"fps\":" << performance.fps
      << ",\"packetsPerSecond\":" << performance.packetsPerSecond
      << ",\"heapFree\":" << performance.heapFree
      << ",\"heapMinFree\":" << performance.heapMinFree
      << ",\"frameTimeUs\":" << performance.frameTimeUs
      << ",\"outputTimeUs\":" << performance.outputTimeUs
      << ",\"lastFrameLatencyMs\":null"
      << ",\"startUniverse\":" << config.startUniverse
      << ",\"outputFrames\":" << stats.outputFrames
      << ",\"timeouts\":" << stats.artnet.timeouts
      << ",\"droppedPackets\":" << stats.artnet.droppedPackets
      << ",\"sequenceErrors\":" << stats.artnet.sequenceErrors << "}";
  return out.str();
}

std::string WebApi::outputsJson(const ControllerConfig& config) {
  std::ostringstream out;
  out << "[";
  for (size_t i = 0; i < config.outputs.size(); ++i) {
    const auto& o = config.outputs[i];
    if (i) out << ",";
    out << "{\"id\":" << o.id << ",\"type\":\"" << outputTypeName(o.type)
        << "\",\"enabled\":" << (o.enabled ? "true" : "false")
        << ",\"startPixel\":" << o.startPixel << ",\"pixelCount\":" << o.pixelCount
        << ",\"dataPin\":" << o.dataPin << ",\"clockPin\":" << o.clockPin
        << ",\"colorOrder\":\"";
    switch (o.colorOrder) {
      case ColorOrder::RBG: out << "RBG"; break;
      case ColorOrder::GRB: out << "GRB"; break;
      case ColorOrder::GBR: out << "GBR"; break;
      case ColorOrder::BRG: out << "BRG"; break;
      case ColorOrder::BGR: out << "BGR"; break;
      case ColorOrder::RGB:
      default: out << "RGB"; break;
    }
    out << "\",\"reverse\":" << (o.reverse ? "true" : "false")
        << ",\"spiHz\":" << o.spiHz
        << ",\"startUniverse\":" << o.startUniverse
        << ",\"targetFps\":" << o.targetFps << "}";
  }
  out << "]";
  return out.str();
}

std::string WebApi::configJson(const ControllerConfig& config) {
  std::ostringstream out;
  out << "{\"pixelCount\":" << config.pixelCount
      << ",\"startUniverse\":" << config.startUniverse
      << ",\"universeCount\":" << config.universeCount
      << ",\"targetFps\":" << config.targetFps
#if defined(LED_PROFILE_FLEX8)
  #if defined(LED_PROFILE_EXTENSION_BOARD)
      << ",\"hardwareProfile\":\"esp32-wroom-flex-8ws-2apa\""
  #else
      << ",\"hardwareProfile\":\"flex8-ws2812-apa102\""
  #endif
#else
      << ",\"hardwareProfile\":\"fixed\""
#endif
      << ",\"outputs\":" << outputsJson(config) << "}";
  return out.str();
}

void WebApi::setApplyConfigCallback(ApplyConfigCallback callback) {
  applyConfig_ = std::move(callback);
}


bool WebApi::loadSavedConfig(ControllerConfig* config) {
#ifdef ARDUINO
  nvs_handle_t handle;
  const esp_err_t opened = nvs_open("artnet-led", NVS_READONLY, &handle);
  if (opened == ESP_ERR_NVS_NOT_FOUND) {
    Serial.println("[config] no saved configuration; using compiled defaults");
    return false;
  }
  if (opened != ESP_OK) {
    Serial.printf("[config] storage read failed: %s; using defaults\n", esp_err_to_name(opened));
    return false;
  }
  size_t size = 0;
  const esp_err_t measured = nvs_get_str(handle, "config-v1", nullptr, &size);
  if (measured != ESP_OK || size < 2 || size > 16384) {
    nvs_close(handle);
    Serial.println("[config] saved configuration absent or unreadable; using defaults");
    return false;
  }
  std::vector<char> saved(size);
  const esp_err_t read = nvs_get_str(handle, "config-v1", saved.data(), &size);
  nvs_close(handle);
  if (read != ESP_OK) return false;
  JsonDocument doc;
  if (deserializeJson(doc, saved.data())) return false;
  ControllerConfig next;
  next.pixelCount = doc["pixelCount"] | 0U;
  next.startUniverse = doc["startUniverse"] | 0;
  next.targetFps = doc["targetFps"] | 30;
  std::string error;
  if (!parseOutputs(doc["outputs"], &next.outputs, &error)) {
    Serial.println("[config] saved config invalid; using compiled defaults");
    return false;
  }
  next.universeCount = mappedUniverseCount(next);
  if (!validateRuntimeHardwareConfig(next, &error)) {
    Serial.println("[config] saved config invalid; using compiled defaults");
    return false;
  }
  *config = std::move(next);
  Serial.println("[config] restored saved configuration");
  return true;
#else
  (void)config;
  return false;
#endif
}

const char* WebApi::indexHtml() {
  return kControllerWebHtml;
}

bool WebApi::begin(ControllerConfig* config, SystemStats* stats,
                   PerformanceMonitor* performance, PreviewStreamer* preview,
                   HardwareTest* hardwareTest) {
  config_ = config;
  stats_ = stats;
  performance_ = performance;
  preview_ = preview;
  hardwareTest_ = hardwareTest;
#ifdef ARDUINO
  registerOptions("/api/status");
  registerOptions("/api/config");
  registerOptions("/api/config/save");
  registerOptions("/api/storage");
  registerOptions("/api/outputs");
  registerOptions("/api/artnet/stats");
  registerOptions("/api/performance");
  registerOptions("/api/mappings");
  registerOptions("/api/preview/target");
  registerOptions("/api/test-pattern");
  registerOptions("/api/reboot");

  // Keep the original artwork in the separate data partition so OTA firmware fits.
  // An absent filesystem must not erase data or prevent controller operation.
  const bool artworkReady = SPIFFS.begin(false);
  if (!artworkReady) Serial.println("[web] Artwork filesystem unavailable; using CSS background");
  server.on("/assets/orbital-prism.webp", HTTP_GET, [artworkReady]() {
    if (!artworkReady) { server.send(404, "text/plain", "Artwork unavailable"); return; }
    File artwork = SPIFFS.open("/orbital-prism.webp", "r");
    if (!artwork) { server.send(404, "text/plain", "Artwork unavailable"); return; }
    server.sendHeader("Cache-Control", "public, max-age=86400");
    server.streamFile(artwork, "image/webp");
  });

  server.on("/", HTTP_GET, []() {
    sendHtml(200, WebApi::indexHtml());
  });
  server.on("/live", HTTP_GET, []() {
    sendHtml(200, WebApi::indexHtml());
  });
  server.on("/api/status", HTTP_GET, [this]() {
    sendJson(200, statusJson(*config_, *stats_, performance_->snapshot()));
  });
  server.on("/api/storage", HTTP_GET, [this]() {
    Preferences prefs;
    String saved;
    if (prefs.begin("artnet-led", true)) {
      saved = prefs.getString("config-v1", "");
      prefs.end();
    }
    const bool matches = saved.length() && saved == configJson(*config_).c_str();
    sendJson(200, std::string("{\"saved\":") + (saved.length() ? "true" : "false") +
        ",\"matches\":" + (matches ? "true" : "false") + "}");
  });
  server.on("/api/config/save", HTTP_POST, [this]() {
    std::string error;
    if (!validateRuntimeHardwareConfig(*config_, &error)) {
      sendJson(400, jsonErrorBody(error));
      return;
    }
    Preferences prefs;
    const std::string data = configJson(*config_);
    if (!prefs.begin("artnet-led", false)) {
      sendError(500, "storage unavailable");
      return;
    }
    const bool ok = prefs.putString("config-v1", data.c_str()) == data.size();
    prefs.end();
    if (!ok) { sendError(500, "storage write failed"); return; }
    sendJson(200, "{\"saved\":true,\"matches\":true}");
  });
  server.on("/api/config", HTTP_GET, [this]() {
    sendJson(200, configJson(*config_));
  });
  server.on("/api/config", HTTP_POST, [this]() {
    JsonDocument doc;
    std::string error;
    if (!parseRequest(&doc, &error)) {
      sendJson(400, jsonErrorBody(error));
      return;
    }

    ControllerConfig next = *config_;
    JsonObjectConst root = doc.as<JsonObjectConst>();
    next.pixelCount = root["pixelCount"] | next.pixelCount;
    next.startUniverse = root["startUniverse"] | next.startUniverse;
    next.targetFps = root["targetFps"] | next.targetFps;
    if (root["outputs"]) {
      if (!parseOutputs(root["outputs"], &next.outputs, &error)) {
        sendJson(400, jsonErrorBody(error));
        return;
      }
    }
    next.universeCount = mappedUniverseCount(next);
    if (!validateConfig(next, &error)) {
      sendJson(400, jsonErrorBody(error));
      return;
    }
    if (applyConfig_) {
      if (!applyConfig_(next, &error)) {
        sendJson(400, jsonErrorBody(error));
        return;
      }
    } else {
      *config_ = next;
    }
    sendJson(200, configJson(*config_));
  });
  server.on("/api/outputs", HTTP_GET, [this]() {
    sendJson(200, outputsJson(*config_));
  });
  server.on("/api/outputs", HTTP_POST, [this]() {
    JsonDocument doc;
    std::string error;
    if (!parseRequest(&doc, &error)) {
      sendJson(400, jsonErrorBody(error));
      return;
    }

    std::vector<OutputConfig> outputs;
    JsonVariantConst root = doc.as<JsonVariantConst>();
    JsonVariantConst outputsValue = root["outputs"] ? root["outputs"] : root;
    if (!parseOutputs(outputsValue, &outputs, &error)) {
      sendJson(400, jsonErrorBody(error));
      return;
    }
    ControllerConfig next = *config_;
    next.outputs = outputs;
    next.pixelCount = 0;
    for (auto& output : next.outputs) {
      output.startPixel = next.pixelCount;
      if (output.enabled) next.pixelCount += output.pixelCount;
    }
    next.universeCount = mappedUniverseCount(next);
    if (!validateConfig(next, &error)) {
      sendJson(400, jsonErrorBody(error));
      return;
    }
    if (applyConfig_) {
      if (!applyConfig_(next, &error)) {
        sendJson(400, jsonErrorBody(error));
        return;
      }
    } else {
      *config_ = next;
    }
    sendJson(200, outputsJson(*config_));
  });
  server.on("/api/artnet/stats", HTTP_GET, [this]() {
    sendJson(200, statusJson(*config_, *stats_, performance_->snapshot()));
  });
  server.on("/api/performance", HTTP_GET, [this]() {
    sendJson(200, statusJson(*config_, *stats_, performance_->snapshot()));
  });
  server.on("/api/mappings", HTTP_GET, [this]() {
    sendJson(200, mappingsJson_);
  });
  server.on("/api/mappings", HTTP_POST, [this]() {
    sendError(501, "Mapping conversion is PC-side; no runtime mapping is applied on this controller.");
  });
  server.on("/api/preview/target", HTTP_POST, [this]() {
    JsonDocument doc;
    std::string error;
    if (!parseRequest(&doc, &error)) {
      sendJson(400, jsonErrorBody(error));
      return;
    }
    const char* host = doc["host"] | "";
    const uint16_t port = doc["port"] | 6455;
    if (preview_ == nullptr) {
      sendError(500, "preview streamer not configured");
      return;
    }
    if (host[0] == '\0' || port == 0) {
      sendError(400, "host and port are required");
      return;
    }
    preview_->setTarget(host, port);
    JsonDocument result;
    result["host"] = host;
    result["port"] = port;
    String body;
    serializeJson(result, body);
    sendJson(200, body.c_str());
  });
  server.on("/api/test-pattern", HTTP_GET, [this]() {
    if (hardwareTest_ == nullptr) {
      sendJson(200, "{\"active\":false,\"phase\":\"unavailable\"}");
      return;
    }
    sendJson(200, hardwareTest_->statusJson());
  });
  server.on("/api/test-pattern", HTTP_POST, [this]() {
    if (hardwareTest_ == nullptr) {
      sendError(500, "hardware test not configured");
      return;
    }
    JsonDocument doc;
    const bool hasBody = server.arg("plain").length() > 0;
    if (hasBody) {
      std::string error;
      if (!parseRequest(&doc, &error)) {
        sendJson(400, jsonErrorBody(error));
        return;
      }
    }
    const char* action = hasBody ? (doc["action"] | "start") : "start";
    const int outputId = doc["outputId"] | -1;
    if (outputId != -1) {
      bool found = false;
      for (const auto& output : config_->outputs) {
        if (output.enabled && output.id == outputId) found = true;
      }
      if (!found) { sendError(400, "output is unavailable"); return; }
    }
    if (std::strcmp(action, "blackout") == 0) {
      hardwareTest_->blackout();
    } else if (std::strcmp(action, "stop") == 0 || std::strcmp(action, "off") == 0) {
      hardwareTest_->stop();
    } else if (std::strcmp(action, "loop") == 0 || std::strcmp(action, "start-loop") == 0) {
      hardwareTest_->startLoop(millis(), outputId);
    } else if (std::strcmp(action, "start") == 0) {
      hardwareTest_->start(millis(), outputId);
    } else {
      sendError(400, "unknown test action");
      return;
    }
    sendJson(202, hardwareTest_->statusJson());
  });
  server.on("/api/reboot", HTTP_POST, []() {
    sendAccepted();
    delay(100);
    ESP.restart();
  });
  registerFirmwareUpdate(server);
  server.onNotFound([]() { sendJson(404, "{\"error\":\"not found\"}"); });
  server.begin();
#endif
  return config_ != nullptr && stats_ != nullptr && performance_ != nullptr;
}

void WebApi::handleClient() {
#ifdef ARDUINO
  server.handleClient();
#endif
}

}  // namespace led
