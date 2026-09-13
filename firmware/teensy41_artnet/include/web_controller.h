#pragma once
#include <stddef.h>
#include <ArduinoJson.h>
#include "web_config.h"

namespace controller {
const webcfg::Config& config();
bool apply(const webcfg::Config&, char* error, size_t errorSize);
bool save(char* error, size_t errorSize);
bool start(char* error, size_t errorSize);
bool test(unsigned physicalOutput, unsigned seconds, char* error, size_t errorSize);
bool pattern(unsigned physicalOutput, bool loop, char* error, size_t errorSize);
void endPattern();
void stop();
bool reboot(char* error, size_t errorSize);
void status(JsonObject target);
bool isStopped();
}
