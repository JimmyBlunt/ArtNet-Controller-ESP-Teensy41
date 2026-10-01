# ESP32 Art-Net-Nodes

Aktueller Repository-Einstieg, Stand 01.10.2026. Import aus dem Arbeitsstand von
Schrank-LED, einschließlich uncommitteter Erweiterungen vom September.
[Alle Buildprofile und ihr Status](../../docs/NODE_BUILDS.md),
[Importmanifest](IMPORT_MANIFEST.json), [Anpassungen](IMPORT_ADAPTATIONS.json).
`README_UPSTREAM.md` und `STATUS.md` bewahren die frühere Projektchronik.

## Bauen

Im Repository-Root unter PowerShell:

```powershell
./firmware/esp32_artnet/build.ps1 -Environments esp32-wifi-flex8 -FileSystem -NativeTests
```

Voraussetzungen: Python mit venv/pip, Node.js für den Webgenerator, für Hosttests
C++17 (`g++`). Node 24.13.0 ist lokal vorhanden. Buildskript installiert
PlatformIO 6.1.19, pyserial 3.5 und esptool 4.11.0 samt dessen Python-Abhängigkeiten
in `.toolchain`; isolierter kurzer Paket-/Buildpfad
ist `C:/codex-build/artnet5-esp32` (über `-CacheRoot` änderbar).
Optional erlaubt `-CoreDirectory <vorhandener-PlatformIO-Core>` die Nutzung bereits
installierter Pakete; Build- und Bibliotheksordner bleiben unter `CacheRoot` getrennt.
Espressif32 7.0.1, FastLED 3.10.3 und ArduinoJson 7.4.3 sind nun in der INI fixiert.
Die übrigen Paketauflösungen erscheinen im Buildlog. Kein Upload im Buildskript.

Ergebnisse: `C:/codex-build/artnet5-esp32/build/<Profil>/firmware.bin`,
`firmware.elf`, Bootloader/Partitionstabelle und bei `-FileSystem` `spiffs.bin`.
`-FileSystem` nur für Controllerprofile, nicht für `native` oder Power-Diagnose.
Ein Standardbuild ohne private WLAN-Datei ist kompilierbar, verbindet sich aber
nicht mit WLAN. Für einen persönlichen Gerätebuild `firmware/include/WifiSecrets.example.h`
lokal nach `WifiSecrets.h` kopieren und ausfüllen. Diese Datei und mit ihr erzeugte
private Binärdateien nicht in Git aufnehmen.

HTML/CSS/JS stammen aus `web/`; `tools/pio-build-web.py` ruft automatisch
`node tools/build-web-ui.js` auf. `firmware/include/WebUi.generated.h` wird daraus
regeneriert. Das Orbital-WebP liegt getrennt in `web/assets` und wird als SPIFFS-
Partition ausgeliefert. App-OTA ersetzt dieses Dateisystem nicht. Fehlendes Bild
führt zum CSS-Hintergrund, nicht zum Ausfall des Controllers.
Details: [ESP_BACKGROUND.md](docs/ESP_BACKGROUND.md).

## Profile und Konfiguration

`esp32-wifi-flex8` ist der zuletzt dokumentierte Betriebsbuild des ESP `.251`
(siehe [Farbreihenfolge-Bericht](../../reports/esp-color-order-20260915.md)).
Es startet ohne gültige gespeicherte Konfiguration mit GPIO32, 256 WS2812B, U0.
Bis zu acht Ausgänge, im aktuellen Validator höchstens 1200 LEDs je Ausgang;
zusätzliche Gesamt-/Konfigurationsgrenzen stehen in `ConfigManager.cpp`.
NVS-Werte sind unabhängig von den Defaults und werden beim Boot wieder geladen.
Farbreihenfolge lässt sich pro Port ohne Neuregistrierung des Treibers einstellen.

`esp32-wifi-extensionboard` verwendet im aktuellen Code:
WS-Pins 25,26,27,14,19,18,5,17; APA102 DATA/CLOCK 18/5 oder 26/27;
Default GPIO25 mit 256 LEDs. Das ist die Softwarebelegung, keine hier neu bestätigte
physische Platinenabnahme. Die früher beschriebene 30P-Belegung ist überholt.

`esp32-wifi-esp251` erbt dieses eingeschränkte Profil, hat aber noch Defaults
GPIO32 und APA102 18/19. Deshalb scheitert sein vorhandener Profiltest.
Es ist weder der Flex8-Betriebsbuild noch als Ersatz dafür freigegeben.
`esp32-w5500` enthält weiterhin nur einen Netzwerk-Platzhalter.
RMII und allgemeine Testprofile sind ebenfalls nicht automatisch Hardwarefreigaben.

## Hosttests

Aus diesem Verzeichnis:

```powershell
python tools/test_host.py
python tools/test_host.py --include-historical
node tools/build-web-ui.js --check
```

Der erste Lauf prüft Firmwarekern, Bootschutz, Flex8, Extensionboard und fünf
JavaScript-Gruppen, ohne Netzwerkgeräte zu kontaktieren. Ergebnisse stehen in
`build/host-test-results.json`. Der zweite Lauf nimmt den bekannten ESP251-Fehler
hinzu und meldet ihn mit einem Fehlerexitcode; er wird nicht als Erfolg kaschiert.

Die beiden bestehenden Browsertests benötigen zusätzlich Playwright samt Chromium.
Einmal im Repository-Root `npm ci` und `npx playwright install chromium` ausführen;
danach aus diesem ESP-Unterverzeichnis:

```powershell
node tests/test_web_ui_playwright.js
node tests/test_esp_web_menu_playwright.js
```

Die Tests verwenden lokale API-Fixtures, keine echten Controller.
Die alte Makefile bleibt als Herkunftsdatei erhalten; ihre Processing-Pfade und
Flashanleitungen in historischen Dokumenten können rechner-/gerätespezifisch sein.
Für Firmwarebuild und Hosttests gelten die obigen portablen Einstiegspunkte.
