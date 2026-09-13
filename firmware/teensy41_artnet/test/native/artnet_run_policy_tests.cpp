#include <cassert>
#include <iostream>
#include "../../include/artnet_run_policy.h"
#include "../../include/octo_test_core.h"
#include "../../include/runtime_receiver.h"

int main() {
    using artnet_run::Action;
    artnet_run::Policy mode;
    assert(!mode.enabled());
    mode.start();
    // Boot without cable/sender stays ON indefinitely, without repeated blackouts.
    for (int i = 0; i < 100; ++i) {
        assert(mode.observe(false, false, false) == Action::Discard);
        assert(mode.enabled() && mode.waiting());
    }
    assert(mode.observe(true, true, false) == Action::Wait); // incomplete frame
    assert(mode.observe(true, true, true) == Action::Render); // delayed sender
    assert(!mode.waiting());
    assert(mode.observe(true, true, false) == Action::Wait);
    assert(!mode.waiting()); // holding a valid frame between packets is normal
    assert(mode.observe(true, false, true) == Action::Blackout); // stale queued frame
    assert(mode.enabled() && mode.waiting());
    assert(mode.observe(true, false, true) == Action::Discard); // only one blackout
    assert(mode.observe(true, true, true) == Action::Render); // sender returns
    assert(mode.observe(false, true, true) == Action::Blackout); // link loss
    auto state = octo_test::State::RunningArtNet;
    state = octo_test::requestStop(state);
    assert(octo_test::advanceStop(state, false) == state); // wait previous DMA
    state = octo_test::advanceStop(state, true);
    assert(octo_test::advanceStop(state, false) == state); // wait black DMA/latch
    mode.stop(); // manual STOP while recovering cancels automatic restart
    state = octo_test::advanceStop(state, true);
    assert(state == octo_test::State::Stopped && !mode.enabled());
    assert(mode.observe(true, true, true) == Action::Wait);
    assert(!mode.waiting());
    mode.start(); // next boot or explicit start restores ON
    assert(mode.enabled() && mode.waiting());
    assert(mode.observe(true, true, true) == Action::Render);
    // Regression: the first multi-universe frame arrives over separate loop
    // iterations after a long silence; no complete timestamp exists yet.
    runtime_artnet::Receiver receiver;
    runtime_artnet::Port ports[8] = {{1, 1}, {2, 1}};
    assert(receiver.configure(ports));
    mode.start();
    for (unsigned universe = 1; universe <= 2; ++universe) {
        uint8_t packet[22] = {'A','r','t','-','N','e','t',0,0,0x50,0,14,1,0,
                              uint8_t(universe),0,0,4,0,0,0,0};
        receiver.ingest(packet, sizeof(packet), 5000 + universe);
        const auto action = mode.observe(true, (5000 + universe - receiver.lastComplete()) <= 1000,
                                         receiver.ready());
        if (action == Action::Discard || action == Action::Blackout) receiver.clear();
        assert(action == (universe == 1 ? Action::Wait : Action::Render));
    }
    assert(receiver.counts.complete == 1 && receiver.ready());
    std::cout << "Art-Net boot/wait/loss/recovery/manual STOP tests passed\n";
}
