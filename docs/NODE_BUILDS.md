# Art-Net-Node-Builds

Stand: 01.10.2026. Diese Übersicht trennt Quellstand, Buildfähigkeit und belegte
Geräteprüfungen. Ein erfolgreicher Compilerlauf bestätigt keine Pinbelegung,
Netzwerkstabilität oder sichtbare LED-Ausgabe. IP-/COM-Angaben in älteren Berichten
sind historische Zuordnungen und müssen vor einem Upload frisch geprüft werden.

## Aktive Projekte

| Projekt | Build-Einstieg | Bevorzugtes Profil | Ausgabe / Netzwerk |
|---|---|---|---|
| [Teensy](../firmware/teensy41_artnet/README_DE.md) | `firmware/teensy41_artnet/build.ps1` | `teensy41_octo_web_rx32` | FastLED Channel API / ObjectFLED, natives Ethernet mit QNEthernet RX32 |
| [ESP32](../firmware/esp32_artnet/README.md) | `firmware/esp32_artnet/build.ps1` | `esp32-wifi-flex8` | FastLED WS2812B/APA102, WLAN, Webkonfiguration und OTA |

Beide Projekte enthalten Webquellen, Generatoren, Bildassets, Firmwarequellen und
Tests. Teensy bindet das Hintergrundbild ins Programm ein; beim ESP gehört
zusätzlich ein SPIFFS-Image aus `web/assets` zum vollständigen Build.
Die Buildskripte führen keinen Upload aus.

## Teensy-Umgebungen

Alle sechs Umgebungen sind in `firmware/teensy41_artnet/platformio.ini` definiert.

| Umgebung | Quell-Einstieg | Bedeutung / Einschränkung |
|---|---|---|
| `teensy41_octo_web_rx32` | `src/web_main.cpp`, `src/web_http.cpp` | Aktueller Webstand `orbital-port-tests-20260913`; Autostart AN, feste Octo-Pins, EEPROM-Konfiguration, Porttests einmal/Loop und animierte Vorschau, Orbital mit 50 % Deckkraft |
| `teensy41_octo_identify_rx32` | `src/octo_identify.cpp` | USB-Identifikation und Einzel-/Paralleltests; feste Octo-Pins, kein Webinterface; Ausgabe über ausdrücklichen Test/ARM |
| `teensy41_safe` | `src/main.cpp` | Empfang/USB-Diagnose ohne initialisierte LED-Ausgänge; Standard bei direktem `pio run` |
| `teensy41_driver_compile` | `src/main.cpp` | Reiner Compiler-Test; Fixture-Pins sind keine Verdrahtungsvorgabe |
| `teensy41_unwired_bench` | `src/main.cpp` | Historischer GPIO2–8-Lasttest, nur für unverdrahteten Aufbau; kein Octo-RJ45-Profil |
| `teensy41_hardware` | `src/main.cpp` | Benötigt eigene geprüfte `include/hardware_pins.h`; im Repository absichtlich nicht konfiguriert, kein freigegebener Octo-Build |

Boardprofil: `PJRC_OCTO_ADAPTER_T41`, OUT1–8 = **2,14,7,8,6,20,21,5**.
Octo-Adapter bleibt als Pegelwandler/RJ45-Träger; keine OctoWS2811-Bibliothek.
Kompilierte Defaults: 203/738/880/810/352/536/512/0 LEDs, Startuniverses
120/122/127/133/139/142/146/150; OUT8 deaktiviert. Das sind 4031 aktive Pixel
und 29 erwartete Universes. Die lesende Aufnahme vom 19.09.2026 zeigte davon
abweichend OUT6=610, insgesamt 4105 Pixel. Siehe [Empfangspfad](ARTNET_RECEIVER.md).

Abhängigkeiten: PlatformIO 6.1.18, Teensy-Plattform 6.0.0,
Arduino-Teensy 1.162.0, GCC-Toolchain 1.150201.0, Teensy-Tool 1.162.0,
SCons 4.40801.0, FastLED 3.10.4, QNEthernet 0.37.0, im Webbuild ArduinoJson 6.21.6.
Die SHA-gesicherten FastLED-/QNEthernet-Hooks sind erforderliche Buildinputs.

## ESP32-Umgebungen

Alle elf Umgebungen sind in `firmware/esp32_artnet/platformio.ini` definiert.
Die nachstehenden Werte sind Quellcode-Defaults; NVS-Konfigurationen können sie ersetzen.

| Umgebung | Funktion / Default | Einordnung |
|---|---|---|
| `esp32-wifi-flex8` | Ein WS2812B-Port GPIO32, 256 LEDs ab U0; konfigurierbare WS-/APA-Ausgänge | Profil des zuletzt am 15.09. dokumentierten ESP `.251`; Farbreihenfolge pro Port und SPIFFS-Hintergrund enthalten |
| `esp32-wifi-extensionboard` | Ein WS2812B-Port GPIO25, 256 LEDs ab U0; eingeschränkte Pinliste | Eigenes Boardprofil; Quellcode-Pins siehe ESP-README, ältere 30P-Dokumentation ist überholt |
| `esp32-wifi-esp251` | WS2812B 285 an GPIO32 + APA102 1024 an 18/19; U0/U2 | Erbt Flex8; historische Defaults und gespeicherte `.251`-Konfiguration mit APA102 18/19 und 25/26 werden durch den Profiltest geprüft |
| `esp32-wifi-ws2812-apa102-1679` | WS2812B 512 an GPIO23 + APA102 1167 an 18/19 | Feste ältere Mischkonfiguration |
| `esp32-wifi-ws2812-output0` | WS2812B 256 an GPIO23 | Einzelport-Testprofil |
| `esp32-wifi-matrix16` | APA102 256 an DATA23/CLOCK18 | 16×16-Testprofil |
| `esp32-wifi` | Allgemeine 4500-Pixel-Mischkonfiguration | Entwicklungsprofil; kein automatisch passendes Profil für vorhandene Nodes |
| `esp32-rmii-ethernet` | Allgemeines Mischprofil, `ETH.begin()` | Boardabhängige RMII-Hardware nicht hier abgenommen |
| `esp32-w5500` | Allgemeines Mischprofil | Netzwerk-Initialisierung ist ausdrücklich Platzhalter, keine betriebsfähige W5500-Freigabe |
| `esp32-power-diagnostic` | Nur `PowerDiagnostic.cpp`, USB/WLAN-Versorgungsdiagnose | Keine LED-Treiber, API oder Konfigurationswrites; kein Art-Net-Betriebsimage |
| `native` | Host-Entwicklungsprofil | Tests bevorzugt über `tools/test_host.py`; kein flashbares Image |

Für den importierten ESP-Stand jetzt festgelegt: PlatformIO 6.1.19,
Espressif32-Plattform 7.0.1, FastLED 3.10.3, ArduinoJson 7.4.3, Native-Plattform 1.2.1.
Diese Versionswerte stammen aus den lokal verfügbaren Paketen (Native explizit
festgelegt); der frühere Quellstand verwendete ungebundene Namen. Die neuen
Buildbelege ersetzen keine Herkunftsprüfung früherer geflashter Binärdateien.
Python, Node.js und für Hosttests ein C++17-Compiler werden zusätzlich benötigt.
`requirements-build.txt` enthält auch esptool 4.11.0: Eine frische virtuelle
Umgebung benötigt dessen Python-Abhängigkeiten (unter anderem intelhex) bereits
zum Erzeugen von Bootloader und Firmware, nicht erst beim Flashen.

## Quellen, Assets und Artefakte

- `firmware/esp32_artnet/IMPORT_MANIFEST.json`: SHA-256 der 116 importierten
  Dateien, ursprünglicher ESP-HEAD und Abweichungen des Arbeitsstands. Es ist ein
  Importbeleg; spätere lokale Anpassungen sind in Git und `IMPORT_ADAPTATIONS.json` dokumentiert.
- `reference/teensy-rx32-385d5ed`: unveränderter RX32-Übergabestand mit Manifest,
  Fotos, HEX/ELF und Rohmessungen. `python tools/verify_handover.py` prüft ihn.
- `reference/teensy-apa102-legacy`: separat gesicherter historischer 60-LED-Test
  auf GPIO20/21; ohne Art-Net und ohne Octo-Profil, kein Ersatz für einen Node.
- `firmware/teensy41_artnet/artifacts`: bisherige Release-/Rückfallimages.
  Neue Prüfbuilds überschreiben diese Belege nicht.
- `reports/receiver-analysis-20260919`: gespeicherte Konfiguration/Status und
  lokaler Receiver-Test. Kein Live-Status vom Oktober.
- ESP-WLAN-Zugangsdaten, private Firmware mit einkompilierten Zugangsdaten,
  NVS-/Vollflash-Backups und Paket-Caches werden nicht versioniert. Das
  `WifiSecrets.example.h` und eine künstliche Hosttest-Fixture sind enthalten.

## Prüfung und bekannte Grenzen

Die Ergebnisse des Repository-Abgleichs werden unter
`reports/build-verification-20261001/` dokumentiert. Historische Gerätetests
bleiben unter `reports/octo-*`, `reports/esp-color-order-20260915.md` und
`reports/esp-usb-diagnostic-20260915.md` getrennt erhalten.

Abschluss der lokalen Prüfung am 02.10.2026:

| Prüfung | Ergebnis |
|---|---|
| Teensy Safe, Octo Identify und Octo Web | Build bestanden |
| Teensy acht C++-Hostprogramme und acht Vendor-/Patchtests | Bestanden; Web-ELF mit RX32/ObjectFLED geprüft |
| ESP Flex8 Firmware und SPIFFS-Hintergrund | Build bestanden, ohne private WLAN-Konfiguration |
| ESP Core, Bootschutz, Flex8, Extensionboard und fünf JS-Gruppen | Bestanden; generierter Webheader aktuell |
| Historischer ESP251-Profiltest | Bekannter Defaultpin-Konflikt bestätigt |
| Übrige Firmwareprofile | Dokumentiert, bei diesem Abgleich nicht frisch fertig gebaut |
| Drei README-SVGs | Im Browser und auf mobilen Überlauf geprüft |

Details und Grenzen: [Abschlussbericht](../reports/build-verification-20261001/README.md).

Der ESP251-Profilkonflikt wurde anschließend im Art-Net-RX-Task-Fix durch
Flex8-Vererbung behoben und mit der gespeicherten Pinbelegung getestet.
Offen bleiben insbesondere die physische Octo-Buchsenabnahme und die APA102-
Erweiterung des Teensy. Die controllerweite
Teensy-Sequence-Verarbeitung wird durch das Zusammenführen nicht geändert.
Der ESP-Receiver hat einen eigenen Empfangspfad; Teensy-Frame-Regeln dürfen
nicht ungeprüft auf ihn übertragen werden.
