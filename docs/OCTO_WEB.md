# Teensy-Webinterface

Der Build `teensy41_octo_web_rx32` stellt die Bedienoberfläche direkt über natives
Ethernet auf HTTP-Port 80 bereit. Letzte bekannte DHCP-Adresse: **10.0.0.253**.
Die vorhandene Octo-Adapterplatine bleibt erhalten; LED-RJ45 und Ethernet-RJ45
haben unterschiedliche Aufgaben. Das Boardprofil `PJRC_OCTO_ADAPTER_T41` bindet
OUT1–OUT8 fest an **2,14,7,8,6,20,21,5**. Hersteller/Revision und tatsächliche
Buchsenzuordnung sind weiter elektrisch beziehungsweise optisch zu bestätigen.

## Bedienung

- Boot und Neustart aktivieren Art-Net **automatisch AN** mit der gespeicherten
  Konfiguration. Ein initiales Schwarzbild wird abgeschlossen; vollständige
  Daten eines später startenden Senders werden ohne manuellen Klick ausgegeben.
- **Art-Net starten** aktiviert die Ausgabe nach einem manuellen Stop erneut.
- **Stop** wartet laufende DMA, sendet Schwarz und wartet dessen Abschluss plus
  Latchzeit. Die Zustandsanzeige unterscheidet beide Wartephasen von `STOPPED`.
  Ein manueller Stop bleibt auch bei eintreffenden Daten wirksam; der nächste
  ausdrückliche Start oder Neustart aktiviert die Ausgabe wieder.
- **Einmal · 20 s** und **Im Loop** starten direkt auf der jeweiligen Portkarte
  ein RGB-Lauflicht über die konfigurierte Kette. **Alle · Einmal / Im Loop**
  testen alle aktivierten Ports parallel. Kein vorheriger Stop ist nötig.
  **Test beenden** und das natürliche Ende eines Einmaldurchlaufs stellen den
  vorherigen Art-Net-Modus wieder her. **Stop · LEDs aus** bleibt dagegen gestoppt.
- **Portnummer blinken** identifiziert nach Stop OUT1–OUT8 anhand von Blinkanzahl und wechselndem
  Rot/Grün/Blau; bei jeder Ausgabe leuchten höchstens so viele Pixel wie die
  Ausgangsnummer. Diese Identifikation endet nach 30 Sekunden, Helligkeit maximal
  8/255. Der Gesamttest umfasst alle aktiv konfigurierten Ausgänge.
- Unter **Ausgänge** werden Länge, Startuniversum, RGB-Farbreihenfolge,
  Pixelrichtung, globale Helligkeit und Ziel-FPS eingestellt. Die GPIOs sind fest.
- **Anwenden · RAM** validiert und übernimmt den Formularentwurf ausschließlich
  bei gestoppter Ausgabe. **Dauerhaft speichern** schreibt ausdrücklich in den
  Konfigurationsspeicher. Ein Schieberegler/Formularfeld löst keinen Flash-Write aus.
- Nach LED-Kanalinitialisierung benötigen strukturelle Änderungen einen
  gespeicherten Neustart. Netzwerkänderungen benötigen ebenfalls einen Neustart.
  Bis dahin sind Start und Tests gesperrt. Ungespeicherte RAM-Änderungen verhindern
  einen Neustart über die API; zuerst speichern oder die Konfiguration zurücksetzen.
- JSON-Export/Import enthält das gesamte Schema 1 einschließlich Netzwerk.
  Import lädt zunächst das Formular. Fremde ESP-GPIO-Profile werden abgewiesen.

## Ausgangsprofil bei leerem Speicher

| Ausgang | GPIO | LEDs | Universen, 0-basiert |
|---|---:|---:|---|
| OUT1 | 2 | 203 | 120–121 |
| OUT2 | 14 | 738 | 122–126 |
| OUT3 | 7 | 880 | 127–132 |
| OUT4 | 8 | 810 | 133–137 |
| OUT5 | 6 | 352 | 139–141 |
| OUT6 | 20 | 536 | 142–145 |
| OUT7 | 21 | 512 | 146–149 |
| OUT8 | 5 | 0, deaktiviert | — |

4031 LEDs, 29 aktive Universen; U138 bleibt frei. Pro Universum werden bis zu
170 RGB-Pixel verwendet. U145 braucht mindestens 78 Bytes, U149 mindestens
6 Bytes. Standard: WS2812B, GRB, vorwärts, 30 Ziel-FPS, Helligkeit 8/255, DHCP.
Maximal 1200 Pixel pro Ausgang und 64 getrennte Universen; keine Überschneidungen.
Die effektive Frameperiode berücksichtigt die längste Kette (30 µs/Pixel plus
300 µs Latch) und begrenzt entsprechend die einstellbaren 1–60 Ziel-FPS.

## Implementierung und Grenzen

Die moderne FastLED-Channel-API/ObjectFLED bleibt aktiv. Keine OctoWS2811-
Bibliothek und kein Legacy-Wrapper. QNEthernet 0.37.0 behält den SHA-geprüften
RX32-Patch (49152 Byte Empfangspuffer, 1024 Byte Deskriptoren). FastLED-Release
3.10.4 meldet in seinen Paketmetadaten 3.10.3. ArduinoJson ist auf 6.21.6 gepinnt;
diese Version enthält die Korrektur für den Zahlenkonvertierungsfehler aus
[den offiziellen Release Notes](https://arduinojson.org/v6/revisions/).

HTTP verarbeitet höchstens zwei Clients mit je 512 Byte pro Schleifendurchlauf.
Header sind auf 1536 Byte, JSON-Body auf 6144 Byte, verschachtelte JSON-Strukturen
auf sechs Ebenen begrenzt. Idle-Timeout: 1500 ms; gesamte Verbindung: 5000 ms.
Kein blockierendes `flush`, `writeFully`, `stop` oder Warten auf Clients.
Antworten verwenden `Connection: close`; Assets sind deterministisch gzip-komprimiert
in Flash eingebettet. UI benötigt weder SD-Karte noch Internet/CDN.

Konfiguration: zwei CRC32-geprüfte, versionierte EEPROM-Slots zu je 124 Byte,
Commit-Marker zuletzt, Generation mit Überlaufvergleich. EEPROM-Adressen 0–247
sind reserviert. Beim gepinnten Teensy-4.1-Core liegen beide Slots in getrennten
Flashsektoren (`sector=(address>>2)%63`); das schützt die ältere Kopie bei der
Sektorbereinigung der neuen. Alle Writes nur nach abgeschlossenem STOP/DMA.
Zwei ungültige nichtleere Kopien laden Defaults und sperren Start/Test bis zur
ausdrücklichen Bestätigung durch Anwenden oder Speichern.

Art-Net empfängt weiter im gestoppten Zustand, emittiert dabei aber keine LED-Daten.
Nur vollständige Frames werden freigegeben. Ein neuer vollständiger Frame ersetzt
einen älteren wartenden. Es gibt keine nachholenden Ausgabebursts. Netzverlust oder
mehr als eine Sekunde ohne vollständiges Bild schalten die Ausgabe einmal auf Schwarz.
Art-Net bleibt AN und wartet auf vollständige neue Daten. Die Wiederkehr des
Senders benötigt keinen Startklick. Die Zustandsanzeige nennt diesen Wartezustand;
`armed=true` und `artnet_waiting=true` beschreiben ihn in der API.
Sequenz 0 kann keine vollständige zeitliche Kohärenz verschiedener Universen garantieren.
Es wird ein aktiver Art-Net-Sender auf dem Netz angenommen.

Diagnose trennt Art-Net-Pakete, vollständige Empfangsframes/FPS, Submit-FPS,
beobachtete DMA-Abschlüsse/FPS, unvollständige, alte und ersetzte Frames sowie
UDP-Queue-Drops und maximale Zeiten. Diese Softwarezähler ersetzen keine
elektrische/optische Messung. `physical_fps_verified` bleibt false.

Nicht enthalten: ESP-OTA, WLAN, APA102, geometrische PC-Vorschau und freie GPIO-
Wahl. Firmware wird per USB aktualisiert. Benutzerverwaltung/TLS sind nicht vorhanden.

## API und reproduzierbare Prüfung

`GET /api/config`, `GET /api/status`; JSON-POST auf `/api/config`, `/api/save`,
`/api/start`, `/api/stop`, `/api/test`, `/api/reboot`. Testbody:
`{"output":1,"seconds":30}`, Ausgabe 0 bedeutet alle aktivierten Ausgänge.
Sonstige Aktionsbodies: `{}`. Erfolg `{ "ok": true }`; Fehler enthalten `error`.

`firmware/teensy41_artnet/build.ps1 -NativeTests` baut Safe/Web und führt
Receiver-, HTTP-, Konfigurations-, Journal-, JSON- und Patchtests aus.
`tools/verify_octo_build.py` prüft den ELF und zeichnet Quell-/Artefakt-SHAs auf.
`tools/flash_octo_verified.py` lädt nur ein geprüftes Octo-Artefakt auf den frisch
identifizierten Teensy; niemals ARM/TEST. `tools/check_octo_web.py` prüft HTTP,
Fehlereingaben, langsame Clients und reale Save/Reboot-Persistenz bei uninitialisierten
LED-Ausgängen; die ursprüngliche Konfiguration wird wiederhergestellt.
`tools/check_octo_receive.py` prüft den 29-Universe-Empfang ebenfalls ohne LED-Ausgabe.

Die unveränderte Vorgängerfirmware mit USB-Konfiguration bleibt als
`teensy41_octo_identify_rx32` und in ihren bisherigen HEX-/ELF-Artefakten erhalten.
Die historische GPIO2–8-Testfirmware darf nicht auf den angeschlossenen Octo-Aufbau.

## Frühere Abnahme am 13.09.2026 (vor Orbital/Autostart)

Der Release-Build wurde auf Teensy-Seriennummer **7858800**, HalfKay **000BFDD8**
übertragen. Damals **10.0.0.253**, Boot `DISARMED`, ohne initialisierte LED-Ausgänge.
`reports/octo-web-build-release.json` und `octo-web-flash-release-20260913.json`
halten Binary-/Quell-SHAs, RX32-Symbole und Geräteidentität fest. Aktuelle Kopien:
`artifacts/octo-web-4031-rx32-20260913.hex` beziehungsweise `.elf`.

- Native HTTP-, Receiver-, Konfigurations-, Journal- und JSON-Prüfungen bestanden;
  außerdem acht Vendor-/Patchtests. Journaltests simulieren 100 unterbrochene
  Schreibabläufe sowie CRC-Fehler und Generationsüberlauf.
- Reale HTTP-Prüfung: HTML/CSS/JS, acht falsche Konfigurationen ohne Teiländerung,
  doppelte Content-Length, übergroßer Body, Chunked-Encoding, JSON-Nachlauf und
  langsamer Client. Letzterer blockiert die zweite Statusabfrage nicht (21 ms).
  Siehe `reports/octo-web-http-release-20260913.json`.
- Reale Speicherung: Helligkeit geändert, gespeichert, neu gestartet und
  zurückgelesen; danach die ursprüngliche Konfiguration wieder gespeichert und
  nach zweitem Neustart geprüft. Sämtliche LED-Kanäle blieben uninitialisiert.
- Empfang unter Weblast: **300/300 vollständige Frames**, **8728/8728 Pakete**,
  keine UDP-Queue-Drops, Rejections oder ungewollte Teilframes. Ein Teilframe ohne
  U149 wurde absichtlich erzeugt und korrekt verworfen. Daneben 94 HTTP-Abfragen,
  maximal 296 ms Antwortzeit, keine HTTP-Fehler; zusätzlicher geöffneter Browser.
  Tatsächliche Senderrate 29,28 FPS bei 30-FPS-Soll auf diesem Windows-Host.
  Beleg: `reports/octo-web-receive-current-20260913.json`.

Frühere `octo-web-receive-*`-Fehlberichte bleiben als Diagnosehistorie erhalten:
Sie werteten ältere periodische USB-Statusmeldungen als aktuelle Antwort aus.
Teensy hat zusätzlich zum Hostpuffer eine 8192-Byte-USB-Sendewarteschlange. Die
folgenden Testberichte und frische HTTP-Abfragen enthalten jeweils bereits die
vollständigen Empfangszähler. Die korrigierte Prüfung verwendet synchrones HTTP
für Vorher-/Nachher-Zähler und einen separat getakteten Sender ohne Nachholbursts.
Die zwischenzeitliche Hypothese über wiederholte UDP-Payloads wurde nicht bestätigt.

Frühe HTTP-Fehlberichte dokumentieren eine korrigierte Fehlerantwort beim Schließen
mit verbleibenden TCP-Daten. Solche Verbindungen nutzen nun `closeOutput()` und
begrenztes Leeren bis Peer-Close/Timeout. Vollständige normale Anfragen geben den
Client sofort frei. Bei einigen Header-only-Proben erreichten bis zum Timeout
null Nutzbytes den Anwendungs-Slot; der Ort dieser Verzögerung ist nicht bestimmt.
Ein vollständig gesendeter übergroßer Body wird mit HTTP 413 beantwortet.

Die physische Abnahme der Adapterbuchsen, LED-Ketten und parallelen Ausgabe bleibt
offen; sämtliche Gerätetests dieser früheren Abnahme liefen ohne LED-Ausgabe.

## Orbital Prism und Autostart – aktueller Build

`orbital-prism-autostart-20260913` wurde auf denselben Teensy übertragen.
Autostart ist AN, mit den unveränderten gespeicherten 4031 LEDs und 29 Universen.
Ein Boot-Schwarzbild wird vollständig einschließlich DMA/Latch abgeschlossen.
Danach wartet der Controller auf vollständige Daten. Senderpause und Linkverlust
behalten den AN-Modus bei; ein manueller Stop hebt ihn bis zum nächsten Start auf.
Beschädigte Konfigurationen und Initialisierungsfehler bleiben Start-Hindernisse.

Orbital Prism liegt als 178766 Byte großes WebP direkt im Firmware-Flash. Alle
vier Web-Assets sind deterministisch gzip-komprimiert. Die Bildstärke beträgt
100 Prozent; Screen-Mischung erhält das Blau-Lila-Farbschema. Der linke Scheitel
des unteren Bogens ist über die CSS-Skalierung etwa 2 Pixel innerhalb des linken
Bildschirmrands verankert. Bild und Glow sind statisch. Konvertierung des Originals:
`python tools/prepare_orbital_asset.py` (Pillow 12.2.0, WebP quality 92/method 6).

Aktuelle Prüfnachweise:

- `reports/octo-web-orbital-build-verified.txt`: Safe-/Web-Build, alle nativen
  Receiver-/Konfigurations-/HTTP-/Journal-/JSON-Tests und acht Patchtests bestanden.
  Zusätzliche Tests prüfen Wartezustand, Link-/Senderverlust, manuelles STOP während
  Schwarz-DMA sowie den ersten über mehrere Schleifendurchläufe aufgebauten Frame.
- `reports/octo-web-orbital-build-verified.json` und
  `reports/octo-web-orbital-flash-verified.json`: Quell-/Binary-SHAs, RX32-Symbole,
  modernes ObjectFLED, Geräteidentität und bestätigter AN-Boot.
- `reports/octo-web-orbital-acceptance-verified.json`: 60/60 vollständige Bilder,
  1740/1740 Pakete, keine Queue-Drops bei gleichzeitigem Hintergrund-Download
  (268 ms, vollständig und SHA-geprüft). Reale Ausgabesubmits/DMA-Abschlüsse mit
  ausschließlich schwarzen RGB-Daten; verspäteter Sender, Timeout-Schwarzbild,
  automatische Wiederaufnahme, fehlendes U149, Stop trotz eintreffender Daten,
  ausdrücklicher Wiederstart und AN-Modus nach API-Neustart bestanden.
  Konfiguration vor/nach der Prüfung identisch. Gerät bleibt AN und wartet auf Daten.
- `reports/octo-web-orbital-layout-verified.txt`: reales Webinterface bei
  1440×900, 1920×1080 und 390×844 geprüft, Bild decodiert, Opazität 1,
  Bogenverankerung rund 2 Pixel, kein horizontaler Überlauf.

Physisches Kabelziehen und die optische LED-/Buchsenabnahme sind damit nicht
nachgewiesen; Linkverlust ist zusätzlich im nativen Zustandstest geprüft.
Release-Kopien: `artifacts/octo-web-orbital-autostart-20260913.hex` und `.elf`;
`reports/octo-web-orbital-release.json` verbindet sie mit den Nachweisen.

Die ersten Orbital-Berichte ohne `verified` dokumentieren den Zwischenstand:
Das zu frühe Leeren eines Teilframes beim Warten auf den ersten Sender wurde
im endgültigen Build korrigiert. Historische Prüfprogramme mit der Voraussetzung
„DISARMED, keine initialisierten Kanäle“ bleiben für die älteren Builds erhalten;
für den aktuellen Build gilt `tools/check_octo_autostart.py`.

## Porttests, animierte Vorschau und 50 % Hintergrund

Build `orbital-port-tests-20260913` ergänzt den aus `Schrank-LED/firmware/src/HardwareTest.cpp`
übernommenen WS2812B-Ablauf: 1 s Schwarz, 6 s Rot, 6 s Grün, 6 s Blau, 1 s Schwarz.
Ein Lauflichtblock enthält vier Pixel pro 32, Zielintervall 50 ms. Der Rotanteil
ist doppelt so hoch wie Grün/Blau; globale Testhelligkeit höchstens 36/255 und
niemals höher als die gespeicherte Einstellung. Ein einmaliger Test endet nach
20 s; Loop läuft bis „Test beenden“ oder globalem Stop. Deaktivierte Ports bleiben aus.

Vor Teststart/Portwechsel, Testende und Rückkehr zu Art-Net werden vorige DMA und
ein eigenes Schwarzbild samt Latch abgeschlossen. Während des Tests aufgebaute
Art-Net-Frames werden beim Übergang verworfen; der folgende vollständige Frame
darf wieder ausgegeben werden. Manueller Stop löscht auch einen vorgemerkten Test.
Wer einen Test aus STOP startet, bleibt nach Testende gestoppt.

Neue API: `POST /api/test-pattern` mit `{"action":"start","output":6}` oder
`{"action":"loop","output":0}`. Physische OUT1–OUT8, 0 = alle aktiven Ports.
`{"action":"stop"}` beendet nur den Test und stellt den vorherigen Betriebsmodus
wieder her. Die ältere `/api/test`-Identifikation bleibt erhalten.

Status ergänzt `test_pattern`, `test_loop`, `test_pending`, `test_phase`,
`test_frame_index`, `test_resume_artnet`. Die Portkarten zeigen eine animierte
Vorschau der ersten bis zu 32 Pixel einschließlich Pixelrichtung, Farbe und Muster.
Sie wird aus Testzeit/Framezähler zwischen Statusabfragen fortgeschrieben; sie ist
keine optische Rückmeldung. Bei veraltetem Status (>2,5 s) wird sie ausgeblendet;
reduzierte Bewegung verzichtet auf das Fortschreiben zwischen Statusmeldungen.

Orbital Prism ist jetzt mit **50 % Deckkraft** eingebettet. Die linke Bogenposition
bleibt erhalten. Die 100-%-Angaben im vorigen Abnahmeabschnitt sind historisch.
Build-/Flash-/Geräte-/Browsernachweise dieser Erweiterung tragen den Präfix
`reports/octo-web-port-tests-*`. RGB-Ausgabe wurde damit funktional geprüft;
die optische Bestätigung der Buchsenbelegung bleibt separat.

Geräteabnahme: alle sieben Portauswahlen, sämtliche RGB-Phasen des Einmaldurchlaufs,
Wiederholung des Gesamttests nach 20 Sekunden, Moduswiederherstellung, manueller
Stop und sieben fehlerhafte Anfragen geprüft. Browserabnahme: einzelne wandernde
4er-Blöcke auf OUT6, sieben gleichzeitige Portvorschauen beim Gesamttest und
Mobilansicht ohne horizontalen Überlauf bei 50 % Deckkraft. Autostart-/Empfangstest
erneut bestanden (60/60 Bilder, 1740/1740 Pakete, keine Queue-Drops). Gespeicherte
Konfiguration unverändert. `octo-web-port-tests-release.json` verknüpft die finalen
Artefakte und Prüfnachweise; zum Abschluss bleibt Art-Net AN ohne laufenden Test.
