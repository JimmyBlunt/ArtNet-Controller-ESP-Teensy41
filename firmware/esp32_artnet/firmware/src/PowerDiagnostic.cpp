// Explicit diagnostic environment only. No LED driver, API, or configuration writes.
#if defined(ARDUINO) && defined(LED_POWER_DIAGNOSTIC)
#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h>
#if __has_include("WifiSecrets.h")
#include "WifiSecrets.h"
#endif
#ifndef SCHRANK_WIFI_SSID
#define SCHRANK_WIFI_SSID ""
#define SCHRANK_WIFI_PASSWORD ""
#endif

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.printf("[power-diag] boot resetReason=%d; no LED drivers\n", esp_reset_reason());
  Serial.println("[power-diag] radio OFF; send w to enable STA, c to connect, r to restart");
}

void loop() {
  static bool radioEnabled = false;
  static uint32_t lastLog = 0;
  if (Serial.available()) {
    const char command = Serial.read();
    if (command == 'w' && !radioEnabled) {
      Serial.println("[power-diag] BEFORE WiFi.mode(WIFI_STA)");
      Serial.flush();
      radioEnabled = WiFi.mode(WIFI_STA);
      Serial.printf("[power-diag] AFTER WiFi.mode result=%d\n", radioEnabled);
    } else if (command == 'c' && radioEnabled) {
      Serial.println("[power-diag] BEFORE WiFi.begin (credentials not logged)");
      Serial.flush();
      WiFi.begin(SCHRANK_WIFI_SSID, SCHRANK_WIFI_PASSWORD);
      Serial.println("[power-diag] AFTER WiFi.begin");
    } else if (command == 'r') {
      ESP.restart();
    }
  }
  if (millis() - lastLog >= 1000) {
    lastLog = millis();
    Serial.printf("[power-diag] uptime=%lu radio=%d heap=%u\n",
                  millis(), radioEnabled, ESP.getFreeHeap());
    if (radioEnabled) {
      Serial.printf("[power-diag] status=%d IP=%s\n", WiFi.status(),
                    WiFi.localIP().toString().c_str());
    }
  }
  delay(10);
}
#endif
