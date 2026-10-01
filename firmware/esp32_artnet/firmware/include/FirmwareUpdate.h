#pragma once
#ifdef ARDUINO
class WebServer;
namespace led {
void registerFirmwareUpdate(WebServer& server);
}
#endif
