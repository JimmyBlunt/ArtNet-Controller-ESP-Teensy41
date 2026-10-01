# Für den nächsten Controller-Build vorgemerkt

Repository-Abgleich 01.10.2026: ESP-Quellen liegen jetzt unter
`firmware/esp32_artnet`; Buildprofile und offene Einschränkungen stehen in
[NODE_BUILDS.md](NODE_BUILDS.md). Autostart, Porttests, animierte Vorschau und
50 % Orbital-Hintergrund sind im Teensy-Webstand bereits umgesetzt. Die unten
vorgemerkte APA102-Erweiterung des Octo bleibt offen. Dieser Abgleich hat keine
Geräte geflasht oder gespeicherte LED-/Universe-Konfigurationen verändert.

## APA102-Erweiterung: Octo später, ESP zuerst

Nutzerentscheidung vom 15.09.2026: Zwei zusätzliche APA102-Ausgänge an der
seitlichen Octo-Pinleiste vorerst nur vormerken; zunächst den ESP verwenden.

- Octo-Kandidaten: DATA/CLOCK 23/22 und 19/18 am Teensy 4.1, per Software-SPI.
  Keine dieser Leitungen ist im aktuellen Octo-Ausgangsprofil belegt.
- Noch nicht implementiert oder am Board getestet. Vor einer Umsetzung
  Pegelanpassung für DATA und CLOCK, Art-Net-Zuordnung und Laufzeitbudget prüfen.
- Für zuverlässige 5-V-APA102-Ansteuerung vier Pegelwandlerkanäle vorsehen,
  beispielsweise einen 74AHCT125 für beide Ausgänge.
- ESP-Expansion-Board laut Nutzerfoto: ESP32S 38P/V4/Goouuu, violette Platine.
  Der 3,3-V/5-V-Jumper wählt die Versorgung der V-Pinreihe; er ist keine
  Pegelanpassung der GPIO-Signale. Das genaue eingesteckte ESP-Modul und seine
  APA102-Pinbelegung sind noch zu klären.

Referenzen: https://www.pjrc.com/store/octo28_adaptor.html und
https://learn.adafruit.com/adafruit-dotstar-leds/power-and-connections

## Automatischer Art-Net-Start

Nutzerwunsch vom 13.09.2026: Der Controller soll nach Einschalten beziehungsweise
Neustart automatisch im **Art-Net-Ausgabemodus** starten. Ein manueller Klick auf
„Art-Net starten“ soll für den normalen Betrieb nicht mehr erforderlich sein.

Für die Umsetzung im nächsten Firmware-Build:

- Gespeicherte Konfiguration laden und die konfigurierten LED-Ausgänge automatisch
  initialisieren/aktivieren. Beim Boot keine Testmuster ausgeben.
- Auf Netzwerk und vollständige Art-Net-Frames warten; Daten automatisch ausgeben,
  sobald diese eintreffen. Fehlendes Ethernet beim Einschalten darf kein späteres
  manuelles Aktivieren nötig machen.
- Auch nach vorübergehendem Netzwerk-/Senderausfall den Empfang und die Ausgabe
  automatisch wieder aufnehmen. Bis zu vollständigen gültigen Frames bleibt die
  Ausgabe schwarz; die bestehenden DMA-/Latch-Schutzbedingungen bleiben erhalten.
- Der ausdrücklich betätigte Web-STOP muss im laufenden Betrieb weiterhin
  wirksam bleiben. Beim nächsten Einschalten/Neustart gilt wieder Autostart.
- Defekte Konfiguration und Initialisierungsfehler weiterhin sichtbar melden;
  keine Ausgabe über ungültige Pins oder aus unvollständigen Frames.
- Boot, verspäteten Netzwerk-Link, Senderstart und Wiederverbindung praktisch
  prüfen; UI und Bedienungsdokumentation auf den neuen Startzustand aktualisieren.

**Status:** im Build `orbital-prism-autostart-20260913` umgesetzt und geflasht.
Boot initialisiert die gespeicherten Ausgänge, sendet Schwarz und wartet im
AN-Modus auf vollständige Daten. Signalverlust beendet den AN-Modus nicht;
manueller Stop bleibt bis zum nächsten Start oder Neustart wirksam.

## Hintergrundgestaltung

Neue Entwurfsrunde: Orbital-Bögen nur auf der linken Seite; zusätzliche feine
Leiterbahnstrukturen aus Violet Circuit, Cyan Glass und Iridescent Etch. Breiter
über die Bildfläche verteilt, nach außen auslaufend, Blau-Lila/Cyan beibehalten.
Hauchfeine Strukturen auch hinter etwa 40–50 Prozent der LED-Flächenbereiche,
mit vereinzelten helleren Pixel-/Leiterbahn-Glanzpunkten. Fünf Bildvarianten
wurden getrennt zur Auswahl erstellt. Gewählt wurde **Orbital Prism**; im Build
als WebP eingebettet, inzwischen 50 % Bildstärke im Build `orbital-port-tests-20260913`.
Die CSS-Position hält den linken Scheitel
des unteren Bogens etwa 2 Pixel innerhalb des linken Bildschirmrands.

Ein PNG zeigt statische Glanzpunkte. Zeitliches Aufglitzern/Schimmern kann bei
der späteren UI-Integration als dezente Animation ergänzt werden.
