# Teensy RX32 / Octo – Übergabe

Firmware exakt aus Git-Commit: 385d5edc906d40a6418a9c26329b9ddb8a6316ba
Messberichte aus Repository-Stand: 26d430464ccaeeddb3d410c30251469b189c0126
Quellrepository: E:\Daten\Dokumente\ChatGPT\LED-Touchdesigner-Controller\PixelblazeMapper

## Inhalt

- firmware/teensy41_artnet: vollständiger versionierter Firmwareordner einschließlich Quellen, Buildkonfiguration, RX32-Patch, Tests, Flashwerkzeug, HEX/ELF und Rückfallständen.
- docs/performance-program-20260912/teensy: Messberichte und Rohbelege, auch frühere fehlgeschlagene Versuche zur Nachvollziehbarkeit.
- docs/performance-program-20260912/td: 90-s-Integrationstest einschließlich Wiederherstellungsbelegen.
- fotos: sechs unveränderte Originalbilder des Aufbaus, der Adapterplatine und des Kabelsteckers.
- MANIFEST.json: Herkunft und SHA-256 jeder Nutzdatei.

Dieses ZIP ist ein Quellsnapshot, kein vollständiger Git-Klon. Nach dem Entpacken kann auf dem anderen Computer ein eigenes Repository angelegt werden. Plattform-/Bibliotheksabhängigkeiten werden anhand der gepinnten Buildkonfiguration installiert; globale Toolchain-Caches sind nicht enthalten. Build- und Flashskripte enthalten teilweise alte lokale Pfade sowie Geräteidentitäten, die vor Nutzung an den Zielcomputer angepasst und geprüft werden müssen.

## Gemessener Stand

FastLED mit modernem ObjectFLED-Channel-/DMA-Pfad und QNEthernet 0.37.0, ohne OctoWS2811-Bibliothek. RX32-Patch in tools/patch_qnethernet.py vergrößert den vorgelagerten Empfangsring im Testbuild von 5 auf 32 Deskriptoren und prüft die Vendorquelle per SHA.
Unabhängiger Sender: 29,983 DMA-Abschlüsse/s über 120 s; alle 114.268 Pakete empfangen.
TouchDesigner: 29,771 DMA-Abschlüsse/s über 90 s.
Keine LEDs waren bei diesen Lasttests angeschlossen. Dies ist keine optische Abnahme. Ausgang im Test auf höchstens 30 FPS begrenzt.
Der historische Legacy-FastLED-Wrapper hatte Fehler; finalen modernen Channel-Pfad erhalten. Frühere Artefakte nicht mit dem RX32-Abschlussstand verwechseln.

## Hardwarekorrektur: Octo-Adapter verwenden

Der Nutzer bestätigt inzwischen einen Teensy 4.1 auf einer Octo-artigen Adapterplatine. Sie soll wegen der Pegelwandlung und zwei LED-RJ45-Buchsen verwendet werden. Natives Ethernet befindet sich auf dem separaten Flachbandmodul. Die LED-RJ45-Buchsen sind keine Netzwerkanschlüsse.

ACHTUNG: Das getestete Image verwendet die Testpins 2–8. Es ist noch NICHT an die Octo-Buchsenbelegung angepasst. Erwartete Standard-PJRC-Ausgangsfolge 1–8: 2,14,7,8,6,20,21,5. Exakte Boardbelegung elektrisch bestätigen, benanntes Boardprofil erstellen und jeden Ausgang einzeln testen. Fotos allein bestätigen keine Leiterbahndurchgängigkeit oder genaue Chipvariante.

Bisheriges siebenkanaliges Profil:
LED-Zahlen 203/738/880/810/352/680/442.
Startuniversen 120/122/127/133/139/142/146.
Die 1.122er-Kette wurde nur im Testprofil als 680+442 abgebildet; physische Teilung nicht bestätigt.
Nutzer hat acht Ausgangskabel mit 1–8 nummeriert. Gezeigter RJ45-Stecker wurde anhand des Fotos als T568A eingeordnet; zweites Kabel separat prüfen.
Standard-Octo bei T568A: 1/5 Grün, 2/6 Blau, 3/7 Orange, 4/8 Braun. Einfarbig DATA, jeweils zugehörige weiß gestreifte Ader GND. LED-Stromversorgung separat. Zuordnung der zwei Buchsen zu 1–4 und 5–8 im Einzeltest bestätigen.
Referenz: https://www.pjrc.com/store/octo28_adaptor.html

## Nächster Entwicklungsauftrag

ESP-Webinterface, Bedienlogik und Konfiguration auf Teensy portieren; getesteten Teensy-Ausgabepfad erhalten. Aktuell kein Webinterface, USB-Kommandos STATUS/ARM/STOP/CRASH, Netzwerk über DHCP. Letzte Identität COM4 / Seriennummer 7858800 / IP 10.0.0.253 ist historisch, vor Zugriff erneut prüfen.
Webserver nichtblockierend integrieren, Einstellungen validieren und dauerhaft speichern. Unterstützte Pins und LED-Typen nicht ungeprüft vom ESP übernehmen. Empfangsrate, DMA-Abschlüsse, Teilbilder, Queue-Drops und ersetzte wartende Bilder getrennt anzeigen. DMA-Abschlüsse nicht als optische FPS beschriften.
Nach Integration Vergleich mit geöffnetem/geschlossenem Webinterface, Einzel- und Mehrkanalausgabe, mindestens zehn Minuten LED-Dauertest sowie STOP/Netzwerkausfall/Neustart-Verhalten prüfen. Alle Änderungen versionieren und Messbelege aufbewahren.
