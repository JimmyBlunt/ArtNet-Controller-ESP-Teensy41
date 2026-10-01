#include <Arduino.h>
#include <FastLED.h>

constexpr uint16_t NUM_LEDS = 60;
constexpr uint8_t BRIGHTNESS = 32;
constexpr uint32_t SERIAL_BAUD = 115200;

// Clocked LEDs have two signal wires:
// green wire -> DI / data input
// red wire   -> CI / clock input
//
// Change these two Teensy pin numbers to the pads where green/red are connected.
constexpr uint8_t DATA_PIN_DI_GREEN = 20;
constexpr uint8_t CLOCK_PIN_CI_RED = 21;

CRGB leds[NUM_LEDS];
uint32_t lastLogMs = 0;
uint32_t frameCount = 0;

void fillStripTestPattern() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  const uint16_t head = (millis() / 35) % NUM_LEDS;

  for (uint8_t tail = 0; tail < 8; tail++) {
    const uint16_t pos = (head + NUM_LEDS - tail) % NUM_LEDS;
    leds[pos] = CHSV(96, 255, 255 - (tail * 28));
  }

  leds[0] = CRGB::White;
}

void showSolid(const CRGB &color, const char *name, uint16_t holdMs) {
  Serial.print("Test: ");
  Serial.println(name);
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
  delay(holdMs);
}

void printStartupInfo() {
  Serial.println();
  Serial.println("=== Teensy 4.1 APA102 FastLED test ===");
  Serial.print("FastLED version: ");
  Serial.println(FASTLED_VERSION);
  Serial.print("Data DI green Teensy pin: ");
  Serial.println(DATA_PIN_DI_GREEN);
  Serial.print("Clock CI red Teensy pin: ");
  Serial.println(CLOCK_PIN_CI_RED);
  Serial.print("LED count: ");
  Serial.println(NUM_LEDS);
  Serial.print("Brightness: ");
  Serial.println(BRIGHTNESS);
  Serial.println("Wiring: green -> DI, red -> CI, LED GND must share Teensy GND.");

#if defined(__IMXRT1062__)
  if (CrashReport) {
    Serial.println("Previous crash report:");
    Serial.print(CrashReport);
  } else {
    Serial.println("No previous crash report.");
  }
#endif
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(SERIAL_BAUD);
  delay(1500);

  FastLED.addLeds<APA102, DATA_PIN_DI_GREEN, CLOCK_PIN_CI_RED, BGR>(leds, NUM_LEDS);

  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear(true);

  printStartupInfo();

  showSolid(CRGB::Red, "all red", 700);
  showSolid(CRGB::Green, "all green", 700);
  showSolid(CRGB::Blue, "all blue", 700);
  showSolid(CRGB::White, "all white low brightness", 700);
  showSolid(CRGB::Black, "all off", 300);

  Serial.println("Entering chase loop...");
}

void loop() {
  fillStripTestPattern();
  FastLED.show();

  frameCount++;
  digitalWrite(LED_BUILTIN, (frameCount / 15) % 2);

  const uint32_t now = millis();
  if (now - lastLogMs >= 1000) {
    lastLogMs = now;
    Serial.print("running ms=");
    Serial.print(now);
    Serial.print(" frames=");
    Serial.print(frameCount);
    Serial.print(" DI=");
    Serial.print(DATA_PIN_DI_GREEN);
    Serial.print(" CI=");
    Serial.println(CLOCK_PIN_CI_RED);
  }
}
