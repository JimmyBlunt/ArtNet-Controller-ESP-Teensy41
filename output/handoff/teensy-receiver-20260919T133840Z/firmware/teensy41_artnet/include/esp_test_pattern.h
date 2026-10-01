#pragma once
#include <stdint.h>

// Port of Schrank-LED HardwareTest's WS2812B sequence: 1s black, 6s per
// RGB chase, 1s black. Four pixels per 32; 50ms frames. Physical OUT IDs
// and DMA ownership remain in the Teensy controller.
namespace esp_test {
constexpr uint32_t kCycleMs = 20000, kFrameMs = 50;
enum class Phase { Black, Red, Green, Blue, FinalBlack, Done };
inline Phase phase(uint32_t elapsed, bool loop) {
    if (loop) elapsed %= kCycleMs;
    if (elapsed < 1000) return Phase::Black;
    if (elapsed < 7000) return Phase::Red;
    if (elapsed < 13000) return Phase::Green;
    if (elapsed < 19000) return Phase::Blue;
    if (elapsed < kCycleMs) return Phase::FinalBlack;
    return Phase::Done;
}
inline const char* name(Phase phase) {
    switch (phase) {
        case Phase::Black: return "blackout";
        case Phase::Red: return "all-red-chase";
        case Phase::Green: return "all-green-chase";
        case Phase::Blue: return "all-blue-chase";
        case Phase::FinalBlack: return "final-blackout";
        case Phase::Done: return "stopped";
    }
    return "stopped";
}
struct Color { uint8_t r=0, g=0, b=0; };
inline Color pixel(Phase phase, unsigned logicalPixel, uint32_t frameIndex) {
    if ((logicalPixel + frameIndex % 32) % 32 >= 4) return {};
    // Normalize the ESP 36:18:18 values; the driver caps global brightness
    // at 36 and honors lower configured brightness without losing green/blue.
    if (phase == Phase::Red) return {255, 0, 0};
    if (phase == Phase::Green) return {0, 128, 0};
    if (phase == Phase::Blue) return {0, 0, 128};
    return {};
}
inline bool selected(unsigned output, unsigned physical, bool enabled) {
    return enabled && (output == 0 || output == physical);
}
} // namespace esp_test
