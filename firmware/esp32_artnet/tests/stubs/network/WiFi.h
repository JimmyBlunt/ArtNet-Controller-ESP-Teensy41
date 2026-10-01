#pragma once
#include "Preferences.h"
#include <stdexcept>
#include <string>
constexpr int WIFI_STA = 1;
constexpr int WL_CONNECTED = 3;
namespace fake {
inline bool crash = false;
inline bool modeWorks = true;
inline bool online = true;
inline int modeCalls = 0;
inline int connectCalls = 0;
}
struct FakeIp { std::string toString() const { return "192.0.2.1"; } };
struct FakeWiFi {
  bool mode(int) {
    ++fake::modeCalls;
    if (!fake::pending) throw std::logic_error("radio started before guard committed");
    if (fake::crash) throw std::runtime_error("simulated supply reset");
    return fake::modeWorks;
  }
  void begin(const char*, const char*) { ++fake::connectCalls; }
  void setSleep(bool) {}
  int status() const { return fake::online ? WL_CONNECTED : 0; }
  FakeIp localIP() const { return {}; }
};
inline FakeWiFi WiFi;
