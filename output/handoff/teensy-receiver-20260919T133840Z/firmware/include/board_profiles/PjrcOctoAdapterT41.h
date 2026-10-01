#pragma once

#include <stddef.h>
#include <stdint.h>

namespace artnet {
namespace boards {
namespace pjrc_octo_adapter_t41 {

// Wiring profile only; this does not select or initialize an LED driver.
// Integrate with the verified FastLED Channel/ObjectFLED path, not OctoWS2811.
// Standard documented at https://www.pjrc.com/store/octo28_adaptor.html
// Actual adapter model, orientation and optical operation remain unverified.
constexpr char kId[] = "PJRC_OCTO_ADAPTER_T41";
constexpr uint8_t kRevision = 1;

enum class Jack : uint8_t { PjrcTop = 1, PjrcBottom = 2 };

struct Output {
    uint8_t number;       // Physical/logical OUT1..OUT8; never renumber on disable.
    uint8_t teensyPin;
    Jack jack;            // PJRC drawing orientation, not the installed orientation.
    uint8_t pair;         // 1..4 within each jack.
    uint8_t dataContact;  // RJ45 contact, not Teensy GPIO.
    uint8_t groundContact;
};

constexpr Output kOutputs[] = {
    {1,  2, Jack::PjrcTop,    1, 2, 1},
    {2, 14, Jack::PjrcTop,    2, 4, 5},
    {3,  7, Jack::PjrcTop,    3, 6, 3},
    {4,  8, Jack::PjrcTop,    4, 8, 7},
    {5,  6, Jack::PjrcBottom, 1, 2, 1},
    {6, 20, Jack::PjrcBottom, 2, 4, 5},
    {7, 21, Jack::PjrcBottom, 3, 6, 3},
    {8,  5, Jack::PjrcBottom, 4, 8, 7},
};
constexpr size_t kOutputCount = sizeof(kOutputs) / sizeof(kOutputs[0]);

// nullptr prevents invalid output numbers from silently selecting GPIO 0.
constexpr const Output* findOutput(unsigned number) {
    return number >= 1 && number <= kOutputCount ? &kOutputs[number - 1] : nullptr;
}

// For import validation if the existing configuration also stores dataPin.
// A pin valid elsewhere on Teensy is still invalid for a different RJ45 output.
constexpr bool matchesOutputPin(unsigned number, unsigned pin) {
    return findOutput(number) != nullptr && findOutput(number)->teensyPin == pin;
}

static_assert(kOutputCount == 8, "PJRC Octo adapter has eight fixed outputs");

}  // namespace pjrc_octo_adapter_t41
}  // namespace boards
}  // namespace artnet
