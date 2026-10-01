# Quellstand für die Integration

Stand: 13.09.2026.

## Web-Port

`teensy41_octo_web_rx32` ergänzt eingebettete Bedienoberfläche, begrenztes HTTP,
dynamische Ausgangsrouten und CRC-geschützte dauerhafte Konfiguration. Die
USB-Identifikationsfirmware bleibt unverändert als Rückfallstand erhalten.
Bedienung, Prüfwerkzeuge und Grenzen: [OCTO_WEB.md](OCTO_WEB.md).
Aktuelle Build-/Test-/Flashbelege werden getrennt unter `reports/octo-web-*` geführt.

## Aktueller Stand nach Übergabe und Nutzerkorrektur

Die zunächst fehlenden Quellen, Originalberichte und sechs Aufbaufotos sind
inzwischen vollständig vorhanden. `reference/teensy-rx32-385d5ed` bewahrt den
Übergabeordner bytegenau; `tools/verify_handover.py` bestätigt alle 80
Manifestdateien. Herkunft laut Manifest: Firmwarecommit
`385d5edc906d40a6418a9c26329b9ddb8a6316ba`. Der Snapshot enthält keine Git-Historie.

Ein separater Build `teensy41_octo_identify_rx32` integriert feste Octo-Pins,
moderne FastLED-Channels/ObjectFLED, RX32, Einzeltests und Art-Net-Ausgabe.
Auf Nutzerwunsch wurden die ESP-Universen/-Längen übernommen und danach
OUT6/OUT7 auf **536/512** korrigiert: **4031 Pixel, 29 Universen**, bis U149.
Details und Grenzen: [OCTO_RX32_FIRMWARE.md](OCTO_RX32_FIRMWARE.md).
Der Build wurde inzwischen erfolgreich geflasht und per USB zurückgelesen;
ein realer Empfangstest ohne LED-Ausgabe bestätigte 300/300 vollständige
29-Universe-Bilder. Protokolle liegen unter `reports/octo-*`.

Teensy neu identifiziert: COM4, USB VID:PID 16C0:0483, Seriennummer 7858800;
STATUS vor Upload zeigt den alten unverdrahteten RX32-Build, disarmed,
Ethernet-Link aktiv und IP10.0.0.253. Physische Ausgabe wurde daraus nicht abgeleitet.

Die folgenden Abschnitte dokumentieren die anfängliche Suche und die
Integrationserkenntnisse; frühere Aussagen über fehlende Übergabequellen sind
durch diesen aktuellen Abschnitt ersetzt.

## Initialer Stand vor Übergabe

Das aktuelle Repository `C:/Users/jimmy/Documents/ChatGPT/ArtNet-Controller 5`
war leer (nur `.git`, keine Commits, kein Remote). Die Hardwarekorrektur wird
hier auf `codex/teensy-octo-profile` versioniert.

## Gefunden

- ESP-Projekt: `C:/Users/jimmy/Documents/~ projects ~/Schrank-LED`.
  HEAD `9c236b57901406313999995608acdfaa865c4776`. Viele uncommittete Änderungen,
  einschließlich Firmware, Webinterface, Konfiguration, Tests und Boardprofilen.
  HEAD allein beschreibt deshalb nicht den aktuellen funktionsfähigen Stand.
  Quellen wurden nur gelesen, nicht geändert, zurückgesetzt oder kopiert.
- Älterer Teensy-Ordner: `C:/Users/jimmy/Documents/Teensy-ArtNet`.
  Kein Git-Repository; `main.cpp` enthält einen APA102-Test mit GPIO20/21,
  Legacy-`addLeds`, ohne QNEthernet. Das ist **nicht** der beschriebene RX32-Stand.
- Der Nutzertext nennt RX32-Commit `385d5ed` und Messresultate. Der Commit und
  die Originalmessberichte sind bisher nicht lokal gefunden/verifiziert.
  Nachfragen nach Quell-/Fotopfaden ist gestellt.

## Für den nächsten Integrationsschritt benötigt

Pfad/Repository des getesteten Teensy-Commits `385d5ed`, SHA-geprüfter
RX32-Patch samt Tests und die zugehörigen Originalmessberichte. Außerdem
die Fotos des aktuellen Octo-Aufbaus mit lesbarem Platinenaufdruck sowie
bestätigte LED-Typen/-Längen und Verkabelung für reale Ausgabe.

Mit diesen Quellen zunächst unveränderte ESP-/Teensy-Rückfallstände inklusive
uncommitteter Quelldateien, Buildkonfiguration und verfügbarer Binärdateien
nachvollziehbar sichern. Keine Credentials in Git übernehmen. Erst danach
die geprüfte Hardwareanbindung integrieren. Keine vorhandenen fremden
Arbeitsbäume verändern und keine Ersatzimplementierung als getesteten Stand ausgeben.

## Historisches GPIO-Testprofil (kein RJ45-Profil)

| Testkanal | GPIO | LED-Zahl | Startuniversum |
|---|---:|---:|---:|
| 1 | 2 | 203 | 120 |
| 2 | 3 | 738 | 122 |
| 3 | 4 | 880 | 127 |
| 4 | 5 | 810 | 133 |
| 5 | 6 | 352 | 139 |
| 6 | 7 | 680 | 142 |
| 7 | 8 | 442 | 146 |

Werte ausschließlich aus dem Nutzertext; keine bestätigte neue Verdrahtung.
Nicht in das Octo-Profil migrieren, solange die reale Zuordnung unbekannt ist.
Die berichteten 120-/90-Sekunden-Messungen fanden ohne angeschlossene LEDs
statt und sind keine optische Abnahme des Adapters.

## Konkrete Integrationsbefunde im ESP-Arbeitsstand

Read-only geprüft am 13.09.2026; folgende Pfade beziehen sich auf `Schrank-LED`:

- `firmware/src/main.cpp:43`: Unter `LED_PROFILE_FLEX8` wird GPIO 2 als
  Netzwerk-Fehler-LED verwendet. Das kollidiert mit Octo-OUT1. Das Flag daher
  nicht einfach für Teensy aktivieren. Es steuert zugleich die freie
  Universe-Zuordnung; diese Fähigkeit vom Hardwareprofil trennen.
- `firmware/src/ConfigManager.cpp`: Defaults und Validierung auf das neue
  Boardprofil ausrichten. Nicht nur GPIO-Mitgliedschaft, sondern feste
  OUT/GPIO-Paare auch für deaktivierte Ausgänge prüfen.
- `firmware/src/WebApi.cpp`: Aktuelle Ausgabe-IDs sind überwiegend 0-basiert;
  das neue Headerprofil verwendet physische OUT1–OUT8. Beim API-Port
  explizit übersetzen (bestehende ID 0 -> OUT1) oder versioniert migrieren.
  Keine implizite Übergabe der alten ID an `findOutput`.
  `parseOutputs` behandelt fehlende/unbekannte Typen derzeit als APA102.
  Teensy muss solche Werte ablehnen und nur implementierte Typen anbieten.
- `web/app.js`: Unbekannte Profile fallen auf ESP-Pinlisten zurück.
  Backupimport ersetzt den Profilnamen durch den des Zielgeräts.
  Beides für Teensy korrigieren; Profile dürfen nicht durch Umbenennen
  kompatibel erscheinen. Feste Ausgangszeilen statt freier GPIO-Auswahl.
- `firmware/src/HardwareTest.cpp` und `WebApi.cpp`: Es existiert bereits ein
  Test mit `outputId` sowie Gesamtauswahl `-1`. Diese Abläufe wiederverwenden,
  aber Nummerierung, Schwarzframes, STOP und DMA-Bereitschaft anpassen.
- `firmware/src/LedOutputs.cpp`: ESP-spezifische templatebasierte
  FastLED-Registrierung ersetzt nicht den getesteten Teensy-Channel-Treiber.
- `WebApi.cpp`: `WebServer`, `Preferences` und NVS sind ESP-spezifisch,
  obwohl sie teilweise nur unter `ARDUINO` stehen. Teensy benötigt eigene
  begrenzte HTTP-Verarbeitung und persistente Konfiguration.
- `docs/architecture/TEENSY_PORTING_PLAN.md`: Der alte Plan nennt APA102/SPI
  und OctoWS2811-Evaluation. Diese Teile sind durch die aktuelle Anforderung
  überholt. `tools/build-web-ui.js` erzeugt den eingebetteten Header aus `web/`.

Diese Befunde sind dokumentiert, aber noch nicht im ESP-Arbeitsbaum verändert.

## Durchgeführte Prüfung dieses Profils

Host-Kompilation mit `g++ -std=c++11 -Wall -Wextra -Werror -pedantic` bestanden.
Ein lokaler Smoke-Test prüfte ungültige Ausgangsnummern (0, 9, 255, 65535),
die Ablehnung des alten sequenziellen GPIO-Mappings und vertauschter gültiger
Pins, jeweils genau einen akzeptierten GPIO pro Ausgang sowie eindeutige GPIOs.
Das CSV wurde eingelesen: acht Ausgänge, alle `NOT_RUN`, keine behauptete
Beschriftung oder Messung. Unabhängige Agentenprüfung fand keine
Pin-/Buchsengruppenfehler. Kein Teensy-Build, Flash oder Hardwarezugriff erfolgt.
Die vorhandenen RX32-Protokoll-/Patchtests können erst mit deren Quellen laufen.
