#include <cmath>
#include <cstring>
#include <iostream>
#include <string>

#include "ConfigManager.h"
#include "LedColorOrder.h"
#include "ArtNetReceiver.h"
#include "HardwareTest.h"
#include "LedOutputs.h"
#include "MappingEngine.h"
#include "OutputRouter.h"
#include "Performance.h"
#include "PerformanceMonitor.h"
#include "PreviewProtocol.h"
#include "UniverseAssembler.h"
#include "WebApi.h"

using namespace led;

static int failures = 0;

static void expect(bool condition, const std::string& message) {
  if (!condition) {
    failures++;
    std::cerr << "FAIL: " << message << "\n";
  }
}

static UniversePacket packet(uint16_t universe, uint8_t sequence, uint8_t base) {
  UniversePacket p;
  p.universe = universe;
  p.sequence = sequence;
  p.data.resize(512);
  for (size_t i = 0; i < p.data.size(); ++i) p.data[i] = static_cast<uint8_t>(base + i);
  return p;
}

static void testPerformanceMath() {
  expect(universesForPixels(1) == 1, "one pixel needs one universe");
  expect(universesForPixels(170) == 1, "170 RGB pixels fit in one universe");
  expect(universesForPixels(171) == 2, "171 RGB pixels need two universes");
  expect(std::abs(ws2812FrameTimeUs(500) - 15300.0) < 0.01, "WS2812 timing");
  expect(apa102FrameTimeUs(500, 4000000) > 4000.0, "APA102 timing at 4 MHz");
  expect(artNetDataRateMbps(4500, 60) > 6.0, "Art-Net data rate");
}

static void testAssembler() {
  UniverseAssembler assembler(10, 2, 200);
  expect(assembler.accept(packet(10, 1, 0)), "accept first universe");
  expect(!assembler.frameReady(), "frame incomplete after one universe");
  expect(assembler.accept(packet(11, 2, 10)), "accept second universe");
  expect(assembler.frameReady(), "frame complete");
  LogicalPixelBuffer frame;
  assembler.compose(frame);
  expect(frame.size() == 200, "composed pixel count");
  expect(frame.get(0).r == 0 && frame.get(0).g == 1 && frame.get(0).b == 2,
         "first pixel RGB");
  expect(assembler.stats().framesComplete == 1, "complete frame counted");
}

static void testPerOutputUniverseRouting() {
  ControllerConfig config;
  config.pixelCount = 4;
  config.startUniverse = 0;
  config.targetFps = 30;
  config.outputs = {
      {0, OutputType::WS2812B, true, 0, 2, 32, -1, ColorOrder::GRB, false, 0, 5, 30},
      {1, OutputType::WS2812B, true, 2, 2, 27, -1, ColorOrder::GRB, false, 0, 100, 30},
  };
  config.universeCount = mappedUniverseCount(config);
  expect(config.universeCount == 2, "sparse output mapping stores only used universes");
  UniverseAssembler assembler(config);
  expect(!assembler.accept(packet(6, 1, 9)), "unassigned universe is rejected");
  expect(assembler.accept(packet(5, 1, 1)), "first output universe accepted");
  expect(assembler.accept(packet(100, 1, 50)), "distant second output universe accepted");
  expect(assembler.frameReady(), "sparse mapped frame completes");
  LogicalPixelBuffer frame;
  assembler.compose(frame);
  expect(frame.get(0).r == 1 && frame.get(2).r == 50,
         "each output starts at channel one of its selected universe");
}

static void testOutputRouter() {
  ControllerConfig config = defaultMixedConfig();
  std::string error;
  expect(validateConfig(config, &error), "default mixed config validates: " + error);
  LogicalPixelBuffer frame(config.pixelCount);
  frame.set(1500, Rgb{1, 2, 3});
  frame.set(1999, Rgb{4, 5, 6});
  OutputRouter router(config.outputs);
  auto ws = router.sliceForOutput(4, frame);
  expect(ws.size() == 500, "WS slice size");
  expect(ws.front().r == 1 && ws.back().r == 4, "WS slice preserves order");
  config.outputs[1].startPixel = 400;
  expect(!validateConfig(config, &error), "overlap is rejected");
}

static void testMapping() {
  LogicalPixelBuffer src(3);
  src.set(0, Rgb{1, 0, 0});
  src.set(1, Rgb{0, 2, 0});
  src.set(2, Rgb{0, 0, 3});
  MappingEngine engine;
  engine.setMapping(Mapping{"reverse3", MappingMode::Points1D, 3, {2, 1, 0}, {}});
  std::string error;
  expect(engine.validate(&error), "reverse mapping validates");
  auto out = engine.apply(src);
  expect(out.get(0).b == 3 && out.get(2).r == 1, "index mapping applies");
  engine.setMapping(Mapping{"bad", MappingMode::Points3D, 1, {}, {Point3{NAN, 0, 0}}});
  expect(!engine.validate(&error), "NaN rejected");
  auto contained = normalizeContain({Point3{10, 0, 0}, Point3{20, 10, 0}});
  expect(contained[1].x <= 1.0f && contained[1].y <= 1.0f, "contain normalization");
}

static void testPreviewProtocol() {
  PreviewHeader header;
  header.frameId = 42;
  header.pixelCount = 2;
  std::vector<Rgb> pixels = {Rgb{1, 2, 3}, Rgb{4, 5, 6}};
  auto encoded = encodePreviewChunk(header, pixels, 0, 2);
  PreviewHeader decoded;
  std::vector<Rgb> out;
  expect(decodePreviewChunk(encoded, decoded, out), "preview packet decodes");
  expect(decoded.frameId == 42 && out.size() == 2 && out[1].g == 5, "preview fields");
}

static void testArtNetParser() {
  std::vector<uint8_t> bytes(18 + 6, 0);
  std::memcpy(bytes.data(), "Art-Net\0", 8);
  bytes[8] = 0x00;
  bytes[9] = 0x50;
  bytes[12] = 9;
  bytes[14] = 42;
  bytes[16] = 0;
  bytes[17] = 6;
  bytes[18] = 1;
  bytes[19] = 2;
  bytes[20] = 3;
  bytes[21] = 4;
  bytes[22] = 5;
  bytes[23] = 6;
  UniversePacket packet;
  expect(ArtNetReceiver::parseArtDmx(bytes.data(), bytes.size(), packet), "ArtDMX parses");
  expect(packet.universe == 42 && packet.sequence == 9 && packet.data.size() == 6,
         "ArtDMX fields");
  bytes[9] = 0x21;
  expect(!ArtNetReceiver::parseArtDmx(bytes.data(), bytes.size(), packet),
         "non-ArtDMX opcode rejected");
}

static void testPerformanceMonitor() {
  PerformanceMonitor monitor;
  monitor.begin(0);
  monitor.markPacket(10);
  monitor.markPacket(20);
  monitor.markFrameStart(100);
  monitor.markOutputDone(600);
  monitor.update(1000, 120000, 100000);
  const auto& snap = monitor.snapshot();
  expect(snap.fps == 1, "performance fps");
  expect(snap.packetsPerSecond == 2, "performance packet rate");
  expect(snap.outputTimeUs == 500, "performance output time");
  expect(snap.heapFree == 120000 && snap.heapMinFree == 100000, "performance heap");
}

static void testWebApiJsonAndLedManager() {
  ControllerConfig config = defaultMixedConfig();
  SystemStats stats;
  PerformanceMonitor perf;
  perf.begin(0);
  perf.update(1000, 777, 555);
  const auto status = WebApi::statusJson(config, stats, perf.snapshot());
  const auto outputs = WebApi::outputsJson(config);
  expect(status.find("\"pixelCount\":4500") != std::string::npos, "status json pixel count");
  expect(status.find("\"frameTimeUs\":0") != std::string::npos, "status json frame time");
  expect(status.find("\"outputTimeUs\":0") != std::string::npos, "status json output time");
  expect(status.find("\"lastFrameLatencyMs\":null") != std::string::npos, "unmeasured latency is null");
  expect(outputs.find("\"type\":\"APA102\"") != std::string::npos, "outputs json APA102");
  expect(outputs.find("\"type\":\"WS2812B\"") != std::string::npos, "outputs json WS2812B");
  expect(outputs.find("\"colorOrder\":\"BGR\"") != std::string::npos,
         "outputs json includes APA102 color order");
  expect(outputs.find("\"startUniverse\":") != std::string::npos,
         "outputs json includes output start universe");
  const std::string index = WebApi::indexHtml();
  expect(index.find("ArtNet LED Controller") != std::string::npos, "shared web menu title");
  expect(index.find("/api/test-pattern") != std::string::npos, "embedded web menu test api");
  expect(index.find("/api/preview/target") != std::string::npos,
         "embedded web menu preview api");
  expect(index.find("Auto Layout") != std::string::npos, "embedded web menu auto layout");
  expect(index.find("normalizeLayout") != std::string::npos,
         "embedded web menu normalizes output layout");
  LedOutputManager manager;
  expect(manager.begin(config), "LED output manager begins");
  expect(manager.outputCount() == config.outputs.size(), "LED output manager output count");
  LogicalPixelBuffer frame(config.pixelCount);
  manager.show(frame);
  manager.show(frame);
  const auto& outputStats = manager.stats();
  expect(outputStats.size() == config.outputs.size(), "LED output manager stats count");
  expect(!outputStats.empty() && outputStats.front().totalUpdates == 2,
         "LED output manager counts updates");
}

static void testHardwareTestSequence() {
  ControllerConfig config;
  config.pixelCount = 1679;
  config.startUniverse = 0;
  config.universeCount = universesForPixels(config.pixelCount);
  config.outputs = {
      {0, OutputType::WS2812B, true, 0, 512, 23, -1, ColorOrder::GRB, false, 0, 0, 30},
      {1, OutputType::APA102, true, 512, 1167, 18, 19, ColorOrder::BGR, false, 4000000, 4, 30},
  };
  LogicalPixelBuffer frame(config.pixelCount);
  HardwareTest test;
  test.begin(&config);
  test.start(1000);
  expect(test.active(), "hardware test starts");
  expect(test.update(2000, &frame), "hardware test updates ws phase");
  expect(test.statusJson().find("all-red-chase") != std::string::npos,
         "hardware test ws phase");
  expect(frame.get(0).r > 0 || frame.get(1).r > 0 || frame.get(2).r > 0 ||
             frame.get(3).r > 0,
         "hardware test lights ws sparse pixels");
  frame.clear();
  expect(test.update(9000, &frame), "hardware test updates green phase");
  expect(test.statusJson().find("all-green-chase") != std::string::npos,
         "hardware test apa phase");
  bool apaLit = false;
  for (uint32_t i = 512; i < 552; ++i) apaLit = apaLit || frame.get(i).g > 0;
  expect(apaLit, "hardware test lights apa sparse pixels");
  test.stop(&frame);
  expect(test.update(10000, &frame), "stop schedules a physical black frame even with no network data");
  expect(!test.active(), "hardware test releases control after black frame");
  expect(frame.get(512).r == 0 && frame.get(512).g == 0 && frame.get(512).b == 0,
         "hardware test stop clears frame");
  test.startLoop(1000);
  expect(test.active(), "hardware test loop starts");
  expect(test.update(33000, &frame), "hardware test loop wraps");
  expect(test.active(), "hardware test loop remains active after cycle");
  expect(test.statusJson().find("\"loop\":true") != std::string::npos,
         "hardware test loop status");
  test.blackout();
  frame.set(0, Rgb{20, 30, 40});
  expect(test.update(34000, &frame), "blackout emits black frame");
  expect(test.active() && frame.get(0).r == 0, "blackout keeps ownership after clearing");
  expect(!test.update(35000, &frame), "held blackout avoids unnecessary refreshes");
  test.stop();
  expect(test.update(36000, &frame) && !test.active(), "stop releases latched blackout");
  test.start(40000, 1);
  test.update(42000, &frame);
  expect(frame.get(0).r == 0, "APA-only test does not light WS output");
}

static void testAssemblerRecovery() {
  UniverseAssembler assembler(0, 2, 200);
  auto shortPacket = packet(0, 1, 10);
  shortPacket.data.resize(6);
  assembler.accept(shortPacket, 10);
  assembler.accept(packet(1, 1, 77), 11);
  LogicalPixelBuffer frame;
  assembler.compose(frame);
  expect(frame.get(2).r == 0 && frame.get(170).r == 77,
         "short universe preserves next universe's fixed pixel offset");
  expect(!assembler.accept(packet(0, 1, 0), 12), "duplicate sequence rejected");
  assembler.accept(packet(0, 2, 0), 20);
  expect(assembler.expire(120), "partial frame expires");
  assembler.accept(packet(1, 2, 0), 121);
  expect(!assembler.frameReady(), "late universe cannot complete expired collection");
  assembler.accept(packet(1, 3, 0), 122);
  expect(assembler.stats().framesIncomplete == 2, "repeated universe abandons partial collection");
  assembler.accept(packet(0, 3, 0), 123);
  expect(assembler.frameReady(), "new collection recovers");
  assembler.compose(frame);
  UniverseAssembler wrap(0, 1, 1);
  wrap.accept(packet(0, 255, 0), 1);
  wrap.compose(frame);
  expect(wrap.accept(packet(0, 1, 0), 2), "sequence wraps from 255 to 1");
  wrap.compose(frame);
  expect(!wrap.accept(packet(0, 254, 0), 3), "out-of-order sequence rejected");
  expect(wrap.accept(packet(0, 0, 0), 4), "disabled sequence accepted");
  wrap.compose(frame);
  expect(wrap.accept(packet(0, 0, 0), 5), "sequence zero can repeat");
  UniverseAssembler clockWrap(0, 2, 200);
  clockWrap.accept(packet(0, 0, 0), 0xfffffff0u);
  expect(clockWrap.expire(100), "timeout survives millis rollover");
}

static void testRuntimeHardwareConfigValidation() {
  ControllerConfig config;
  config.pixelCount = 1879;
  config.startUniverse = 0;
  config.universeCount = universesForPixels(config.pixelCount);
  config.targetFps = 30;
  config.outputs = {
      {0, OutputType::WS2812B, true, 0, 712, 23, -1, ColorOrder::GRB, false, 0, 0, 30},
      {1, OutputType::APA102, true, 712, 1167, 18, 19, ColorOrder::BGR, false, 4000000, 4, 30},
  };
  std::string error;
  expect(validateRuntimeHardwareConfig(config, &error), "runtime config accepts WS2812 712");
  expect(config.universeCount == 12, "runtime config 1879 pixels needs 12 universes");

  auto invalidPin = config;
  invalidPin.outputs[0].dataPin = 22;
  expect(!validateRuntimeHardwareConfig(invalidPin, &error), "runtime config rejects bad WS pin");

  const ColorOrder orders[] = {ColorOrder::RGB, ColorOrder::RBG, ColorOrder::GRB,
                               ColorOrder::GBR, ColorOrder::BRG, ColorOrder::BGR};
  const uint8_t expected[][3] = {{17, 83, 211}, {17, 211, 83}, {83, 17, 211},
                                {83, 211, 17}, {211, 17, 83}, {211, 83, 17}};
  for (size_t i = 0; i < 6; ++i) {
    auto changed = config;
    for (auto& output : changed.outputs) output.colorOrder = orders[i];
    expect(validateRuntimeHardwareConfig(changed, &error), "all six color orders accepted");
    for (auto type : {OutputType::APA102, OutputType::WS2812B}) {
      const auto c = colorForFixedDriver({17, 83, 211}, orders[i], type);
      const Rgb wire = type == OutputType::APA102 ? Rgb{c.b, c.g, c.r} : Rgb{c.g, c.r, c.b};
      expect(wire.r == expected[i][0] && wire.g == expected[i][1] && wire.b == expected[i][2],
             "fixed FastLED driver emits the selected wire byte order");
    }
  }

  auto invalidColor = config;
  invalidColor.outputs[0].colorOrder = static_cast<ColorOrder>(6);
  expect(!validateRuntimeHardwareConfig(invalidColor, &error),
         "runtime config rejects unsupported WS color order");

  auto tooManyWs = config;
  tooManyWs.outputs[0].pixelCount = 1025;
  tooManyWs.outputs[1].startPixel = 1025;
  tooManyWs.pixelCount = 2192;
  tooManyWs.universeCount = universesForPixels(tooManyWs.pixelCount);
  expect(!validateRuntimeHardwareConfig(tooManyWs, &error),
         "runtime config rejects WS over compiled limit");

  auto tooManyTotal = config;
  tooManyTotal.pixelCount = 2401;
  tooManyTotal.outputs[1].pixelCount = 1689;
  expect(!validateRuntimeHardwareConfig(tooManyTotal, &error),
         "runtime config rejects total pixel limit");
}

int main() {
  testPerformanceMath();
  testAssembler();
  testPerOutputUniverseRouting();
  testAssemblerRecovery();
  testOutputRouter();
  testMapping();
  testPreviewProtocol();
  testArtNetParser();
  testPerformanceMonitor();
  testWebApiJsonAndLedManager();
  testHardwareTestSequence();
  testRuntimeHardwareConfigValidation();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "All firmware core tests passed\n";
  return 0;
}
