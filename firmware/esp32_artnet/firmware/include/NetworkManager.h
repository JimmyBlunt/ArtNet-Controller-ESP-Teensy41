#pragma once

#include <string>

namespace led {

enum class NetworkProfile { Wifi, RmiiEthernet, W5500 };

class NetworkManager {
 public:
  bool begin(bool explicitRetry = false);
  void handleSerial();
  bool safeMode() const { return safeMode_; }
  bool connected() const;
  std::string ipAddress() const;
  NetworkProfile profile() const;

 private:
  bool connected_ = false;
  bool safeMode_ = false;
  std::string serialCommand_;
  bool serialOverflow_ = false;
  std::string ip_ = "0.0.0.0";
};

}  // namespace led
