#ifdef ARDUINO
#include <Arduino.h>
#include <FastLED.h>
#include <vector>

#include "ArtNetReceiver.h"
#include "ConfigManager.h"
#include "HardwareTest.h"
#include "LedOutputs.h"
#include "LogicalPixelBuffer.h"
#include "NetworkManager.h"
#include "OutputRouter.h"
#include "Performance.h"
#include "PerformanceMonitor.h"
#include "PreviewStreamer.h"
#include "SystemStats.h"
#include "UniverseAssembler.h"
#include "WebApi.h"

namespace {
led::ControllerConfig config;
led::LogicalPixelBuffer logicalFrame;
led::LogicalPixelBuffer finalFrame;
led::NetworkManager network;
led::UniverseAssembler* assembler = nullptr;
led::LedOutputManager outputs;
led::ArtNetReceiver artnet;
led::PreviewStreamer preview;
led::WebApi webApi;
led::HardwareTest hardwareTest;
led::PerformanceMonitor performance;
led::SystemStats stats;
uint32_t frames = 0;
uint32_t lastLogMs = 0;
uint32_t lastPreviewMs = 0;
uint32_t lastStatsLogMs = 0;
uint32_t lastStatsFrames = 0;
uint32_t lastOutputMs = 0;
bool framePending = false;
bool networkServicesStarted = false;
std::vector<uint32_t> lastOutputUpdates;

#if defined(LED_PROFILE_FLEX8)
constexpr int kNetworkErrorLedPin = 2;
#elif defined(LED_BUILTIN)
constexpr int kNetworkErrorLedPin = LED_BUILTIN;
#else
constexpr int kNetworkErrorLedPin = -1;
#endif

void updateNetworkErrorLed(uint32_t nowMs) {
  if (kNetworkErrorLedPin >= 0) {
  // Two short flashes every two seconds mean that WiFi has no usable connection.
    const uint32_t phase = nowMs % 2000;
    const bool on = !network.connected() && (phase < 120 || (phase >= 260 && phase < 380));
    digitalWrite(kNetworkErrorLedPin, on ? HIGH : LOW);
  }
}

void startNetworkServices() {
  if (networkServicesStarted || !network.connected()) return;
  artnet.begin();
  preview.begin();
  webApi.begin(&config, &stats, &performance, &preview, &hardwareTest);
  networkServicesStarted = true;
  Serial.print("[network] services ready; IP: ");
  Serial.println(network.ipAddress().c_str());
}

const char* outputTypeName(led::OutputType type) {
  return type == led::OutputType::APA102 ? "APA102" : "WS2812B";
}

void resetStatsWindow(uint32_t nowMs) {
  lastStatsLogMs = nowMs;
  lastStatsFrames = frames;
  lastOutputUpdates.clear();
  for (const auto& stat : outputs.stats()) {
    lastOutputUpdates.push_back(stat.totalUpdates);
  }
}

void logOutputStats(uint32_t nowMs) {
  if (lastStatsLogMs == 0) {
    resetStatsWindow(nowMs);
    return;
  }
  if (nowMs - lastStatsLogMs < 20000) return;

  const uint32_t windowMs = nowMs - lastStatsLogMs;
  const uint32_t frameDelta = frames - lastStatsFrames;
  const double seconds = static_cast<double>(windowMs) / 1000.0;
  const auto& outputStats = outputs.stats();

  Serial.print("[output-stats] windowMs=");
  Serial.print(windowMs);
  Serial.print(" frames20s=");
  Serial.print(frameDelta);
  Serial.print(" fps20s=");
  Serial.print(frameDelta / seconds, 2);
  Serial.print(" fps1s=");
  Serial.print(performance.snapshot().fps);
  Serial.print(" outputs=");
  Serial.println(outputStats.size());

  for (size_t i = 0; i < outputStats.size(); ++i) {
    const auto& stat = outputStats[i];
    const uint32_t previous = i < lastOutputUpdates.size() ? lastOutputUpdates[i] : 0;
    const uint32_t updates = stat.totalUpdates - previous;
    Serial.print("[output-stats] id=");
    Serial.print(stat.id);
    Serial.print(" type=");
    Serial.print(outputTypeName(stat.type));
    Serial.print(" leds=");
    Serial.print(stat.pixelCount);
    Serial.print(" updates20s=");
    Serial.print(updates);
    Serial.print(" updatesPerSec=");
    Serial.print(updates / seconds, 2);
    Serial.print(" totalUpdates=");
    Serial.println(stat.totalUpdates);
  }

  resetStatsWindow(nowMs);
}

bool applyRuntimeConfig(const led::ControllerConfig& next, std::string* error) {
  if (!led::validateRuntimeHardwareConfig(next, error)) return false;

  Serial.println("[config] applying runtime config");
  hardwareTest.stop(&finalFrame);
  outputs.show(finalFrame);

  config = next;
  config.universeCount = led::mappedUniverseCount(config);
  logicalFrame.resize(config.pixelCount);
  finalFrame.resize(config.pixelCount);
  delete assembler;
#if defined(LED_PROFILE_FLEX8)
  assembler = new led::UniverseAssembler(config);
#else
  assembler = new led::UniverseAssembler(config.startUniverse, config.universeCount, config.pixelCount);
#endif
  outputs.begin(config);
  hardwareTest.begin(&config);
  stats = led::SystemStats();
  frames = 0;
  framePending = false;
  lastPreviewMs = 0;
  resetStatsWindow(millis());

  Serial.print("[config] applied pixelCount=");
  Serial.print(config.pixelCount);
  Serial.print(" universes=");
  Serial.print(config.universeCount);
  Serial.print(" outputs=");
  Serial.println(config.outputs.size());
  return true;
}
}

void setup() {
  Serial.begin(115200);
  if (kNetworkErrorLedPin >= 0) {
    pinMode(kNetworkErrorLedPin, OUTPUT);
    digitalWrite(kNetworkErrorLedPin, LOW);
  }
  config = led::defaultMixedConfig();
#if defined(LED_PROFILE_WS2812_APA102_1679) || defined(LED_PROFILE_FLEX8)
  led::WebApi::loadSavedConfig(&config);
#endif
  logicalFrame.resize(config.pixelCount);
  finalFrame.resize(config.pixelCount);
#if defined(LED_PROFILE_FLEX8)
  assembler = new led::UniverseAssembler(config);
#else
  assembler = new led::UniverseAssembler(config.startUniverse, config.universeCount, config.pixelCount);
#endif
  performance.begin(millis());

  Serial.println("Schrank LED mixed Art-Net controller boot");
#if defined(LED_PROFILE_RMII_ETHERNET)
  Serial.println("Profile: ESP32 RMII Ethernet preferred for APA102 SPI isolation");
#elif defined(LED_PROFILE_W5500)
  Serial.println("Profile: ESP32 W5500. Guard SPI bus when APA102 shares hardware SPI.");
#elif defined(LED_PROFILE_WS2812_APA102_1679)
  Serial.println("Profile: ESP32 WiFi WS2812B GPIO23 + APA102 data=18 clock=19");
#elif defined(LED_PROFILE_EXTENSION_BOARD)
  Serial.println("Profile: ESP32-WROOM flex 8x WS + APA G18/5 or G26/27");
#elif defined(LED_PROFILE_FLEX8)
  Serial.println("Profile: ESP32 WiFi flexible 8-output WS2812B/APA102 controller");
#else
  Serial.println("Profile: ESP32 WiFi fallback");
#endif

  String configError;
  std::string error;
  if (!led::validateConfig(config, &error)) {
    configError = error.c_str();
    Serial.print("Config error: ");
    Serial.println(configError);
  }

  outputs.begin(config);
  hardwareTest.begin(&config);
  network.begin();
  webApi.setApplyConfigCallback(applyRuntimeConfig);
  startNetworkServices();
  resetStatsWindow(millis());
#if defined(LED_PROFILE_WS2812_APA102_1679) && defined(LED_HARDWARE_TEST_AUTOSTART)
  hardwareTest.startLoop(millis());
  Serial.println("Hardware test loop auto-started; POST /api/test-pattern {\"action\":\"stop\"} to stop");
#elif defined(LED_PROFILE_WS2812_APA102_1679)
  Serial.println("Hardware test loop ready; POST /api/test-pattern {\"action\":\"loop\"} to start");
#endif

  Serial.print("Configured outputs: ");
  Serial.println(outputs.outputCount());
  Serial.print("IP: ");
  Serial.println(network.ipAddress().c_str());
}

void loop() {
  network.handleSerial();
  updateNetworkErrorLed(millis());
  startNetworkServices();
  const uint32_t now = millis();

  if (networkServicesStarted) webApi.handleClient();

  const bool testControlsOutput = hardwareTest.active();
  if (testControlsOutput) {
    framePending = false;
    if (hardwareTest.update(now, &finalFrame)) {
      performance.markFrameStart(micros());
      outputs.show(finalFrame);
      performance.markOutputDone(micros());
      stats.outputFrames++;
      frames++;
    }
  }

  // Drain a bounded burst every loop, including during tests/blackout, so stale
  // queued Art-Net data cannot take over after a test is stopped.
  if (assembler != nullptr) {
    assembler->expire(now);
    for (int i = 0; networkServicesStarted && i < 32 && artnet.poll(*assembler); ++i) {
      performance.markPacket(now);
      if (testControlsOutput || hardwareTest.active()) {
        assembler->discard();
        framePending = false;
      } else if (assembler->frameReady()) {
        assembler->compose(logicalFrame);
        framePending = true;
      }
    }
    stats.artnet = assembler->stats();
  }
  if (!testControlsOutput && !hardwareTest.active() && framePending &&
      now - lastOutputMs >= 1000u / config.targetFps) {
    performance.markFrameStart(micros());
    finalFrame.pixels() = logicalFrame.pixels();
    outputs.show(finalFrame);
    performance.markOutputDone(micros());
    lastOutputMs = now;
    framePending = false;
    stats.outputFrames++;
    frames++;
  }

  if (networkServicesStarted && network.connected() && now - lastPreviewMs >= 100) {
    lastPreviewMs = now;
    if (preview.sendFrame(frames, finalFrame)) {
      stats.previewFrames++;
    }
  }

  performance.update(now, ESP.getFreeHeap(), ESP.getMinFreeHeap());
  logOutputStats(now);

  if (now - lastLogMs >= 1000) {
    lastLogMs = now;
    Serial.print("status frames=");
    Serial.print(frames);
    Serial.print(" fps=");
    Serial.print(performance.snapshot().fps);
    Serial.print(" pps=");
    Serial.print(performance.snapshot().packetsPerSecond);
    Serial.print(" pixels=");
    Serial.print(config.pixelCount);
    Serial.print(" heap=");
    Serial.print(performance.snapshot().heapFree);
    Serial.print(" safeMode=");
    Serial.println(network.safeMode() ? "yes" : "no");
  }
}
#endif
