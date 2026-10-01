#pragma once

#include "OutputRouter.h"

namespace led {

ControllerConfig defaultMixedConfig();
uint16_t mappedUniverseCount(const ControllerConfig& config);
bool validateConfig(const ControllerConfig& config, std::string* error);
bool validateRuntimeHardwareConfig(const ControllerConfig& config, std::string* error);

}  // namespace led
