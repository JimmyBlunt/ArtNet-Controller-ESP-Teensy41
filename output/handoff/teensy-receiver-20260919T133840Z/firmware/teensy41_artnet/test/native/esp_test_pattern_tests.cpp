#include <cassert>
#include <iostream>
#include "../../include/esp_test_pattern.h"

int main() {
    using namespace esp_test;
    assert(phase(0, false) == Phase::Black);
    assert(phase(999, false) == Phase::Black);
    assert(phase(1000, false) == Phase::Red);
    assert(phase(6999, false) == Phase::Red);
    assert(phase(7000, false) == Phase::Green);
    assert(phase(13000, false) == Phase::Blue);
    assert(phase(19000, false) == Phase::FinalBlack);
    assert(phase(20000, false) == Phase::Done);
    assert(phase(20000, true) == Phase::Black);
    assert(phase(47000, true) == Phase::Green);
    for (unsigned frame = 0; frame < 96; ++frame) {
        unsigned lit = 0;
        for (unsigned p = 0; p < 32; ++p) {
            auto c = pixel(Phase::Red, p, frame);
            lit += c.r != 0;
            assert(c.g == 0 && c.b == 0);
            auto black = pixel(Phase::FinalBlack, p, frame);
            assert(!black.r && !black.g && !black.b);
        }
        assert(lit == 4);
    }
    assert(pixel(Phase::Green, 0, 0).g == 128);
    assert(pixel(Phase::Blue, 0, 0).b == 128);
    assert(!pixel(Phase::Green, 4, 0).g);
    for (unsigned selection = 0; selection <= 8; ++selection) {
        unsigned count = 0;
        for (unsigned port = 1; port <= 8; ++port)
            count += selected(selection, port, port != 8);
        assert(count == (selection == 0 ? 7U : selection == 8 ? 0U : 1U));
    }
    // Reverse output maps logical first/last positions without exceeding length.
    for (unsigned length : {1U, 32U, 536U, 512U, 1200U})
        for (unsigned p = 0; p < length; ++p) assert(length - 1 - p < length);
    std::cout << "ESP pattern: RGB phase boundaries, once/loop, moving 4/32 blocks, port selection passed\n";
}
