# Teensy 4.1 FastLED DI/CI Clocked LED Test

Minimaler PlatformIO-Test fuer Teensy 4.1 mit clocked LEDs: `DI` plus `CI`.

Wichtig: Wenn dein LED-Strip `DI` und `CI` hat, ist das kein WS2812B/NeoPixel-clockless Strip. Deshalb nutzt dieser Sketch `APA102`-artige clocked Ausgabe statt ObjectFLED/WS2812.

## Anpassen

In `src/main.cpp` zuerst diese Werte pruefen:

```cpp
constexpr uint16_t NUM_LEDS = 60;
constexpr uint8_t DATA_PIN_DI_GREEN = 20;
constexpr uint8_t CLOCK_PIN_CI_RED = 21;
```

Gruen geht an `DI` / Data Input auf Teensy Pin 20. Rot geht an `CI` / Clock Input auf Teensy Pin 21.

Wenn die Farben falsch wirken, probiere zuerst die Farbfolge `RGB`, `GRB` oder `BGR` in dieser Zeile:

```cpp
FastLED.addLeds<APA102, DATA_PIN_DI_GREEN, CLOCK_PIN_CI_RED, BGR>(leds, NUM_LEDS);
```

## Flashen

```bash
pio run -e teensy41 -t upload
```

Falls `pio` noch fehlt:

```bash
python3 -m pip install --user "platformio<6.2"
```

Danach den Teensy per USB verbinden und beim Upload gegebenenfalls den Program-Knopf am Teensy druecken.

## Erwartetes Bild

Beim Start zeigt der Strip nacheinander Rot, Gruen, Blau, Weiss und Aus. Danach laeuft ein Chase-Effekt. Die Onboard-LED am Teensy blinkt als Lebenszeichen.

Oeffne den Serial Monitor mit `115200` Baud. Erwartete Ausgabe:

```text
=== Teensy 4.1 APA102 FastLED test ===
FastLED version: ...
Data DI green Teensy pin: 20
Clock CI red Teensy pin: 21
LED count: 60
Entering chase loop...
running ms=...
```

## Hinweise

- Datenleitungen am Teensy 4.1 sind 3.3 V. Viele LED-Strips laufen damit, sauberer ist aber ein 74HCT/74AHCT-Levelshifter auf 5 V.
- LED-Strom separat einspeisen. Teensy-GND und LED-Netzteil-GND muessen verbunden sein.
- Fuer vier getrennte clocked Strips brauchst du pro Strip zwei Signalpins: DI und CI.
- Wenn der Serial Monitor laeuft, aber keine LEDs leuchten: Richtung des Strips pruefen (`DI/CI` ist Eingang), GND zwischen Netzteil/Strip/Teensy verbinden, 5 V am Strip messen, und testweise DI/CI tauschen.
