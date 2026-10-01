# Quellen und Repository-Stand

Stand: **01.10.2026**. Maßgebliche Buildübersicht: [NODE_BUILDS.md](NODE_BUILDS.md).

## Teensy / Octo

Aktive Quellen unter `firmware/teensy41_artnet`, festes gemeinsames Boardprofil
unter `firmware/include/board_profiles/PjrcOctoAdapterT41.h`. Der Webstand trägt
weiterhin `orbital-port-tests-20260913`; beim Zusammenführen wurde keine neue
Firmwarefunktion eingebaut und kein Gerät geflasht. Buildskript, gepinnte
Abhängigkeiten, Vendor-Prüfungen/RX32-Patch, Webgenerator, Assets, native Tests
und vorhandene Release-HEX/ELF sind im Repository.

`reference/teensy-rx32-385d5ed` bleibt der unveränderte, SHA-geprüfte
80-Dateien-Übergabestand; Herkunftscommit laut Manifest
`385d5edc906d40a6418a9c26329b9ddb8a6316ba`. Keine Git-Historie im Snapshot.
Die frühere Anleitung des aktiven Unterprojekts beschrieb fälschlich nur diesen
historischen Aufbau ohne Octo. Sie wurde durch eine aktuelle Buildanleitung ersetzt.

## ESP32

Der buildrelevante Arbeitsstand aus
`C:/Users/jimmy/Documents/~ projects ~/Schrank-LED` ist jetzt unter
`firmware/esp32_artnet` enthalten. Ursprünglicher HEAD:
`9c236b57901406313999995608acdfaa865c4776`. **116 Dateien** wurden bytegenau
importiert, einschließlich 55 Dateien, die vom HEAD abweichen oder dort noch
nicht existieren. Das ist ausdrücklich kein reiner Export dieses Commits.
`IMPORT_MANIFEST.json` enthält Herkunft und SHA-256 jedes Originals.
`IMPORT_ADAPTATIONS.json` dokumentiert anschließende lokale Build-/Doku-Anpassungen.
Das fremde Projekt wurde ausschließlich gelesen.

Enthalten: C++-Quellen/Headers, alle PlatformIO-Profile, Webquellen, generierter
Webheader, Orbital-WebP für SPIFFS, Generatoren, Tests/Stubs, benötigte Mapping-
und Preview-Fixtures sowie vorhandene technische Dokumentation. Die originale
README steht als `README_UPSTREAM.md`, alte Gerätechronik in `STATUS.md`.
Aktuelle Anleitung ist die neue Unterprojekt-README; die alte Chronik ist kein
aktueller Live-Status. Das ESP251-Profilproblem bleibt als bekannter Fehler sichtbar.

Nicht übernommen: private WLAN-Header, ESP-Firmware mit eingebauten Zugangsdaten,
Vollflash/NVS-Backups, virtuelle Umgebungen, Paket-Caches und für den Firmwarebuild
irrelevante Medien-/Senderbibliotheken. Eine ausschließlich künstliche WLAN-Fixture
für Hosttests wurde neu angelegt. Private Zugangsdaten werden bei Bedarf lokal
anhand von `WifiSecrets.example.h` eingerichtet und sind in Git ausgeschlossen.

## Weitere Referenzen und Messbelege

Der alte Ordner `C:/Users/jimmy/Documents/Teensy-ArtNet` enthält keinen Art-Net-
Receiver, sondern einen 60-Pixel-APA102-Test auf GPIO20/21. Seine vier Projektdateien
sind unter `reference/teensy-apa102-legacy` mit SHA-Manifest gesichert. Seine
ungepinnten Abhängigkeiten und seine automatische Testausgabe sind historisch;
der Stand ist kein Octo-Node-Profil und wurde jetzt nicht geflasht.

Die lesende Aufnahme vom 19.09.2026 und die Receiver-Probe liegen unter
`reports/receiver-analysis-20260919`; Erklärung: [ARTNET_RECEIVER.md](ARTNET_RECEIVER.md).
Sie zeigt OUT6=610 statt Default536. Daraus wird keine Gerätekonfiguration geändert.
Bestehende Ethernet-Berichte und ihre Auswertewerkzeuge werden ebenfalls erhalten.
Build-/Hostprüfungen der Zusammenführung: `reports/build-verification-20261001`.

Die ursprünglichen Integrationsnotizen bleiben unter
[history/SOURCE_STATUS_20260913.md](history/SOURCE_STATUS_20260913.md) erhalten.
Dort genannte fehlende Quellen, Pinprofile und Geräteadressen sind historisch.
Auf Nutzerwunsch wird dieser Stand als `main` zum Remote `origin` veröffentlicht:
[JimmyBlunt/ArtNet-Controller-ESP-Teensy41](https://github.com/JimmyBlunt/ArtNet-Controller-ESP-Teensy41).
