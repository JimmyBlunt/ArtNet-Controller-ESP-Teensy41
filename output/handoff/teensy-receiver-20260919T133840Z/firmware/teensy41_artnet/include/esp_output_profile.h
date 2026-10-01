#pragma once
#include <stdint.h>

// User-requested migration: ESP id0..4 -> OUT1..5; former id5 -> OUT6/OUT7.
// Source: controller-backup-10.0.0.249-pre-ota-1200-limit-2026-09-11.json,
// later restored and verified on ESP .248 per Schrank-LED/STATUS.md.
// GPIOs are deliberately supplied only by PJRC_OCTO_ADAPTER_T41.
namespace esp_profile {
// User correction 2026-09-13: former OUT6 becomes OUT6=536 and OUT7=512.
// This deliberately adds 24 pixels compared with the saved 1024-pixel chain.
constexpr char kId[] = "ESP4031_SPLIT_20260913";
constexpr unsigned kPixels = 4031;
constexpr unsigned kOutputCount = 8;
struct Output {
    unsigned physicalOutput;
    uint16_t pixels;
    uint16_t startUniverse;
};
constexpr Output outputs[kOutputCount] = {
    {1, 203, 120}, {2, 738, 122}, {3, 880, 127}, {4, 810, 133},
    {5, 352, 139}, {6, 536, 142}, {7, 512, 146}, {8, 0, 0},
};
// All source outputs and both split targets: WS2812B / GRB / reverse=false / targetFps=30.
// Disabled outputs' universe zero is unused, not an active route.
static_assert(203 + 738 + 880 + 810 + 352 + 536 + 512 == kPixels, "Corrected profile size");
}
