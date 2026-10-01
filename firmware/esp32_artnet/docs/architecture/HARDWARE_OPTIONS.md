# Hardware Options

| Option | Build status | Runtime evidence | Notes |
|---|---|---|---|
| ESP32 WiFi | Builds | Boot, HTTP API, CORS, Art-Net RX, controller preview, and one short measured benchmark passed | Useful fallback and development target. The measured 4,500 pixel / 30 FPS WiFi run dropped many packets, so it is not recommended as the final single-controller transport for the full mixed load. |
| ESP32 RMII Ethernet | Builds | Not hardware-tested in this workspace | Preferred ESP32 Ethernet option because it avoids W5500/APA102 SPI contention. Needs real board wiring and packet-loss benchmark. |
| ESP32 W5500 | Builds | Not hardware-tested in this workspace | Usable with caution. It shares SPI-class constraints with APA102 and therefore needs bus-locking and packet-loss measurements before final use. |
| Teensy 4.1 Native Ethernet | Roadmap | User noted a Teensy was flashed, but this repo has no verified Teensy firmware/runtime benchmark yet | Best candidate for mixed high-pixel-count output because native Ethernet and multiple high-speed output paths fit the load better. Needs a port and measurement pass. |

Deployment recommendation for first hardware bring-up: use RMII Ethernet for ESP32 if APA102 hardware SPI is needed. For larger installs, split APA102 and WS2812B across nodes or move final platform to Teensy 4.1 after benchmark comparison.

## Decision Matrix

| Option | Recommendation | Reason |
|---|---|---|
| One controller for everything | Not recommended on ESP32 WiFi | The measured ESP32 WiFi API run for 4,500 pixels at 30 FPS received only 646 of 2,430 sent packets and completed 2 of 90 sent frames. |
| Two controllers split by LED type | Recommended next hardware test | Separating APA102 and WS2812B reduces per-node universe count and avoids timing conflicts between clocked SPI LEDs and one-wire LEDs. |
| Several identical nodes | Recommended for final scalability | Multiple nodes keep universe count, output timing, heap pressure, and web/API latency per device lower. |
| ESP32 as final platform | Conditional | Viable only after RMII/W5500 hardware tests show stable packet reception and LED output timing at the target load. WiFi alone is not enough for the full mixed target. |
| Teensy 4.1 as final platform | Strong roadmap candidate | Native Ethernet and higher headroom make it the better architecture for high-pixel-count mixed output, but it is not yet implemented or benchmarked in this repo. |

## Hardware Risks To Verify

| Risk | ESP32 WiFi | ESP32 RMII | ESP32 W5500 | Teensy 4.1 |
|---|---|---|---|---|
| SPI conflicts | Low for network, still relevant for APA102 output | Low for network, relevant only for APA102 output | High if W5500 and APA102 contend for SPI | Low to medium depending on selected output libraries |
| Pin demand | Low | High | Medium | Medium |
| Parallel LED output | Limited | Limited | Limited plus SPI contention | Better candidate |
| Stability under Art-Net load | Weak in measured WiFi test | Unknown, expected better | Unknown | Unknown, expected best |
| Cost | Low | Low to medium | Low to medium | Higher |
| Complexity | Low | Medium | Medium-high | Medium-high because firmware port is required |
