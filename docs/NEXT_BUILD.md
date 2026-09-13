# Für den nächsten Controller-Build vorgemerkt

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

**Status:** vorgemerkt, noch nicht implementiert oder geflasht. Die aktuell
installierte Firmware startet weiterhin mit gestoppter Ausgabe.

## Hintergrundgestaltung

Neue Entwurfsrunde: Orbital-Bögen nur auf der linken Seite; zusätzliche feine
Leiterbahnstrukturen aus Violet Circuit, Cyan Glass und Iridescent Etch. Breiter
über die Bildfläche verteilt, nach außen auslaufend, Blau-Lila/Cyan beibehalten.
Hauchfeine Strukturen auch hinter etwa 40–50 Prozent der LED-Flächenbereiche,
mit vereinzelten helleren Pixel-/Leiterbahn-Glanzpunkten. Fünf Bildvarianten
werden getrennt zur Auswahl erstellt. Erst den ausgewählten Hintergrund später
für den Controller-Build komprimieren und einbetten.

Ein PNG zeigt statische Glanzpunkte. Zeitliches Aufglitzern/Schimmern kann bei
der späteren UI-Integration als dezente Animation ergänzt werden.
