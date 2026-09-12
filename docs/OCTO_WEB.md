# Teensy-Webinterface

Der Build `teensy41_octo_web_rx32` stellt die Bedienoberfläche direkt über natives
Ethernet auf HTTP-Port 80 bereit. Letzte bekannte DHCP-Adresse: **10.0.0.253**.
Die vorhandene Octo-Adapterplatine bleibt erhalten; LED-RJ45 und Ethernet-RJ45
haben unterschiedliche Aufgaben. Das Boardprofil `PJRC_OCTO_ADAPTER_T41` bindet
OUT1–OUT8 fest an **2,14,7,8,6,20,21,5**. Hersteller/Revision und tatsächliche
Buchsenzuordnung sind weiter elektrisch beziehungsweise optisch zu bestätigen.

## Bedienung

- **Art-Net starten** aktiviert die Ausgabe ausdrücklich. Boot und Neustart
  bleiben gestoppt, ohne LED-Kanalinitialisierung.
- **Stop** wartet laufende DMA, sendet Schwarz und wartet dessen Abschluss plus
  Latchzeit. Die Zustandsanzeige unterscheidet beide Wartephasen von `STOPPED`.
- Einzeltests identifizieren OUT1–OUT8 anhand von Blinkanzahl und wechselndem
  Rot/Grün/Blau; bei jeder Ausgabe leuchten höchstens so viele Pixel wie die
  Ausgangsnummer. Tests enden automatisch nach 30 Sekunden, Helligkeit maximal
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
mehr als eine Sekunde ohne vollständiges Bild stoppen laufendes Art-Net mit Schwarz.
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

## Abnahme am 13.09.2026

Der Release-Build wurde auf Teensy-Seriennummer **7858800**, HalfKay **000BFDD8**
übertragen. Aktuell **10.0.0.253**, Boot `DISARMED`, ohne initialisierte LED-Ausgänge.
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
offen; sämtliche hier automatisierten Gerätetests liefen ohne LED-Ausgabe.
