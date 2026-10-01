# Teensy 4.1 Porting Plan

1. Keep the shared C++ core unchanged: Art-Net assembly, mapping, output routing, performance math, preview protocol.
2. Replace ESP32 network adapter with Teensy Native Ethernet.
3. Add APA102 outputs over multiple SPI buses.
4. Add WS2812B output through FastLED first, then evaluate OctoWS2811 or DMA parallel output.
5. Preserve REST API, mapping JSON, universe ranges, and Processing preview protocol.
6. Re-run benchmark matrix against ESP32 WiFi, ESP32 RMII, ESP32 W5500, and Teensy 4.1.
