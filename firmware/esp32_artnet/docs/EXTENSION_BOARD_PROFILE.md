# Aktuelle Softwarebelegung (Abgleich 01.10.2026)

Der aktuelle Code erlaubt WS-Pins 25,26,27,14,19,18,5,17 und APA102-Paare
DATA/CLOCK 18/5 oder 26/27. Default: GPIO25, 256 LEDs. Validierung derzeit
maximal 1200 LEDs je Ausgang. Die physische Platinenzuordnung ist damit nicht
neu bestätigt. `esp32-wifi-esp251` hat dazu inkompatible alte Defaults;
Details in [../README.md](../README.md).

Die folgende ursprüngliche 30P-Dokumentation ist **historisch** und beschreibt
nicht die aktuelle Pinliste des Quellcodes. Ihre früheren Uploadbefehle und
Gerätezuordnungen nicht ungeprüft auf vorhandene Nodes anwenden.

---

# ESP32 30P Extensionboard profile

The dedicated PlatformIO environment is `esp32-wifi-extensionboard`. It builds the
flexible Art-Net controller for the photographed purple 30-pin G/V/S breakout board.

## LED ports

The six signal pins routed to the external WS2812 connectors are available in this
order:

| Board label | ESP32 GPIO | Use |
| --- | ---: | --- |
| P14 | 14 | WS2812B data |
| P27 | 27 | WS2812B data |
| P26 | 26 | WS2812B data |
| P25 | 25 | WS2812B data |
| P33 | 33 | WS2812B data |
| P32 | 32 | WS2812B data |
| P35 | 35 | Input only; unavailable |
| P34 | 34 | Input only; unavailable |

APA102 uses dedicated pins on the opposite side. Each selection reserves both pins:

| APA102 data | APA102 clock |
| --- | --- |
| P16 / GPIO16 | P17 / GPIO17 |
| P18 / GPIO18 | P19 / GPIO19 |
| P21 / GPIO21 | P22 / GPIO22 |

P0, P2, P5, P12 and P15 are unavailable because they affect ESP32 boot strapping.
RX/TX remain available for flashing and serial diagnostics. P4 and P23 are kept as
unassigned reserve pins. The controller accepts at most eight configured outputs,
1024 LEDs per output and 8192 LEDs total. GPIO reuse across WS2812B and APA102 is
rejected by both the browser and firmware.

## Power connections

The yellow `JUMP` selector changes the voltage on the red `V` row of the G/V/S
headers between 3.3 V and 5 V. It does not change the ESP32 GPIO logic voltage.
Leave it on 3.3 V unless a connected small peripheral explicitly requires 5 V on
the V pin.

The USB connector on the black ESP32 module carries programming data and power.
Use it for flashing and serial diagnostics. The USB-C and Micro-USB connectors on
the purple extension board are 5 V power inputs. The barrel connector accepts
6.5-16 V DC through the extension board regulator. Use only one board power input
at a time.

Large LED chains require their own correctly sized power supply. Do not route their
load current through the ESP32 or extension-board V rail. Join LED-supply GND to
ESP32 GND so the data signal has a common reference. A 74AHCT125 or 74HCT245 level
shifter can translate the ESP32 3.3 V data signal to 5 V for longer or less tolerant
LED wiring.

Board interface reference:
https://manuals.plus/ae/1005005553236672

## Build and flash

```powershell
.\.venv-platformio\Scripts\platformio.exe run -e esp32-wifi-extensionboard -t upload --upload-port COM6
```
