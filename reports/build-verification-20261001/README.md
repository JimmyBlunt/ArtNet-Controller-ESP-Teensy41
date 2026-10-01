# Prüfung der Quellenzusammenführung

Beginn: 01.10.2026, Abschluss: 02.10.2026 (Australia/Sydney).
Ausschließlich lokale Build-/Host-/Browserprüfungen.
Keine Geräte geflasht, keine Art-Net-Testpakete gesendet und keine gespeicherte
Node-Konfiguration geändert.

## Bestätigt

- ESP-Import: 116 Originaldateien mit Herkunftsmanifest; 55 weichen vom alten
  Quell-HEAD ab oder waren dort unversioniert. Lokale Anpassungen separat erfasst.
  Private WLAN-Werte sind auch aus der historischen STATUS-Datei entfernt.
- RX32-Referenz: alle 80 Manifestdateien unverändert. Historischer APA102-Test:
  vier Dateien mit Prüfsummen. Alle 17 aktiven PlatformIO-Umgebungen dokumentiert.
- Teensy: Safe-Image im ersten Lauf erfolgreich gebaut. Im folgenden Lauf
  `teensy41_octo_identify_rx32` und `teensy41_octo_web_rx32` erfolgreich gebaut;
  acht C++-Hostprogramme sowie drei QNEthernet- und fünf FastLED-Patchtests bestanden.
- Teensy-Web-ELF: RX32 (49152 Byte Empfangspuffer, 1024 Byte Deskriptoren),
  moderner ObjectFLED-Channel-Engine, benanntes Octo-Profil, keine verlinkte
  OctoWS2811-Bibliothek. Siehe `teensy-web-elf.json`.
- ESP-Hosttests: Firmwarekern, Netzwerk-Bootschutz, Flex8 und Extensionboard sowie
  fünf JavaScript-Gruppen und Prüfung des generierten Webheaders bestanden.
  Der zusätzlich ausgeführte historische ESP251-Test scheitert wie zuvor an
  inkompatiblen Defaultpins. Der Fehler bleibt in `esp-host-tests.json` sichtbar.
- README: drei gültige, im Browser decodierte SVGs, Desktop-Ansicht visuell geprüft,
  bei 390 Pixeln kein horizontaler Überlauf; lokale Dokumentationslinks geprüft.
  WebP-Hintergründe für beide Nodes SHA-identisch.

## ESP-Firmware und Dateisystem

**Flex8-Firmware und SPIFFS-Image erfolgreich gebaut.** Nachweis:
`esp-flex8-build-final.txt`; Artefakt- und Quell-Prüfsummen:
`esp-flex8-artifacts.json`. RAM 43772/327680 Bytes, Programm
844421/1310720 Bytes; Firmware-BIN 844784 Bytes, SPIFFS 1441792 Bytes.
Das Dateisystem enthält `/orbital-prism.webp`. Der Build ohne persönliche
WLAN-Konfiguration ist kein Ersatznachweis für einen realen WLAN-/LED-Test.

Der erste frische ESP-Lauf scheiterte beim Bootloader-Erzeugen an fehlendem
`intelhex`. `requirements-build.txt` enthält deshalb jetzt esptool 4.11.0 mit
seinen Python-Abhängigkeiten; dieser Fehler ist keine Firmware- oder Hardwarediagnose.
Im Wiederholungslauf wird derselbe lokal verfügbare PlatformIO-Core unter
`C:/Users/jimmy/.platformio` genutzt (`-CoreDirectory`); Build-/Library-Verzeichnisse
bleiben unter `C:/codex-build/artnet5-esp32` getrennt. Es wurde keine private
`WifiSecrets.h` übernommen. Die Buildartefakte dieses Laufs enthalten daher
keine persönliche WLAN-Konfiguration und wurden nicht auf ein Gerät geladen.

## Umfang und historische Logs

Die zuerst gestarteten breiten Buildmatrizen wurden nach verweigerten bzw.
langwierigen Paket-Downloads beendet und mit den verfügbaren Paketen für die
aktiven Profile fortgesetzt. `esp-build.txt` und `teensy-build.txt` dokumentieren
diese abgebrochenen Anläufe. Daraus wird kein Erfolg für die gesamte Buildmatrix
abgeleitet. `teensy41_hardware` benötigt weiterhin eine geprüfte lokale Pin-Datei;
W5500 ist ein Platzhalter; das ESP251-Profil bleibt fehlerhaft.

FastLED im Teensy-Build wird über das Tag-Archiv **3.10.4** bezogen. Dessen
lokale Paketmetadaten melden **3.10.3**; die `.piopm`-Herkunft zeigt die
konfigurierte 3.10.4-URL, und die vorhandenen Vendor-SHA-Prüfungen bestanden.
Die gespeicherten alten Release-HEX/ELF wurden nicht durch Prüfbuilds ersetzt.

Die ESP-Browsertests und die historische physische Buchsen-/LED-Abnahme wurden
bei dieser Quellenzusammenführung nicht neu ausgeführt. Die neuen Browserbilder
belegen nur die Darstellung der README-Grafiken.
