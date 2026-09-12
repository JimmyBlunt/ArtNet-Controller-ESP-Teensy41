# Octo-Adapter: Belegung und Abnahme

Stand: 13.09.2026. **Vorbereitet; keine realen Ausgangstests durchgeführt.**

## Hardware und Beleglage

Laut Nutzer: Teensy 4.1 auf OctoWS2811-artiger Adapterplatine mit zwei
LED-RJ45-Buchsen, Ethernet über separates Flachbandmodul. Der Adapter bleibt.
Die frühere Vorgabe „keine Octo-Platine“ ist damit aufgehoben;
„keine OctoWS2811-Bibliothek“ gilt weiterhin.

Hersteller, Platinenrevision, Bestückung und Lage der Buchsen am tatsächlichen
Aufbau sind noch nicht belegt. Das lokal gefundene Foto unter
`C:/Users/jimmy/Documents/Teensy-ArtNet/WhatsApp Image 2026-08-02 at 05.22.37.jpeg`
zeigt einen Teensy auf Lochraster mit Flachbandkabel, aber keinen identifizierbaren
Octo-Adapter mit zwei LED-RJ45. Es genügt nicht zur Verifikation des beschriebenen Aufbaus.

Die [PJRC-Pintabelle](https://www.pjrc.com/teensy/td_libs_OctoWS2811.html)
bestätigt die unten angegebene GPIO-Folge auch für Teensy 4.1.
Die [PJRC-Adapterdokumentation](https://www.pjrc.com/store/octo28_adaptor.html)
beschreibt Buchsengruppen, Signal-/Masseadern und den Pegelwandler.
Die Kontaktnummern unten sind aus der T568B-Farbbelegung abgeleitet;
der tatsächliche Adapter und das eingesetzte Kabel müssen damit abgeglichen werden.

## Festes Profil `PJRC_OCTO_ADAPTER_T41`, Revision 1

„Top“ und „Bottom“ beziehen sich auf die PJRC-Darstellung. Erst nach Zuordnung
am realen Aufbau örtliche Bezeichnungen wie „links/rechts“ ergänzen.
Jede Buchse führt vier unabhängige LED-Datenausgänge, jeweils mit Masseader.

| Ausgang | Teensy-GPIO | PJRC-Buchse | Paar | T568B DATA / GND | RJ45-Kontakt DATA / GND |
|---|---:|---|---:|---|---|
| OUT1 | 2 | Top | 1 | Orange / Weiß-Orange | 2 / 1 |
| OUT2 | 14 | Top | 2 | Blau / Weiß-Blau | 4 / 5 |
| OUT3 | 7 | Top | 3 | Grün / Weiß-Grün | 6 / 3 |
| OUT4 | 8 | Top | 4 | Braun / Weiß-Braun | 8 / 7 |
| OUT5 | 6 | Bottom | 1 | Orange / Weiß-Orange | 2 / 1 |
| OUT6 | 20 | Bottom | 2 | Blau / Weiß-Blau | 4 / 5 |
| OUT7 | 21 | Bottom | 3 | Grün / Weiß-Grün | 6 / 3 |
| OUT8 | 5 | Bottom | 4 | Braun / Weiß-Braun | 8 / 7 |

Bei T568A ändern sich Orange/Grün; die Kontaktzuordnung bleibt maßgeblich.
LED-RJ45 führen LED-Signale und sind keine Ethernet-Anschlüsse.

## Integration in den getesteten Firmwarestand

Das Headerprofil enthält nur unveränderliche Zuordnungen und geprüfte
Ausgangssuche. Es initialisiert weder GPIO noch DMA noch Netzwerk.
Die tatsächliche Integration folgt erst mit dem RX32-Quellstand:

- Treiberpins aus dem Profil ableiten. OUT-Nummern bleiben auch bei deaktivierten
  Ausgängen stabil. Keine arithmetische Zuordnung `pin = output + 1`.
- Oberfläche zeigt `OUTn · GPIOp · Buchse/Paar`; GPIO ist nicht frei editierbar.
  Pro Ausgang bleiben LED-Zahl, Startuniversum, Farbfolge und implementierter Typ konfigurierbar.
  APA102 nicht anbieten, solange DATA/CLOCK-Zuordnung und Treiber nicht separat verifiziert sind.
- ESP-Ausgangs-IDs sind bisher überwiegend 0-basiert; physische OUT-Nummern
  dieses Profils beginnen bei 1. API-Identität ausdrücklich übersetzen oder
  versioniert migrieren. ESP-FLEX8 verwendet GPIO2 als Netzwerk-Fehler-LED:
  diese Funktion beim Port entkoppeln, sonst wird OUT1 gestört.
- Import muss Profil und Ausgangsidentität prüfen. Falls `dataPin` mitgespeichert
  wird, muss er zum festen Ausgang passen. Fremde ESP- oder GPIO-Testprofile
  nicht stillschweigend umdeuten. Fehlende Identität erfordert eine explizite Migration.
- LED-Zahlen/Universen des früheren GPIO-Tests nicht automatisch zuweisen.
  Insbesondere sind 680+442 keine bestätigte physische Teilung von 1122 LEDs.
- FastLED 3.10.4 Channel/ObjectFLED, QNEthernet 0.37.0 mit SHA-geprüftem
  RX32-Buildpatch und Teensy-Plattform 6.0.0 / Teensyduino 1.62 erhalten.
- Während DMA aktiv ist, aktive Puffer unverändert lassen. Erst bei Bereitschaft
  nächste Übertragung starten; vollständige wartende Bilder durch neueste ersetzen,
  separat zählen; kein Aufholen durch Bursts.
- Testmuster über denselben ObjectFLED-Pfad ausgeben. Kein zusätzlicher
  GPIO-Bitbang-Test und kein OctoWS2811-Testsketch als Ersatznachweis.
- STOP muss Testabläufe abbrechen und Schwarz sicher nach Treiberbereitschaft
  übertragen. Nur keine neuen Frames zu senden schaltet bereits leuchtende LEDs
  nicht aus. Schwarzabschluss und eigentlichen STOP-Zustand getrennt verfolgen.

## Vorbereitung am realen Aufbau

1. Lesbare Ober-/Unterseitenfotos und Aufdrucke Hersteller/Revision protokollieren.
   Buchsen im Foto eindeutig markieren. Pegelwandler und Durchleitung mit
   Herstellerplan vergleichen; bei Abweichung eigenes Profil statt Änderung
   der Bedeutung des PJRC-Profils verwenden.
2. Kabelbelegung DATA/GND und LED-DIN bestimmen. Kontinuität stromlos prüfen;
   der aktive Pegelwandler ist keine einfache Durchgangsverbindung vom GPIO
   zum RJ45, seine Ein-/Ausgangszuordnung anhand Plan und Signalmessung prüfen.
3. LED-Typ, Versorgung, gemeinsame Masse, Farbfolge und tatsächliche LED-Zahl
   je Ausgang feststellen. Für die Identifikation zunächst kurze bekannte
   Testketten verwenden. Ausgangsprofil ist keine Stromversorgungsfreigabe.
4. Gerät vor Zugriff neu identifizieren (historisch COM4, Seriennummer 7858800,
   DHCP 10.0.0.253). Quellcommit, Build-ID, RX32-Patchprüfung und Boot-/Crashstatus erfassen.

## Einzeltest: OUT1, OUT2, …, OUT8

Geplanter Ablauf; noch keine entsprechenden USB-Kommandos implementiert:

1. Art-Net-Ausgabe stoppen. Auf allen angeschlossenen Testketten einen
   vollständigen Schwarzframe ausgeben und DMA-Abschluss abwarten.
2. Genau einen logischen Ausgang auswählen. Geringe Helligkeit (z. B. 8/255)
   verwenden. Nur diesen Ausgang n-mal langsam aufleuchten lassen, n = OUT-Nummer;
   alle anderen Ausgänge in jedem Frame schwarz halten. Nicht auf
   „nicht aufgerufen“ vertrauen, sonst bleiben alte LED-Werte erhalten.
3. Tatsächlich reagierende Buchse, Paar und Strip erfassen. Es darf nur der
   ausgewählte Ausgang reagieren. Falls nötig jedes Paar mit derselben
   bekannten Testkette prüfen. Bei Abweichung Test stoppen und Zuordnung klären.
4. Rot, Grün, Blau nacheinander prüfen und beobachtete Farbfolge dokumentieren.
5. Schwarz übertragen, Abschluss abwarten; anschließend Kabel/Strip dauerhaft
   mit `OUTn | GPIOp | DATA | GND` beschriften. Nummer erst nach Beobachtung bestätigen.
6. Datum, Foto-/Messreferenz, Beobachtung und Ergebnis in
   `reports/octo-output-identification.csv` eintragen. Alle acht Ausgänge einzeln prüfen,
   einschließlich OUT8; der alte GPIO-Test hatte nur sieben Kanäle.

## Paralleltest und Dauerabnahme

Nach acht bestandenen Einzeltests alle acht Ausgänge mit unterscheidbaren
Mustern über den modernen Channel/ObjectFLED-Pfad gleichzeitig betreiben.
Zum Erkennen der Nummer z. B. n leuchtende Punkte verwenden, wenn jede
Testkette mindestens acht LEDs besitzt. Unbenutzte Pixel schwarz setzen.
Gemeinsame Signalstarts mit Logikanalysator/Oszilloskop prüfen, wenn parallele
Übertragung elektrisch nachgewiesen werden soll; sichtbares Leuchten allein
belegt keine DMA-Gleichzeitigkeit.

Anschließend tatsächliche Ketten mit bestätigten Längen mindestens zehn Minuten
bei 30 Ziel-FPS betreiben. Unabhängigen Sender und TouchDesigner getrennt prüfen,
jeweils mit geschlossenem Webinterface und geöffnetem Webinterface samt
wiederholten Statusabfragen, sobald dieses integriert ist. Erst danach höhere FPS.
Bei 1122 RGB-Pixeln/800 kbit/s beträgt allein die Datenzeit 33,66 ms;
30 vollständige Ausgaben/s sind auf dieser ungeteilten Kette nicht erreichbar.

Zu erfassen: Dauer, gesendete/empfangene Pakete, vollständige RX-Bilder/s,
gestartete/abgeschlossene Ausgaben/s, längster Bildabstand, unvollständige und
veraltete Bilder, ersetzte wartende Bilder, RX-Überläufe, Transferzeiten,
Ethernetstatus, Neustarts und optische Auffälligkeiten. DMA-Zähler nicht als
optisch gemessene FPS bezeichnen. Bestehende Protokoll-/Patchtests ausführen.

STOP, Netzwerkverlust, ausbleibende Daten, Neustart und Wiederverbindung
gesondert prüfen. Auch Testmodus darf weder unerwartet neu starten noch nach
STOP alte Frames wiedergeben. Prüfung mit aktivem DMA einschließen.

## Abnahmestatus

| Prüfung | Status |
|---|---|
| PJRC-Standardbelegung dokumentarisch | BESTÄTIGT |
| Hersteller/Revision der tatsächlichen Platine | OFFEN |
| Reale Buchsen-/Kabelzuordnung | OFFEN |
| RX32-Quellstand und Patch lokal verifiziert | OFFEN |
| Profil in Teensy-Firmware integriert/gebaut | NICHT DURCHGEFÜHRT |
| OUT1–OUT8 optisch einzeln und beschriftet | NICHT DURCHGEFÜHRT |
| Alle acht Ausgänge parallel | NICHT DURCHGEFÜHRT |
| Zehn Minuten mit realen LEDs | NICHT DURCHGEFÜHRT |
| Webinterface-/Fehlerfallvergleich | NICHT DURCHGEFÜHRT |
