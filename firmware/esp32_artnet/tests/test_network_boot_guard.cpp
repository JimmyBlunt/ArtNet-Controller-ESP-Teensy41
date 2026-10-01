#include "NetworkManager.h"
#include "Arduino.h"
#include "WiFi.h"
#include "esp_system.h"
#include <iostream>
#include <stdexcept>

static void expect(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}
static void reset() {
  fake::now = 0;
  fake::input.clear();
  fake::log.clear();
  fake::pending = false;
  fake::storageAvailable = true;
  fake::writable = true;
  fake::writes = 0;
  fake::crash = false;
  fake::modeWorks = true;
  fake::online = true;
  fake::resetReason = ESP_RST_POWERON;
  fake::modeCalls = fake::connectCalls = 0;
}
int main() {
  reset();
  led::NetworkManager normal;
  expect(normal.begin(), "normal startup succeeds");
  expect(!fake::pending && fake::writes == 2, "guard armed and cleared exactly once");

  reset();
  fake::crash = true;
  led::NetworkManager crashing;
  try { crashing.begin(); } catch (const std::runtime_error&) {}
  expect(fake::pending && fake::modeCalls == 1, "interrupted startup preserves guard");
  fake::resetReason = ESP_RST_PANIC;
  led::NetworkManager rebooted;
  expect(!rebooted.begin() && rebooted.safeMode(), "next boot is safe");
  expect(!rebooted.connected() && rebooted.ipAddress() == "0.0.0.0", "safe mode reports offline");
  expect(fake::modeCalls == 1 && fake::connectCalls == 0, "safe mode never starts radio");
  fake::input = "garbage\nwifi-retry-extra\n";
  rebooted.handleSerial();
  expect(fake::modeCalls == 1, "only exact command retries");
  fake::input = std::string(40, 'x') + "wifi-retry\n";
  rebooted.handleSerial();
  expect(fake::modeCalls == 1, "oversized line cannot retry");
  fake::crash = false;
  fake::input = "wifi-retry\r\n";
  rebooted.handleSerial();
  expect(fake::modeCalls == 2 && !fake::pending && !rebooted.safeMode(), "explicit retry succeeds");
  fake::input = "wifi-retry\n";
  rebooted.handleSerial();
  expect(fake::modeCalls == 2, "healthy controller ignores retry");

  reset();
  fake::pending = true;
  fake::resetReason = ESP_RST_POWERON;
  led::NetworkManager manualRestart;
  expect(manualRestart.begin() && !manualRestart.safeMode(),
         "EN or power restart retries after a pending startup");
  expect(fake::modeCalls == 1 && !fake::pending,
         "manual restart arms and clears the guard around the retry");

  reset();
  fake::pending = true;
  fake::resetReason = ESP_RST_SW;
  led::NetworkManager softwareRestart;
  expect(softwareRestart.begin() && !softwareRestart.safeMode(),
         "intentional software restart retries after a pending startup");

  reset();
  fake::storageAvailable = false;
  led::NetworkManager noStorage;
  expect(!noStorage.begin() && noStorage.safeMode() && fake::modeCalls == 0,
         "storage failure leaves radio off");
  reset();
  fake::writable = false;
  led::NetworkManager noWrite;
  expect(!noWrite.begin() && noWrite.safeMode() && fake::modeCalls == 0,
         "write failure leaves radio off");
  reset();
  fake::modeWorks = false;
  led::NetworkManager badMode;
  expect(!badMode.begin() && badMode.safeMode() && fake::pending && fake::connectCalls == 0,
         "mode failure does not proceed to connect");
  reset();
  fake::online = false;
  led::NetworkManager noRouter;
  expect(!noRouter.begin() && !noRouter.safeMode() && !fake::pending && fake::now == 15000,
         "ordinary connection timeout is not a startup crash");
  std::cout << "Network boot guard tests passed\n";
}
