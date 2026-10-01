# ESP .251 · einstellbare LED-Farbreihenfolge

Stand: 15.09.2026. Auf `http://10.0.0.251/` per OTA installiert und nach Neustart geprüft.

## Bedienung

**Ausgänge → Farbreihenfolge → Anwenden → Dauerhaft speichern**.
Pro Ausgang: RGB, RBG, GRB, GBR, BRG oder BGR. Gilt für APA102 und WS2812B,
Art-Net und interne LED-Tests. Die Einstellung ist unabhängig von der Pixelrichtung.

## Änderung

ESP-Quellen: `C:/Users/jimmy/Documents/~ projects ~/Schrank-LED`.
Buildprofil: `esp32-wifi-flex8`, passend zum laufenden `flex8-ws2812-apa102`.
UI überschreibt die Farbreihenfolge nicht mehr mit GRB/BGR; die Validierung
akzeptiert alle sechs Werte und lehnt unbekannte Werte ab. `LedColorOrder.h`
ordnet die Farbbytes vor den vorhandenen GRB-/BGR-FastLED-Treibern passend zu.
Die Treiber können dadurch ohne zusätzliche Registrierung weiterverwendet werden.

## Prüfung

- Build erfolgreich: RAM 62.136 / 327.680, Programm 1.221.793 / 1.310.720 Bytes.
- Native Core-, Bootschutz-, Flex8- und Extensionboardtests bestanden.
- Bytefolgen: alle sechs Reihenfolgen für beide LED-Treiber geprüft.
- Desktop- und eingebettete Browserprüfung bestanden, einschließlich individueller
  Auswahl, Auto Layout, Anwenden, Speichern und Neuladen.
- Fünf vorhandene JavaScript-Prüfgruppen bestanden.
- Historischer `test_esp251_profile` scheitert an inzwischen unpassenden Defaultpins
  des Extensionboard-Profils. Mit gesichertem Original-ConfigManager reproduziert;
  unabhängig von dieser Änderung. Das Gerät verwendet Flex8.
- OTA: HTTP 200, Firmwareprüfung erfolgreich; Gerät nach Neustart erreichbar.
- Live: Dropdown im ausgelieferten HTML vorhanden; alle sechs Werte per API übernommen
  und zurückgelesen; XYZ, Zahl und null abgewiesen. Originalkonfiguration wiederhergestellt.
- Vorhandene gespeicherte Konfiguration überlebt OTA: APA102, GPIO18/19,
  256 Pixel, Startuniversum 156, 30 FPS, bisherige Reihenfolge BGR.
  `/api/storage`: saved=true, matches=true. Keine optische LED-Abnahme.

Firmware-SHA256: `ccf544ad8ed6237dd16c0afe243a1ad809cfabf92947af553c5b21d9d577609d`.
Private Firmware, Konfigurationssicherung, Manifest und ausführliche Prüfbelege:
`C:/Temp/esp-recovery-20260915/color-order`.
Vorherige bearbeitete Quellen: `C:/Temp/esp-recovery-20260915/pre-color-order`.
