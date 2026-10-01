#pragma once

namespace artnet_run {
enum class Action { Wait, Render, Blackout, Discard };

// User intent survives signal loss, but never an explicit STOP or test.
class Policy {
 public:
    void start() { enabled_ = true; waiting_ = true; }
    void stop() { enabled_ = false; waiting_ = true; }
    bool enabled() const { return enabled_; }
    bool waiting() const { return enabled_ && waiting_; }
    Action observe(bool link, bool fresh, bool completeReady) {
        if (!enabled_) return Action::Wait;
        if (!link || !fresh) {
            const bool wasPlaying = !waiting_;
            waiting_ = true;
            if (wasPlaying) return Action::Blackout;
            // While waiting for the first complete frame, retain its partial
            // universes across loop iterations. Only discard an actually stale
            // complete frame or data received without a link.
            return (!link || completeReady) ? Action::Discard : Action::Wait;
        }
        if (!completeReady) return Action::Wait;
        waiting_ = false;
        return Action::Render;
    }
 private:
    bool enabled_ = false, waiting_ = true;
};
} // namespace artnet_run
