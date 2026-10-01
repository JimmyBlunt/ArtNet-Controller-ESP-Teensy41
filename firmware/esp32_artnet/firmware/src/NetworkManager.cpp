#include "NetworkManager.h"

#include <cstring>

#ifdef ARDUINO
#include <Arduino.h>
#include <esp_system.h>
#include <Preferences.h>
#include <WiFi.h>
#if __has_include("WifiSecrets.h")
#include "WifiSecrets.h"
#endif
#if defined(LED_PROFILE_RMII_ETHERNET)
#include <ETH.h>
#endif
#endif

#ifndef SCHRANK_WIFI_SSID
#define SCHRANK_WIFI_SSID ""
#endif

#ifndef SCHRANK_WIFI_PASSWORD
#define SCHRANK_WIFI_PASSWORD ""
#endif

namespace led {

#ifdef ARDUINO
namespace {
bool isAutomaticCrashReset(esp_reset_reason_t reason) {
  return reason == ESP_RST_PANIC || reason == ESP_RST_INT_WDT ||
         reason == ESP_RST_TASK_WDT || reason == ESP_RST_WDT ||
         reason == ESP_RST_BROWNOUT;
}
}  // namespace
#endif

bool NetworkManager::begin(bool explicitRetry) {
  (void)explicitRetry;
#ifdef ARDUINO
#if defined(LED_PROFILE_RMII_ETHERNET)
  connected_ = ETH.begin();
  ip_ = connected_ ? ETH.localIP().toString().c_str() : "0.0.0.0";
#elif defined(LED_PROFILE_W5500)
  connected_ = false;
  ip_ = "0.0.0.0";
  Serial.println("W5500 profile compiled; runtime W5500 init requires board-specific CS/SPI wiring.");
#else
  connected_ = false;
  ip_ = "0.0.0.0";
  if (std::strlen(SCHRANK_WIFI_SSID) == 0) {
    Serial.println("WiFi profile compiled; set firmware/include/WifiSecrets.h before network runtime tests.");
    connected_ = false;
  } else {
    // Commit BEFORE radio startup: RTC memory does not survive a supply reset.
    // This flag is cleared only after the complete startup attempt returns.
    Preferences bootState;
    if (!bootState.begin("led-boot", false)) {
      safeMode_ = true;
      Serial.println("[network] SAFE MODE: cannot open boot guard storage; radio stays off");
      return false;
    }
    const bool previousAttemptPending = bootState.getBool("wifi-pending", false);
    const esp_reset_reason_t resetReason = esp_reset_reason();
    if (previousAttemptPending && !explicitRetry && isAutomaticCrashReset(resetReason)) {
      bootState.end();
      safeMode_ = true;
      Serial.println("[network] SAFE MODE: previous WiFi startup did not finish; radio stays off");
      Serial.println("[network] Press EN or power-cycle for ONE new WiFi attempt");
      Serial.println("[network] USB recovery also available: wifi-retry followed by newline");
      return false;
    }
    if (previousAttemptPending && !explicitRetry) {
      Serial.printf("[network] Manual restart detected (reason=%d); retrying WiFi\n",
                    static_cast<int>(resetReason));
    }
    const bool armed = bootState.putBool("wifi-pending", true) == 1;
    bootState.end();
    if (!armed) {
      safeMode_ = true;
      Serial.println("[network] SAFE MODE: cannot persist boot guard; radio stays off");
      return false;
    }
    Serial.println("[network] BEFORE WiFi.mode(WIFI_STA); boot guard armed");
    Serial.flush();
    if (!WiFi.mode(WIFI_STA)) {
      safeMode_ = true;
      Serial.println("[network] SAFE MODE: WiFi.mode failed; boot guard remains armed");
      return false;
    }
    Serial.println("[network] AFTER WiFi.mode; connecting (credentials not logged)");
    // Preserve the existing low-latency Art-Net setting after radio startup.
    WiFi.setSleep(false);
    WiFi.begin(SCHRANK_WIFI_SSID, SCHRANK_WIFI_PASSWORD);
    const uint32_t startMs = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startMs < 15000) {
      delay(250);
      Serial.print(".");
    }
    Serial.println();
    connected_ = WiFi.status() == WL_CONNECTED;
    Serial.println(connected_ ? "WiFi connected." : "WiFi connection failed or timed out.");
    safeMode_ = false;
    if (bootState.begin("led-boot", false)) {
      const bool cleared = bootState.putBool("wifi-pending", false) == 1;
      bootState.end();
      if (!cleared) Serial.println("[network] WARNING: boot guard could not be cleared");
    } else {
      Serial.println("[network] WARNING: boot guard storage unavailable after WiFi startup");
    }
  }
  ip_ = connected_ ? WiFi.localIP().toString().c_str() : "0.0.0.0";
#endif
#endif
  return connected_;
}

void NetworkManager::handleSerial() {
#ifdef ARDUINO
  // Bounded line parser: random serial bytes cannot retry radio startup.
  for (int budget = 0; budget < 64 && Serial.available(); ++budget) {
    const char value = Serial.read();
    if (value == '\r') continue;
    if (value == '\n') {
      if (!serialOverflow_ && serialCommand_ == "wifi-retry") {
        if (safeMode_) begin(true);
        else Serial.println("[network] wifi-retry ignored: not in safe mode");
      }
      serialCommand_.clear();
      serialOverflow_ = false;
    } else if (serialCommand_.size() < 32 && !serialOverflow_) {
      serialCommand_ += value;
    } else {
      serialOverflow_ = true;
    }
  }
#endif
}

bool NetworkManager::connected() const {
  if (safeMode_) return false;
#if defined(ARDUINO) && defined(LED_PROFILE_RMII_ETHERNET)
  return ETH.linkUp() && ETH.localIP() != IPAddress(0, 0, 0, 0);
#elif defined(ARDUINO) && !defined(LED_PROFILE_W5500)
  return WiFi.status() == WL_CONNECTED;
#else
  return connected_;
#endif
}
std::string NetworkManager::ipAddress() const {
  if (safeMode_) return "0.0.0.0";
#if defined(ARDUINO) && defined(LED_PROFILE_RMII_ETHERNET)
  return ETH.localIP().toString().c_str();
#elif defined(ARDUINO) && !defined(LED_PROFILE_W5500)
  return WiFi.localIP().toString().c_str();
#else
  return ip_;
#endif
}

NetworkProfile NetworkManager::profile() const {
#if defined(LED_PROFILE_RMII_ETHERNET)
  return NetworkProfile::RmiiEthernet;
#elif defined(LED_PROFILE_W5500)
  return NetworkProfile::W5500;
#else
  return NetworkProfile::Wifi;
#endif
}

}  // namespace led
