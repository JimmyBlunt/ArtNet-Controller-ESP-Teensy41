#include <Arduino.h>
#include <QNEthernet.h>
#include <FastLED.h>
#include "fl/channels/channel.h"
#include "platforms/arm/teensy/teensy4_common/drivers/objectfled/bus_traits.h"
#include "third_party/object_fled/src/ObjectFLEDDmaManager.h"
#include "octo_test_core.h"
#include "receiver_core.h"
#include "esp_output_profile.h"
#include "../../include/board_profiles/PjrcOctoAdapterT41.h"

namespace {
namespace board = artnet::boards::pjrc_octo_adapter_t41;
namespace test = octo_test;
static_assert(board::kOutputCount == test::kOutputs, "Eight physical outputs required");
static_assert(esp_profile::kOutputCount == test::kOutputs, "ESP physical output slots");
static_assert(artnet::kPixels == esp_profile::kPixels, "Build requires OCTO_ESP_PROFILE");
constexpr bool validEspSlots() {
    for (unsigned i = 0; i < esp_profile::kOutputCount; ++i)
        if (esp_profile::outputs[i].physicalOutput != i + 1 ||
            esp_profile::outputs[i].pixels > test::kMaxLength) return false;
    return esp_profile::outputs[7].pixels == 0;
}
static_assert(validEspSlots(), "Fixed ESP slot IDs and bounded buffer lengths required");

test::Config espConfig() {
    test::Config result;
    result.order = test::Order::GRB;
    for (unsigned i = 0; i < test::kOutputs; ++i)
        result.lengths[i] = esp_profile::outputs[i].pixels;
    return result;
}

// Stable per-output storage; disabled outputs never shift physical identities.
CRGB pixels[test::kOutputs][test::kMaxLength];
test::Config config = espConfig();
test::State state = test::State::Dormant;
bool configured = true, initialized = false, dmaPending = false;
artnet::Receiver receiver;
CRGB receivedPixels[artnet::kPixels];
static_assert(sizeof(CRGB) == 3, "Packed RGB copy required");
uint32_t armedAtMs = 0;
bool dmaEndSeen = false;
uint32_t dmaEndSeenUs = 0;
bool blackLatched = false;
unsigned selection = 0;
uint32_t testStartedMs = 0, lastTestDurationMs = 0, lastSubmitUs = 0;
uint32_t testDurationLimitMs = test::kTestDurationMs;
uint32_t wireGuardUs = 0, frames = 0, blackouts = 0, completed = 0;
uint32_t framePeriodUs = test::kPeriodUs;
uint32_t showCallUs = 0, maxShowCallUs = 0, dmaElapsedUs = 0, maxDmaElapsedUs = 0;
uint32_t lastReportMs = 0, lastReportFrames = 0, lastReportCompleted = 0;
uint32_t telemetrySkipped = 0;
uint32_t commandRepliesSkipped = 0;
qindesign::network::EthernetUDP udp(96);
bool udpStarted = false;
uint32_t udpDrained = 0, queuePeak = 0;
bool stopAckPending = false;

void reply(const char* text) {
    const size_t length = strlen(text);
    if (Serial && Serial.availableForWrite() >= int(length + 1)) {
        Serial.write(reinterpret_cast<const uint8_t*>(text), length);
        Serial.write('\n');
    } else ++commandRepliesSkipped;
}
void acknowledgeStop() {
    if (!stopAckPending) return;
    const char* message = initialized ? "OK STOP BLACK_DMA_COMPLETE\n" : "OK STOP NO_OUTPUT_INITIALIZED\n";
    const size_t length = strlen(message);
    if (Serial && Serial.availableForWrite() >= int(length)) {
        Serial.write(reinterpret_cast<const uint8_t*>(message), length);
        stopAckPending = false;
    }
}

EOrder fastLedOrder(test::Order order) {
    switch (order) {
        case test::Order::RGB: return RGB;
        case test::Order::RBG: return RBG;
        case test::Order::GRB: return GRB;
        case test::Order::GBR: return GBR;
        case test::Order::BRG: return BRG;
        case test::Order::BGR: return BGR;
    }
    return GRB;
}

void registerOutputs() {
    // Exact pinned modern Channel/ObjectFLED path, as in the RX32 main.cpp.
    // Called only by explicit ARM with the canonical boot profile or a valid TEST.
    FastLED.setExclusiveDriver<fl::Bus::OBJECT_FLED>();
    fl::ChannelOptions options;
    options.mBus = fl::Bus::OBJECT_FLED;
    options.mGamma = 1.0f;
    options.mDitherMode = 0;
    for (unsigned i = 0; i < test::kOutputs; ++i) {
        if (!config.lengths[i]) continue;
        fl::ClocklessChipset chipset(board::kOutputs[i].teensyPin,
                                     fl::makeTimingConfig<fl::TIMING_WS2812_800KHZ>());
        fl::ChannelConfig channel(chipset, fl::span<CRGB>(pixels[i], config.lengths[i]),
                                  fastLedOrder(config.order), options);
        FastLED.add(fl::Channel::create(channel));
        const uint32_t guard = config.lengths[i] * 30U + 300U;
        if (guard > wireGuardUs) wireGuardUs = guard;
    }
    FastLED.setBrightness(test::kBrightness);
    FastLED.setDither(0);
    if (wireGuardUs > framePeriodUs) framePeriodUs = wireGuardUs;
    initialized = true;
}

bool transferReady() {
    // DMA manager flags are not valid before the first show().
    if (!dmaPending) return true;
    if (fl::ObjectFLEDDmaManager::getInstance().isBusy()) return false;
    if (!dmaEndSeen) {
        dmaEndSeenUs = micros();
        dmaEndSeen = true;
    }
    // The first observed DMA completion is followed by an explicit reset/latch
    // interval, even if frame preparation took longer than the nominal wire time.
    if (uint32_t(micros() - dmaEndSeenUs) < 300U) return false;
    const uint32_t elapsed = micros() - lastSubmitUs;
    if (elapsed < wireGuardUs) return false;
    dmaPending = false;
    ++completed;
    dmaElapsedUs = elapsed;  // Observed completion, includes poll delay and latch guard.
    if (elapsed > maxDmaElapsedUs) maxDmaElapsedUs = elapsed;
    return true;
}

void clearPixels() { memset(static_cast<void*>(pixels), 0, sizeof(pixels)); }

void submit(bool blackout) {
    // Caller has checked transferReady before changing either pixel/DMA buffer.
    const uint32_t start = micros();
    FastLED.show();
    showCallUs = micros() - start;
    if (showCallUs > maxShowCallUs) maxShowCallUs = showCallUs;
    lastSubmitUs = start;
    dmaPending = true;
    dmaEndSeen = false;
    blackLatched = false;
    ++frames;
    if (blackout) ++blackouts;
}

void renderTest() {
    clearPixels();  // Full configured lengths of every unselected output are black.
    const uint32_t elapsed = millis() - testStartedMs;
    const unsigned color = test::colorIndex(elapsed);
    for (unsigned i = 0; i < test::kOutputs; ++i) {
        const unsigned physical = board::kOutputs[i].number;
        if (!test::selected(selection, physical, config) || !test::pulseLit(physical, elapsed))
            continue;
        const unsigned count = config.lengths[i] < physical ? config.lengths[i] : physical;
        for (unsigned j = 0; j < count; ++j) {
            if (color == 0) pixels[i][j].r = 255;
            else if (color == 1) pixels[i][j].g = 255;
            else pixels[i][j].b = 255;
        }
    }
    submit(false);
}

void renderArtNet() {
    // Only reached after transferReady: neither pixel nor DMA storage is changed
    // during a pending transfer. Receiver publishes only its latest complete frame.
    if (!receiver.take(reinterpret_cast<uint8_t*>(receivedPixels))) return;
    clearPixels();
    unsigned offset = 0;
    for (unsigned i = 0; i < esp_profile::kOutputCount; ++i) {
        const auto& output = esp_profile::outputs[i];
        if (output.pixels)
            ::memcpy(pixels[output.physicalOutput - 1], receivedPixels + offset, output.pixels * sizeof(CRGB));
        offset += output.pixels;
    }
    submit(false);
}

void report() {
    const uint32_t now = millis(), dt = now - lastReportMs;
    const uint32_t duration = state == test::State::Running ? now - testStartedMs : lastTestDurationMs;
    char line[2000];
    const auto ip = qindesign::network::Ethernet.localIP();
    const int n = snprintf(line, sizeof(line),
        "{\"firmware\":\"teensy41-octo-identify-rx32\",\"board_profile\":\"%s\","
        "\"source_profile\":\"%s\",\"artnet_profile_match\":%s,"
        "\"board_model_verified\":false,\"physical_outputs_verified\":false,"
        "\"state\":\"%s\",\"armed\":%s,\"configured\":%s,\"initialized\":%s,\"config_locked\":%s,"
        "\"led_type\":\"%s\",\"color_order\":\"%s\",\"pins\":[%u,%u,%u,%u,%u,%u,%u,%u],"
        "\"lengths\":[%u,%u,%u,%u,%u,%u,%u,%u],\"selection\":%u,\"brightness\":%u,"
        "\"start_universes\":[%u,%u,%u,%u,%u,%u,%u,%u],\"frame_period_us\":%lu,"
        "\"test_duration_ms\":%lu,\"test_limit_ms\":%lu,\"frames_submitted\":%lu,"
        "\"blackouts_submitted\":%lu,\"dma_completed\":%lu,\"dma_pending\":%s,\"black_latched\":%s,"
        "\"submit_fps\":%.3f,\"dma_completion_fps\":%.3f,\"show_call_us\":%lu,"
        "\"max_show_call_us\":%lu,\"dma_observed_us\":%lu,\"max_dma_observed_us\":%lu,"
        "\"telemetry_skipped\":%lu,\"command_replies_skipped\":%lu,"
        "\"ip\":\"%u.%u.%u.%u\",\"link\":%s,\"udp_listening\":%s,\"udp_received\":%lu,"
        "\"udp_queue_drops\":%lu,\"udp_drained\":%lu,\"queue_peak\":%lu,"
        "\"artnet_packets\":%lu,\"artnet_complete\":%lu,\"artnet_rejected\":%lu,"
        "\"artnet_ignored\":%lu,\"artnet_incomplete\":%lu,\"artnet_stale\":%lu,"
        "\"artnet_duplicates\":%lu,\"artnet_overwritten\":%lu,"
        "\"artnet_implemented\":true,\"physical_fps_verified\":false}\n",
        board::kId, esp_profile::kId, test::sameConfig(config, espConfig()) ? "true" : "false",
        test::stateName(state), state == test::State::RunningArtNet ? "true" : "false",
        configured ? "true" : "false",
        initialized ? "true" : "false", initialized ? "true" : "false",
        configured ? "WS2812B" : "UNSET", configured ? test::orderName(config.order) : "UNSET",
        board::kOutputs[0].teensyPin, board::kOutputs[1].teensyPin,
        board::kOutputs[2].teensyPin, board::kOutputs[3].teensyPin,
        board::kOutputs[4].teensyPin, board::kOutputs[5].teensyPin,
        board::kOutputs[6].teensyPin, board::kOutputs[7].teensyPin,
        config.lengths[0], config.lengths[1], config.lengths[2], config.lengths[3],
        config.lengths[4], config.lengths[5], config.lengths[6], config.lengths[7],
        selection, test::kBrightness,
        esp_profile::outputs[0].startUniverse, esp_profile::outputs[1].startUniverse,
        esp_profile::outputs[2].startUniverse, esp_profile::outputs[3].startUniverse,
        esp_profile::outputs[4].startUniverse, esp_profile::outputs[5].startUniverse,
        esp_profile::outputs[6].startUniverse, esp_profile::outputs[7].startUniverse,
        framePeriodUs, duration, testDurationLimitMs, frames, blackouts, completed,
        dmaPending ? "true" : "false", blackLatched ? "true" : "false",
        dt ? double(frames - lastReportFrames) * 1000.0 / dt : 0.0,
        dt ? double(completed - lastReportCompleted) * 1000.0 / dt : 0.0,
        showCallUs, maxShowCallUs, dmaElapsedUs, maxDmaElapsedUs, telemetrySkipped,
        commandRepliesSkipped, ip[0], ip[1], ip[2], ip[3],
        qindesign::network::Ethernet.linkState() ? "true" : "false", udpStarted ? "true" : "false",
        udp.totalReceiveCount(), udp.droppedReceiveCount(), udpDrained, queuePeak,
        receiver.counts.packets, receiver.counts.complete, receiver.counts.rejected,
        receiver.counts.ignored, receiver.counts.incomplete, receiver.counts.stale,
        receiver.counts.duplicate, receiver.counts.overwritten);
    if (Serial && n > 0 && n < int(sizeof(line)) && Serial.availableForWrite() >= n)
        Serial.write(reinterpret_cast<const uint8_t*>(line), n);
    else ++telemetrySkipped;
    lastReportMs = now;
    lastReportFrames = frames;
    lastReportCompleted = completed;
}

void stop() {
    if (state == test::State::Running) lastTestDurationMs = millis() - testStartedMs;
    state = test::requestStop(state);
    receiver.clear();
    if (state == test::State::Dormant || state == test::State::Stopped) stopAckPending = true;
    // Otherwise the acknowledgement follows the final black DMA + latch guard.
}

void command(char* line) {
    const auto cmd = test::parse(line);
    switch (cmd.kind) {
        case test::CommandKind::Status: report(); break;
        case test::CommandKind::Crash:
            if (test::running(state) || test::stopping(state)) reply("ERR STOP_REQUIRED");
            else if (Serial) Serial.print(CrashReport);
            break;
        case test::CommandKind::Arm:
            if (stopAckPending) reply("ERR STOP_ACK_PENDING");
            else if (test::running(state) || test::stopping(state)) reply("ERR STOP_REQUIRED");
            else if (!test::sameConfig(config, espConfig())) reply("ERR PROFILE_MISMATCH");
            else {
                if (!initialized) registerOutputs();
                receiver.clear();
                armedAtMs = millis();
                state = test::State::RunningArtNet;
                char response[80];
                snprintf(response, sizeof(response), "OK ARM %s", esp_profile::kId);
                reply(response);
            }
            break;
        case test::CommandKind::Stop: stop(); break;
        case test::CommandKind::Configure:
            if (test::stopping(state) || test::running(state))
                reply("ERR STOP_REQUIRED");
            else if (initialized && !test::sameConfig(config, cmd.config))
                reply("ERR REBOOT_REQUIRED");
            else {
                config = cmd.config;
                configured = true;
                reply("OK CONFIG WS2812B_800KHZ LENGTHS_ARE_FULL_CONNECTED_CHAIN_LENGTHS");
            }
            break;
        case test::CommandKind::Test:
            if (!configured) reply("ERR CONFIG_REQUIRED");
            else if (stopAckPending) reply("ERR STOP_ACK_PENDING");
            else if (test::stopping(state) || test::running(state))
                reply("ERR STOP_REQUIRED");
            else if (cmd.output && !config.lengths[cmd.output - 1])
                reply("ERR OUTPUT_NOT_CONFIGURED");
            else {
                if (!initialized) registerOutputs();
                selection = cmd.output;
                testStartedMs = millis();
                testDurationLimitMs = cmd.durationMs;
                lastTestDurationMs = 0;
                state = test::State::Running;
                char response[80];
                snprintf(response, sizeof(response), "OK TEST %u DURATION_MS %lu", selection, testDurationLimitMs);
                reply(response);
            }
            break;
        default: reply("ERR INVALID_COMMAND"); break;
    }
}

void serialCommands() {
    static char line[160];
    static unsigned pos = 0;
    static bool overflow = false;
    for (unsigned i = 0; i < 64 && Serial.available(); ++i) {
        const char c = Serial.read();
        if (c == '\n' || c == '\r') {
            line[pos] = 0;
            if (overflow) reply("ERR COMMAND_TOO_LONG_OR_CONTROL_CHARACTER");
            else if (pos) command(line);
            pos = 0;
            overflow = false;
        } else if ((c >= 32 && c <= 126) || c == '\t') {
            if (!overflow && pos + 1 < sizeof(line)) line[pos++] = c;
            else overflow = true;
        } else overflow = true;
    }
}
}  // namespace

void setup() {
    Serial.begin(115200);
    // No LED GPIO or channel registration at boot. Native Ethernet remains active.
    qindesign::network::Ethernet.setHostname("teensy-octo-identify");
    qindesign::network::Ethernet.begin();
    udpStarted = udp.begin(6454);
}

void loop() {
    qindesign::network::Ethernet.loop();
    if (!udpStarted) udpStarted = udp.begin(6454);
    serialCommands();
    // RX32-backed UDP reception remains active in all modes. Art-Net parsing uses
    // the exact ESP source profile; local TEST output remains independent.
    for (unsigned i = 0; i < 128; ++i) {
        const auto queued = udp.receiveQueueSize();
        if (queued > queuePeak) queuePeak = queued;
        const int n = udp.parsePacket();
        if (n < 0) break;
        if (n > 0) {
            ++udpDrained;
            receiver.ingest(udp.data(), udp.size(), udp.receivedTimestamp());
        }
    }
    const uint32_t now = millis();
    receiver.expire(now);
    if (state == test::State::Running && uint32_t(now - testStartedMs) >= testDurationLimitMs)
        stop();
    if (state == test::State::RunningArtNet &&
        (!qindesign::network::Ethernet.linkState() ||
         (uint32_t(now - armedAtMs) > 1000U && uint32_t(now - receiver.lastComplete()) > 1000U)))
        stop();
    const bool ready = transferReady();
    if (state == test::State::WaitBlackout && ready &&
        (!frames || uint32_t(micros() - lastSubmitUs) >= framePeriodUs)) {
        clearPixels();
        submit(true);
        state = test::advanceStop(state, true);
    } else if (state == test::State::WaitBlackoutCompletion && ready) {
        blackLatched = true;
        state = test::advanceStop(state, true);
        stopAckPending = true;
    } else if (state == test::State::Running && ready &&
               (!frames || uint32_t(micros() - lastSubmitUs) >= framePeriodUs)) {
        renderTest();  // Schedule from actual submission; never catch up missed frames.
    } else if (state == test::State::RunningArtNet && ready &&
               (!frames || uint32_t(micros() - lastSubmitUs) >= framePeriodUs)) {
        renderArtNet();
    }
    acknowledgeStop();
    if (uint32_t(now - lastReportMs) >= 1000) report();
}
