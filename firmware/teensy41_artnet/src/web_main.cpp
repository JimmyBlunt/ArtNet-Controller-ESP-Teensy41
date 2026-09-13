#include <Arduino.h>
#include <QNEthernet.h>
#include <FastLED.h>
#include <ArduinoJson.h>
#include "fl/channels/channel.h"
#include "platforms/arm/teensy/teensy4_common/drivers/objectfled/bus_traits.h"
#include "third_party/object_fled/src/ObjectFLEDDmaManager.h"
#include "web_controller.h"
#include "web_config_store.h"
#include "web_http.h"
#include "runtime_receiver.h"
#include "artnet_run_policy.h"
#include "esp_test_pattern.h"
#include "octo_test_core.h"
#include "esp_output_profile.h"
#include "../../include/board_profiles/PjrcOctoAdapterT41.h"

namespace {
namespace board = artnet::boards::pjrc_octo_adapter_t41;
namespace ot = octo_test;
using qindesign::network::Ethernet;
webcfg::Config desired, active, stored, bootNetwork;
webcfg::LoadResult storageState = webcfg::LoadResult::Defaults;
ot::State state = ot::State::Dormant;
artnet_run::Policy artnetMode;
runtime_artnet::Receiver receiver;
CRGB pixels[webcfg::kOutputs][webcfg::kMaxLength];
CRGB receivedPixels[webcfg::kOutputs * webcfg::kMaxLength];
static_assert(sizeof(CRGB) == 3, "Packed ArtDmx RGB bytes required");
qindesign::network::EthernetUDP udp(96);
bool initialized = false, udpStarted = false, dmaPending = false;
bool dmaEndSeen = false, blackLatched = false, rebootScheduled = false;
bool configurationFault = false;
uint32_t rebootAtMs = 0, lastSubmitUs = 0, dmaEndSeenUs = 0;
uint32_t wireGuardUs = 300, framePeriodUs = 33334;
uint32_t testStartedMs = 0, testLimitMs = 30000, lastTestDurationMs = 0;
unsigned selection = 0;
bool patternTest = false, patternLoop = false, patternPending = false, testReturnToArtNet = false;
uint32_t patternFrameIndex = 0, lastPatternFrameMs = 0;
uint8_t outputBrightness = 8;
uint32_t frames = 0, blackouts = 0, completed = 0;
uint32_t showCallUs = 0, maxShowCallUs = 0, dmaElapsedUs = 0, maxDmaElapsedUs = 0;
uint32_t udpDrained = 0, queuePeak = 0, telemetrySkipped = 0, commandRepliesSkipped = 0;
uint32_t lastReportMs = 0, lastSampleMs = 0, sampleFrames = 0, sampleCompleted = 0;
uint32_t sampleArtNetComplete = 0;
double submitFps = 0, dmaFps = 0, artnetCompleteFps = 0;
char lastError[160] = {};

bool fail(char* error, size_t size, const char* message) {
    snprintf(lastError, sizeof(lastError), "%s", message);
    if (error && size) snprintf(error, size, "%s", message);
    return false;
}
bool success(char* error, size_t size) {
    lastError[0] = 0;
    if (error && size) error[0] = 0;
    return true;
}
bool sameNetwork(const webcfg::Config& a, const webcfg::Config& b) {
    return a.dhcp == b.dhcp && !memcmp(a.ip, b.ip, 4) && !memcmp(a.netmask, b.netmask, 4) &&
           !memcmp(a.gateway, b.gateway, 4) && !memcmp(a.dns, b.dns, 4);
}
bool sameOutputs(const webcfg::Config& a, const webcfg::Config& b) {
    for (unsigned i = 0; i < webcfg::kOutputs; ++i) {
        const auto& x = a.outputs[i]; const auto& y = b.outputs[i];
        if (x.enabled != y.enabled || x.pixelCount != y.pixelCount ||
            x.startUniverse != y.startUniverse || x.colorOrder != y.colorOrder || x.reverse != y.reverse)
            return false;
    }
    return true;
}
bool rebootRequired() { return !sameOutputs(desired, active) || !sameNetwork(desired, bootNetwork); }
bool unsaved() { return !webcfg::equal(desired, stored) || storageState == webcfg::LoadResult::Corrupt; }
void reply(const char* message) {
    const size_t n = strlen(message);
    if (Serial && Serial.availableForWrite() >= int(n + 1)) {
        Serial.write(reinterpret_cast<const uint8_t*>(message), n); Serial.write('\n');
    } else ++commandRepliesSkipped;
}
bool configureReceiver(const webcfg::Config& config) {
    runtime_artnet::Port ports[runtime_artnet::kOutputs] = {};
    for (unsigned i = 0; i < webcfg::kOutputs; ++i) {
        ports[i].universe = config.outputs[i].startUniverse;
        ports[i].pixels = config.outputs[i].enabled ? config.outputs[i].pixelCount : 0;
    }
    if (!receiver.configure(ports)) return false;
    lastSampleMs = millis(); sampleFrames = frames; sampleCompleted = completed;
    sampleArtNetComplete = receiver.counts.complete;
    submitFps = dmaFps = artnetCompleteFps = 0;
    return true;
}
void timing() {
    wireGuardUs = 300;
    for (const auto& output : active.outputs)
        if (output.enabled && output.pixelCount * 30U + 300U > wireGuardUs)
            wireGuardUs = output.pixelCount * 30U + 300U;
    framePeriodUs = (1000000UL + active.targetFps - 1U) / active.targetFps;
    if (wireGuardUs > framePeriodUs) framePeriodUs = wireGuardUs;
}
EOrder fastLedOrder(webcfg::Order order) {
    switch (order) {
        case webcfg::Order::RGB: return RGB;
        case webcfg::Order::RBG: return RBG;
        case webcfg::Order::GRB: return GRB;
        case webcfg::Order::GBR: return GBR;
        case webcfg::Order::BRG: return BRG;
        case webcfg::Order::BGR: return BGR;
    }
    return GRB;
}
bool registerOutputs() {
    FastLED.setExclusiveDriver<fl::Bus::OBJECT_FLED>();
    fl::ChannelOptions options;
    options.mBus = fl::Bus::OBJECT_FLED;
    options.mGamma = 1.0f;
    options.mDitherMode = 0;
    fl::ChannelPtr channels[webcfg::kOutputs];
    for (unsigned i = 0; i < webcfg::kOutputs; ++i) {
        const auto& output = active.outputs[i];
        if (!output.enabled) continue;
        fl::ClocklessChipset chipset(board::kOutputs[i].teensyPin,
                                    fl::makeTimingConfig<fl::TIMING_WS2812_800KHZ>());
        fl::ChannelConfig channel(chipset, fl::span<CRGB>(pixels[i], output.pixelCount),
                                  fastLedOrder(output.colorOrder), options);
        channels[i] = fl::Channel::create(channel);
        if (!channels[i]) return false; // No draw-list registration until all creations succeed.
    }
    for (unsigned i = 0; i < webcfg::kOutputs; ++i)
        if (channels[i]) FastLED.add(channels[i]); // Pinned API returns void.
    FastLED.setDither(0);
    FastLED.setBrightness(outputBrightness);
    initialized = true;
    return true;
}
bool transferReady() {
    if (!dmaPending) return true;
    if (fl::ObjectFLEDDmaManager::getInstance().isBusy()) return false;
    if (!dmaEndSeen) { dmaEndSeen = true; dmaEndSeenUs = micros(); }
    if (uint32_t(micros() - dmaEndSeenUs) < 300U) return false;
    const uint32_t elapsed = micros() - lastSubmitUs;
    if (elapsed < wireGuardUs) return false;
    dmaPending = false; ++completed; dmaElapsedUs = elapsed;
    if (elapsed > maxDmaElapsedUs) maxDmaElapsedUs = elapsed;
    return true;
}
void clearPixels() { memset(static_cast<void*>(pixels), 0, sizeof(pixels)); }
void submit(bool black) {
    const uint32_t start = micros();
    FastLED.show();
    showCallUs = micros() - start;
    if (showCallUs > maxShowCallUs) maxShowCallUs = showCallUs;
    lastSubmitUs = start; dmaPending = true; dmaEndSeen = false; blackLatched = false;
    ++frames; if (black) ++blackouts;
}
void renderArtNet() {
    if (!receiver.take(reinterpret_cast<uint8_t*>(receivedPixels))) return;
    clearPixels();
    for (unsigned i = 0; i < webcfg::kOutputs; ++i) {
        const auto& output = active.outputs[i];
        if (!output.enabled) continue;
        const CRGB* source = receivedPixels + receiver.pixelOffset(i);
        if (!output.reverse) ::memcpy(pixels[i], source, output.pixelCount * sizeof(CRGB));
        else for (unsigned j = 0; j < output.pixelCount; ++j) pixels[i][output.pixelCount - 1 - j] = source[j];
    }
    submit(false);
}
void renderTest() {
    if (patternTest && uint32_t(millis() - lastPatternFrameMs) < esp_test::kFrameMs) return;
    clearPixels();
    const uint32_t elapsed = millis() - testStartedMs;
    if (patternTest) {
        lastPatternFrameMs = millis(); ++patternFrameIndex;
        const auto phase = esp_test::phase(elapsed, patternLoop);
        for (unsigned i = 0; i < webcfg::kOutputs; ++i) {
            const auto& output = active.outputs[i];
            if (!esp_test::selected(selection, board::kOutputs[i].number, output.enabled)) continue;
            for (unsigned j = 0; j < output.pixelCount; ++j) {
                const auto color = esp_test::pixel(phase, j, patternFrameIndex);
                pixels[i][output.reverse ? output.pixelCount - 1 - j : j] = CRGB(color.r, color.g, color.b);
            }
        }
        submit(false);
        return;
    }
    const unsigned color = ot::colorIndex(elapsed);
    for (unsigned i = 0; i < webcfg::kOutputs; ++i) {
        const auto& output = active.outputs[i];
        const unsigned physical = board::kOutputs[i].number;
        if (!output.enabled || (selection && selection != physical) || !ot::pulseLit(physical, elapsed)) continue;
        const unsigned count = output.pixelCount < physical ? output.pixelCount : physical;
        for (unsigned j = 0; j < count; ++j) {
            CRGB& pixel = pixels[i][output.reverse ? output.pixelCount - 1 - j : j];
            if (color == 0) pixel.r = 255;
            else if (color == 1) pixel.g = 255;
            else pixel.b = 255;
        }
    }
    submit(false);
}
const char* storageName() {
    switch (storageState) {
        case webcfg::LoadResult::Defaults: return "DEFAULTS";
        case webcfg::LoadResult::Loaded: return "LOADED";
        case webcfg::LoadResult::Corrupt: return "CORRUPT_DEFAULTS";
    }
    return "UNKNOWN";
}
void sampleRates() {
    const uint32_t now = millis(), dt = now - lastSampleMs;
    if (dt < 1000) return;
    submitFps = double(frames - sampleFrames) * 1000.0 / dt;
    dmaFps = double(completed - sampleCompleted) * 1000.0 / dt;
    artnetCompleteFps = double(receiver.counts.complete - sampleArtNetComplete) * 1000.0 / dt;
    lastSampleMs = now; sampleFrames = frames; sampleCompleted = completed;
    sampleArtNetComplete = receiver.counts.complete;
}
void serialReport() {
    static StaticJsonDocument<6144> document;
    static char line[2800];
    document.clear();
    controller::status(document.to<JsonObject>());
    document.remove("dma_fps"); document.remove("show_us_max");
    document.remove("dma_observed_us_max"); document.remove("udp_dropped");
    size_t required = measureJson(document);
    // Full configuration is available over HTTP; keep serial telemetry bounded.
    if (required + 1 >= sizeof(line)) {
        document.remove("color_orders"); document.remove("reverse");
        document["last_error"] = "SEE_WEB_STATUS";
        required = measureJson(document);
    }
    if (document.overflowed() || required + 1 >= sizeof(line)) { ++telemetrySkipped; return; }
    if (!Serial || Serial.availableForWrite() < int(required + 1)) { ++telemetrySkipped; return; }
    const size_t n = serializeJson(document, line, sizeof(line));
    Serial.write(reinterpret_cast<const uint8_t*>(line), n); Serial.write('\n');
}
void serialCommand(char* line) {
    char error[160];
    if (!strcmp(line, "STATUS")) { serialReport(); return; }
    if (!strcmp(line, "STOP")) { controller::stop(); return; }
    if (!strcmp(line, "ARM")) {
        reply(controller::start(error, sizeof(error)) ? "OK ARM" : error); return;
    }
    if (!strcmp(line, "CRASH")) {
        if (controller::isStopped() && Serial) Serial.print(CrashReport);
        else reply("ERR STOP_REQUIRED");
        return;
    }
    if (!strncmp(line, "CONFIG", 6)) { reply("ERR USE_WEB_CONFIG"); return; }
    const auto parsed = ot::parse(line);
    if (parsed.kind != ot::CommandKind::Test) { reply("ERR INVALID_COMMAND"); return; }
    reply(controller::test(parsed.output, parsed.durationMs / 1000U, error, sizeof(error)) ? "OK TEST" : error);
}
void serialCommands() {
    static char line[160]; static unsigned pos = 0; static bool overflow = false;
    for (unsigned i = 0; i < 64 && Serial.available(); ++i) {
        const char c = Serial.read();
        if (c == '\r' || c == '\n') {
            line[pos] = 0;
            if (overflow) reply("ERR COMMAND_TOO_LONG_OR_CONTROL_CHARACTER");
            else if (pos) serialCommand(line);
            pos = 0; overflow = false;
        } else if ((c >= 32 && c <= 126) || c == '\t') {
            if (!overflow && pos + 1 < sizeof(line)) line[pos++] = c;
            else overflow = true;
        } else overflow = true;
    }
}
IPAddress address(const uint8_t ip[4]) { return IPAddress(ip[0], ip[1], ip[2], ip[3]); }
} // namespace

namespace controller {
const webcfg::Config& config() { return desired; }
bool isStopped() { return !ot::running(state) && !ot::stopping(state) && !dmaPending; }
bool apply(const webcfg::Config& next, char* error, size_t size) {
    if (!isStopped()) return fail(error, size, "STOP_REQUIRED");
    if (rebootScheduled) return fail(error, size, "REBOOT_PENDING");
    char reason[160];
    if (!webcfg::validate(next, reason, sizeof(reason))) return fail(error, size, reason);
    if (!initialized && !configureReceiver(next)) return fail(error, size, "RECEIVER_CONFIG_INVALID");
    desired = next;
    if (!initialized) for (unsigned i = 0; i < webcfg::kOutputs; ++i) active.outputs[i] = next.outputs[i];
    active.brightness = next.brightness; active.targetFps = next.targetFps;
    outputBrightness = active.brightness;
    if (initialized) FastLED.setBrightness(outputBrightness);
    timing();
    configurationFault = false;
    return success(error, size);
}
bool save(char* error, size_t size) {
    if (!isStopped()) return fail(error, size, "STOP_REQUIRED");
    if (rebootScheduled) return fail(error, size, "REBOOT_PENDING");
    if (!webcfg::save(desired)) return fail(error, size, "EEPROM_SAVE_FAILED");
    stored = desired; storageState = webcfg::LoadResult::Loaded;
    configurationFault = false;
    return success(error, size);
}
bool start(char* error, size_t size) {
    if (!isStopped()) return fail(error, size, "STOP_REQUIRED");
    if (rebootScheduled) return fail(error, size, "REBOOT_PENDING");
    if (configurationFault) return fail(error, size, "CONFIGURATION_FAULT_ACKNOWLEDGE_WITH_APPLY_OR_SAVE");
    if (rebootRequired()) return fail(error, size, "SAVE_AND_REBOOT_REQUIRED");
    if (!webcfg::totalPixels(active)) return fail(error, size, "NO_ENABLED_OUTPUTS");
    outputBrightness = active.brightness;
    if (!initialized && !registerOutputs()) return fail(error, size, "CHANNEL_CREATION_FAILED");
    else FastLED.setBrightness(outputBrightness);
    receiver.clear(); artnetMode.start();
    // Establish black on boot/start before accepting complete live frames.
    state = ot::State::WaitBlackout;
    return success(error, size);
}
bool test(unsigned output, unsigned seconds, char* error, size_t size) {
    if (output > webcfg::kOutputs || seconds < 1 || seconds > 600) return fail(error, size, "INVALID_TEST_ARGUMENTS");
    if (!isStopped()) return fail(error, size, "STOP_REQUIRED");
    if (rebootScheduled) return fail(error, size, "REBOOT_PENDING");
    if (configurationFault) return fail(error, size, "CONFIGURATION_FAULT_ACKNOWLEDGE_WITH_APPLY_OR_SAVE");
    if (rebootRequired()) return fail(error, size, "SAVE_AND_REBOOT_REQUIRED");
    if (!webcfg::totalPixels(active)) return fail(error, size, "NO_ENABLED_OUTPUTS");
    if (output && !active.outputs[output - 1].enabled) return fail(error, size, "OUTPUT_NOT_CONFIGURED");
    outputBrightness = active.brightness < 8 ? active.brightness : 8;
    if (!initialized && !registerOutputs()) return fail(error, size, "CHANNEL_CREATION_FAILED");
    else FastLED.setBrightness(outputBrightness);
    artnetMode.stop();
    patternTest = patternLoop = patternPending = testReturnToArtNet = false;
    selection = output; testStartedMs = millis(); testLimitMs = seconds * 1000U;
    lastTestDurationMs = 0; state = ot::State::Running;
    return success(error, size);
}
bool pattern(unsigned output, bool loop, char* error, size_t size) {
    if (output > webcfg::kOutputs) return fail(error, size, "OUTPUT_RANGE_0_8");
    if (rebootScheduled) return fail(error, size, "REBOOT_PENDING");
    if (configurationFault) return fail(error, size, "CONFIGURATION_FAULT_ACKNOWLEDGE_WITH_APPLY_OR_SAVE");
    if (rebootRequired()) return fail(error, size, "SAVE_AND_REBOOT_REQUIRED");
    if (!webcfg::totalPixels(active)) return fail(error, size, "NO_ENABLED_OUTPUTS");
    if (output && !active.outputs[output - 1].enabled) return fail(error, size, "OUTPUT_NOT_CONFIGURED");
    if (!initialized && !registerOutputs()) return fail(error, size, "CHANNEL_CREATION_FAILED");
    testReturnToArtNet = artnetMode.enabled() || testReturnToArtNet;
    artnetMode.stop(); receiver.clear();
    selection = output; patternLoop = loop; patternPending = patternTest = true;
    testLimitMs = loop ? 0 : esp_test::kCycleMs;
    lastTestDurationMs = 0;
    // Change ownership only after previous DMA and a complete black frame.
    if (!ot::stopping(state)) state = ot::State::WaitBlackout;
    return success(error, size);
}
void endPattern() {
    if (!patternTest && !patternPending) return;
    if (state == ot::State::Running) lastTestDurationMs = millis() - testStartedMs;
    patternTest = patternLoop = patternPending = false;
    receiver.clear();
    if (testReturnToArtNet) artnetMode.start();
    testReturnToArtNet = false;
    state = ot::requestStop(state);
}
void stop() {
    artnetMode.stop();
    patternTest = patternLoop = patternPending = testReturnToArtNet = false;
    if (state == ot::State::Running) lastTestDurationMs = millis() - testStartedMs;
    state = ot::requestStop(state); receiver.clear();
    if (state == ot::State::Dormant) reply("OK STOP NO_OUTPUT_INITIALIZED");
    else if (state == ot::State::Stopped) reply("OK STOP BLACK_DMA_COMPLETE");
}
bool reboot(char* error, size_t size) {
    if (!isStopped()) return fail(error, size, "STOP_REQUIRED");
    if (unsaved()) return fail(error, size, "UNSAVED_CHANGES");
    if (!rebootScheduled) { rebootScheduled = true; rebootAtMs = millis() + 700U; }
    return success(error, size);
}
void status(JsonObject out) {
    sampleRates();
    out["firmware"] = "teensy41-octo-web-rx32";
    out["build_revision"] = "orbital-port-tests-20260913";
    out["boot_mode"] = "ARTNET_ON";
    out["artnet_waiting"] = artnetMode.waiting();
    out["board_profile"] = board::kId; out["source_profile"] = esp_profile::kId;
    out["state"] = ot::stateName(state); out["armed"] = artnetMode.enabled();
    out["configured"] = true; out["initialized"] = initialized;
    out["config_locked"] = !isStopped() || rebootScheduled; out["channels_locked"] = initialized;
    out["configuration_fault"] = configurationFault;
    out["reboot_required"] = rebootRequired(); out["reboot_pending"] = rebootScheduled;
    out["unsaved"] = unsaved(); out["storage_state"] = storageName(); out["last_error"] = lastError;
    out["board_model_verified"] = false; out["physical_outputs_verified"] = false;
    out["physical_fps_verified"] = false; out["artnet_implemented"] = true;
    out["artnet_profile_match"] = sameOutputs(active, webcfg::defaults());
    out["led_type"] = "WS2812B"; out["brightness"] = outputBrightness;
    out["target_fps"] = active.targetFps; out["frame_period_us"] = framePeriodUs;
    out["total_pixels"] = webcfg::totalPixels(active); out["expected_universes"] = receiver.universeCount();
    JsonArray pins = out.createNestedArray("pins"), lengths = out.createNestedArray("lengths");
    JsonArray universes = out.createNestedArray("start_universes");
    JsonArray orders = out.createNestedArray("color_orders"), reverse = out.createNestedArray("reverse");
    for (unsigned i = 0; i < webcfg::kOutputs; ++i) {
        pins.add(board::kOutputs[i].teensyPin);
        lengths.add(active.outputs[i].enabled ? active.outputs[i].pixelCount : 0);
        universes.add(active.outputs[i].startUniverse);
        orders.add(webcfg::orderName(active.outputs[i].colorOrder)); reverse.add(active.outputs[i].reverse);
    }
    const auto ip = Ethernet.localIP(); char ipText[16];
    snprintf(ipText, sizeof(ipText), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    out["ip"] = ipText; out["link"] = Ethernet.linkState(); out["dhcp"] = bootNetwork.dhcp;
    out["udp_listening"] = udpStarted; out["udp_received"] = udp.totalReceiveCount();
    out["udp_queue_drops"] = udp.droppedReceiveCount(); out["udp_drained"] = udpDrained;
    out["queue_peak"] = queuePeak; out["selection"] = selection;
    out["test_pattern"] = patternTest;
    out["test_loop"] = patternLoop;
    out["test_pending"] = patternPending;
    out["test_frame_index"] = patternFrameIndex;
    out["test_resume_artnet"] = testReturnToArtNet;
    out["test_phase"] = patternPending ? "starting" : patternTest ?
        esp_test::name(esp_test::phase(millis() - testStartedMs, patternLoop)) : "stopped";
    out["test_duration_ms"] = state == ot::State::Running ? millis() - testStartedMs : lastTestDurationMs;
    out["test_limit_ms"] = testLimitMs; out["frames_submitted"] = frames;
    out["blackouts_submitted"] = blackouts; out["dma_completed"] = completed;
    out["dma_pending"] = dmaPending; out["black_latched"] = blackLatched;
    out["submit_fps"] = submitFps; out["dma_completion_fps"] = dmaFps;
    out["dma_fps"] = dmaFps; out["artnet_complete_fps"] = artnetCompleteFps;
    out["show_call_us"] = showCallUs; out["max_show_call_us"] = maxShowCallUs;
    out["dma_observed_us"] = dmaElapsedUs; out["max_dma_observed_us"] = maxDmaElapsedUs;
    out["show_us_max"] = maxShowCallUs; out["dma_observed_us_max"] = maxDmaElapsedUs;
    out["udp_dropped"] = udp.droppedReceiveCount();
    out["telemetry_skipped"] = telemetrySkipped; out["command_replies_skipped"] = commandRepliesSkipped;
    out["artnet_packets"] = receiver.counts.packets; out["artnet_complete"] = receiver.counts.complete;
    out["artnet_rejected"] = receiver.counts.rejected; out["artnet_ignored"] = receiver.counts.ignored;
    out["artnet_incomplete"] = receiver.counts.incomplete; out["artnet_stale"] = receiver.counts.stale;
    out["artnet_duplicates"] = receiver.counts.duplicate; out["artnet_overwritten"] = receiver.counts.overwritten;
}
} // namespace controller

void setup() {
    Serial.begin(115200);
    storageState = webcfg::load(desired);
    configurationFault = storageState == webcfg::LoadResult::Corrupt;
    active = stored = bootNetwork = desired;
    configureReceiver(active); timing(); outputBrightness = active.brightness;
    if (storageState == webcfg::LoadResult::Corrupt) snprintf(lastError, sizeof(lastError), "EEPROM_CORRUPT_DEFAULTS_LOADED");
    Ethernet.setHostname("teensy-octo-controller");
    if (active.dhcp) Ethernet.begin();
    else {
        Ethernet.begin(address(active.ip), address(active.netmask), address(active.gateway));
        Ethernet.setDNSServerIP(address(active.dns));
    }
    udpStarted = udp.begin(6454);
    http::begin();
    // No network wait: normal boot is ON and accepts a sender arriving later.
    // start() retains all configuration/driver checks and reports failures.
    if (!configurationFault) controller::start(nullptr, 0);
}

void loop() {
    Ethernet.loop();
    if (!udpStarted) udpStarted = udp.begin(6454);
    serialCommands();
    for (unsigned i = 0; i < 128; ++i) {
        const auto queued = udp.receiveQueueSize(); if (queued > queuePeak) queuePeak = queued;
        const int n = udp.parsePacket(); if (n < 0) break;
        if (n > 0) { ++udpDrained; receiver.ingest(udp.data(), udp.size(), udp.receivedTimestamp()); }
    }
    http::poll();
    const uint32_t now = millis(); receiver.expire(now);
    if (state == ot::State::Running && (!patternTest || !patternLoop) &&
        uint32_t(now - testStartedMs) >= testLimitMs) {
        if (patternTest) controller::endPattern(); else controller::stop();
    }
    artnet_run::Action streamAction = artnet_run::Action::Wait;
    if (state == ot::State::RunningArtNet) {
        const bool link = Ethernet.linkState();
        const bool fresh = uint32_t(now - receiver.lastComplete()) <= 1000U;
        streamAction = artnetMode.observe(link, fresh, receiver.ready());
        if (streamAction == artnet_run::Action::Discard ||
            streamAction == artnet_run::Action::Blackout) receiver.clear();
        if (streamAction == artnet_run::Action::Blackout)
            state = ot::requestStop(state); // Keep ON intent during loss/black DMA.
    }
    const bool ready = transferReady();
    const bool due = !frames || uint32_t(micros() - lastSubmitUs) >= framePeriodUs;
    if (state == ot::State::WaitBlackout && ready && due) {
        clearPixels(); submit(true); state = ot::State::WaitBlackoutCompletion;
    } else if (state == ot::State::WaitBlackoutCompletion && ready) {
        blackLatched = true;
        if (patternPending) {
            patternPending = false; testStartedMs = now;
            patternFrameIndex = 0; lastPatternFrameMs = now - esp_test::kFrameMs;
            outputBrightness = active.brightness < 36 ? active.brightness : 36;
            FastLED.setBrightness(outputBrightness);
            state = ot::State::Running;
        } else {
            if (artnetMode.enabled()) {
                receiver.clear(); outputBrightness = active.brightness;
                FastLED.setBrightness(outputBrightness);
            }
            state = artnetMode.enabled() ? ot::State::RunningArtNet : ot::State::Stopped;
            reply(artnetMode.enabled() ? "OK ARTNET ON WAITING_FOR_DATA" : "OK STOP BLACK_DMA_COMPLETE");
        }
    } else if (state == ot::State::Running && ready && due) renderTest();
    else if (state == ot::State::RunningArtNet && ready && due &&
             streamAction == artnet_run::Action::Render) renderArtNet();
    if (rebootScheduled && controller::isStopped() && int32_t(now - rebootAtMs) >= 0) {
        __asm__ volatile("dsb" ::: "memory");
        SCB_AIRCR = 0x05FA0004;
        __asm__ volatile("dsb" ::: "memory");
    }
    if (uint32_t(now - lastReportMs) >= 1000) { lastReportMs = now; serialReport(); }
}
