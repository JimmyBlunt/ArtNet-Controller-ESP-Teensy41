# Art-Net Controller

**ESP32 · Teensy 4.1 · Lichtdaten bis zum LED-Ausgang**

Firmware, Weboberflächen, Boardprofile, Hintergrundgrafiken und Buildwerkzeuge
für unsere Art-Net-Nodes — mit nachvollziehbaren Quellen und Prüfnachweisen.

Quellen und Build-Dokumentation zusammengeführt am **01.10.2026**.

GitHub: [JimmyBlunt/ArtNet-Controller-ESP-Teensy41](https://github.com/JimmyBlunt/ArtNet-Controller-ESP-Teensy41), Branch `main`.

![Systemübersicht: Art-Net-Quelle, ESP32 über WLAN und Teensy 4.1 über natives Ethernet mit Octo-Adapter](docs/assets/readme/01-system.svg)

| Node | Betriebsbuild | Ausgabe | Konfiguration |
|---|---|---|---|
| **ESP32** | `esp32-wifi-flex8` | WS2812B / APA102, flexible Ports | Web, NVS, App-OTA |
| **Teensy 4.1 / Octo** | `teensy41_octo_web_rx32` | FastLED Channels / ObjectFLED, feste Octo-Pins | Web, EEPROM, USB-Update |

## Vom Paket zur Ausgabe

![Teensy-Empfang: Pakete prüfen, Universes sammeln, vollständigen Frame puffern und bei freiem DMA zeitgesteuert ausgeben](docs/assets/readme/02-frame-pipeline.svg)

Die Frame-Prüfung des Teensy gilt controllerweit für alle aktiven Ports.
Reservierte Lücken und deaktivierte Ports werden nicht erwartet. Der ESP hat
einen eigenen Empfangspfad mit Sequence-Verarbeitung pro Universe.

## Start und Porttests

![Teensy-Autostart, automatische Wiederaufnahme nach Signalverlust und Einzel-/Loop-Porttests](docs/assets/readme/03-run-and-test.svg)

Der Teensy startet bei gültiger Konfiguration mit **Art-Net AN**. Porttests laufen
einmal oder im Loop, einzeln oder für alle aktiven Ports. Die Webvorschau zeigt
Testfarbe und Laufmuster; sie ist keine optische Rückmeldung der LEDs.

## Quellen und Builds

Aus dem Repository-Root unter PowerShell; beide Befehle bauen lokal ohne Upload:

```powershell
./firmware/teensy41_artnet/build.ps1 -NativeTests
./firmware/esp32_artnet/build.ps1 -Environments esp32-wifi-flex8 -FileSystem -NativeTests
```

Python, Node.js und für Hosttests ein C++17-Compiler werden benötigt.
Private WLAN-Zugangsdaten werden lokal anhand der Beispiel-Datei ergänzt.

- [Buildübersicht aller Varianten und Prüfstatus](docs/NODE_BUILDS.md)
- [ESP32: Quellen, Build und SPIFFS-Hintergrund](firmware/esp32_artnet/README.md)
- [Teensy: aktueller Build und Abhängigkeiten](firmware/teensy41_artnet/README_DE.md)
- [Herkunft der Quellen und Importgrenzen](docs/SOURCE_STATUS.md)
- [Art-Net-Empfangspfad und Aufnahme vom 19.09.2026](docs/ARTNET_RECEIVER.md)

## Grafiken und Web-Assets

- [README-Diagramme als eigenständige SVGs](docs/assets/readme), ohne externe Fonts oder Bilddienste.
- [Editierbarer Diagramm-Generator](tools/render_readme_diagrams.py): `python tools/render_readme_diagrams.py`.
- [Hintergrundentwürfe, PNG-Originale und Vergleichsseiten](output/imagegen/artnet-backgrounds).
- Orbital Prism als WebP für [Teensy](firmware/teensy41_artnet/web/orbital-prism.webp) und [ESP](firmware/esp32_artnet/web/assets/orbital-prism.webp), jeweils mit 50 % Deckkraft.

Beim Teensy ist das Bild im Programm eingebettet; beim ESP liegt es in der
separaten SPIFFS-Partition. HTML, CSS, JavaScript und die jeweiligen Generatoren
sind ebenfalls enthalten.

## Stand und Konfiguration

Die gespeicherten Gerätekonfigurationen sind von den kompilierten Defaults zu
unterscheiden. Beim letzten dokumentierten Teensy-Abruf am 19.09.2026 waren
OUT6=610 und OUT7=512 gespeichert; die Defaults bleiben 536/512. Die folgenden
Buildbeschreibungen sind keine neue Live-Abfrage oder optische Hardwareabnahme.

## Teensy / Octo

Hardwarekorrektur vom 13.09.2026: Der vorhandene Octo-Adapter bleibt für
Pegelwandlung und die beiden LED-RJ45-Buchsen erhalten. Natives Ethernet läuft
über das separate Flachbandmodul. Der vorgesehene Ausgabepfad bleibt die moderne
FastLED-Channel-API mit ObjectFLED; keine OctoWS2811-Bibliothek und kein Legacy-Wrapper.

Das Repository enthält den SHA-geprüften RX32-Übergabestand als unveränderte
Referenz und einen eigenen Teensy-Build `teensy41_octo_web_rx32` mit dem
festen Profil `PJRC_OCTO_ADAPTER_T41`, Revision 1. Der Build bietet lokale
Einzel-/Paralleltests und Art-Net-Ausgabe mit der vom Nutzer verlangten
ESP-Belegung. Der Web-Build startet automatisch mit Art-Net **AN**, wartet auf
vollständige Daten und setzt die Ausgabe nach Signalverlust automatisch fort.
Ein manueller Stop gilt bis zum nächsten Start oder Neustart. Orbital Prism ist
mit 50 % Bildstärke und dem unteren Bogen am linken Bildschirmrand eingebettet.
RGB-Lauflichttests lassen sich pro Port oder für alle aktiven Ports einmal bzw.
im Loop starten; animierte Portvorschauen zeigen Farbe und Muster.
Das Webinterface bietet Konfiguration, Start/Stopp, Einzel-/Paralleltests,
Netzwerk, Diagnose und dauerhafte Einstellungen einschließlich JSON-Import/Export.
Letzte bekannte Geräteadresse: [http://10.0.0.253](http://10.0.0.253).

- [Festes C++-Boardprofil](firmware/include/board_profiles/PjrcOctoAdapterT41.h)
- [Belegung, Beschriftung und Einzel-/Paralleltest](docs/OCTO_ADAPTER_ABNAHME.md)
- [Auszufüllendes Ausgangsprotokoll](reports/octo-output-identification.csv)
- [Herkunft und Quellstand](docs/SOURCE_STATUS.md)
- [Bedienung und ESP-Belegung des neuen Builds](docs/OCTO_RX32_FIRMWARE.md)
- [Webinterface, Speichern und technische Grenzen](docs/OCTO_WEB.md)

Pins OUT1–OUT8: **2, 14, 7, 8, 6, 20, 21, 5**.
Das bestätigt die PJRC-Standardbelegung, nicht die tatsächlich verbaute Platine.
Boardmodell/Revision, Buchsenorientierung und reale LED-Ausgabe sind offen.
Das historische Profil mit Pins 2–8 darf nicht als RJ45-Profil geladen werden.
