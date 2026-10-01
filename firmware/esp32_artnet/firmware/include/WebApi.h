#pragma once

#include "ConfigManager.h"
#include "HardwareTest.h"
#include "PerformanceMonitor.h"
#include "PreviewStreamer.h"
#include "SystemStats.h"

#include <functional>
#include <string>

namespace led {

class WebApi {
 public:
  using ApplyConfigCallback = std::function<bool(const ControllerConfig&, std::string*)>;

  bool begin(ControllerConfig* config, SystemStats* stats, PerformanceMonitor* performance,
             PreviewStreamer* preview = nullptr, HardwareTest* hardwareTest = nullptr);
  void handleClient();
  void setApplyConfigCallback(ApplyConfigCallback callback);

  static std::string statusJson(const ControllerConfig& config, const SystemStats& stats,
                                const PerformanceSnapshot& performance);
  static std::string configJson(const ControllerConfig& config);
  static std::string outputsJson(const ControllerConfig& config);
  static const char* indexHtml();
  static bool loadSavedConfig(ControllerConfig* config);

 private:
  ControllerConfig* config_ = nullptr;
  SystemStats* stats_ = nullptr;
  PerformanceMonitor* performance_ = nullptr;
  PreviewStreamer* preview_ = nullptr;
  HardwareTest* hardwareTest_ = nullptr;
  ApplyConfigCallback applyConfig_;
  std::string mappingsJson_ = "{\"mappings\":[]}";
};

}  // namespace led
