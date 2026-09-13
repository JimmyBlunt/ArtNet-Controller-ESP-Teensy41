# Octo RX32: Firmware und ESP-Belegung

Aktueller Web-Build mit Orbital Prism und automatischem Art-Net-Start:
siehe [OCTO_WEB.md](OCTO_WEB.md). Die folgenden Angaben betreffen den früheren
USB-Identifikationsbuild, der weiterhin getrennt erhalten bleibt.

## Frühere Abnahme am 13.09.2026

`teensy41_octo_identify_rx32` wurde erfolgreich gebaut, im ELF auf RX32,
modernes Channel/ObjectFLED und fehlende OctoWS2811-Symbole geprüft und auf
den frisch identifizierten Teensy (COM4, Seriennummer7858800, HalfKay000BFDD8)
übertragen. USB-STATUS bestätigt `ESP4031_SPLIT_20260913`, alle Pins/Längen/
Universen und disarmed Boot ohne initialisierte LED-Ausgänge. Nach DHCP ist
der Teensy wieder unter 10.0.0.253 erreichbar.

Ein anschließender echter Ethernet-Empfangstest bei gestoppter Ausgabe bestand:
300/300 vollständige Bilder in etwa10Sekunden, 8728/8728 UDP-Pakete, keine
Queue-Drops und keine Protokollablehnungen. Davon waren28 Pakete ein absichtlich
unvollständiges Vorab-Bild ohne U149: Es wurde korrekt nicht veröffentlicht
und als ein unvollständiges Bild gezählt. Danach8700Pakete =300×29 vollständig.
Ausgabestarts/DMA-Abschlüsse blieben0. Das ist ein realer Empfangsnachweis,
keine LED- oder DMA-Abnahme. Einzel-/Parallel-Leuchttests stehen aus.

Belege: `reports/octo-build.json`, `reports/octo-build-log.txt`,
`reports/octo-flash-20260913.json`, `reports/octo-receive-20260913.json`.
Native Octo-/4031-Grenztests und acht Vendor-/RX32-Patchtests bestanden;
historischer Receiver:15Fälle und10000vollständige Stressframes bestanden.

## Herkunft und Konfiguration

Die Übergabe `Teensy_RX32_385d5ed_Octo_Uebergabe_20260913-053637` wurde unter
`C:/Users/jimmy/Documents/~ projects ~/ArtNet-Controller` gefunden und unter
`reference/teensy-rx32-385d5ed` unverändert gesichert. Alle 80 Nutzdateien passen
zu ihrem SHA-256-Manifest. Das Manifest nennt Firmwarecommit
`385d5edc906d40a6418a9c26329b9ddb8a6316ba`; Git-Objekthistorie ist nicht enthalten.
Referenz-Tag mit geschützten Dateibytes: `baseline/teensy-rx32-385d5ed-exact`.

Auf ausdrücklichen Nutzerwunsch wird die ESP-Ausgangsbelegung übernommen.
Quelle ist `reports/esp-source-4007-20260911.json`: letztes Backup, laut
`Schrank-LED/STATUS.md` später auf dem Ersatz-ESP 10.0.0.248 wiederhergestellt
und bestätigt. Live-Abfragen von .248/.249 antworteten bei Übernahme nicht.
Es handelt sich daher um den letzten belegten Stand, nicht eine aktuelle Live-Kopie.
Danach hat der Nutzer die reale Aufteilung ausdrücklich korrigiert:
OUT6 = 536 LEDs, OUT7 = 512 LEDs. Diese Angabe hat Vorrang vor der alten
1024er-Kette im Backup. Das aktuelle Profil heißt `ESP4031_SPLIT_20260913`.

| Octo OUT | früher ESP-ID | Teensy-Pin | LEDs | Startuniversum | letzte belegte Universe |
|---|---:|---:|---:|---:|---:|
| 1 | 0 | 2 | 203 | 120 | 121 |
| 2 | 1 | 14 | 738 | 122 | 126 |
| 3 | 2 | 7 | 880 | 127 | 132 |
| 4 | 3 | 8 | 810 | 133 | 137 |
| 5 | 4 | 6 | 352 | 139 | 141 |
| 6 | 5, erster Teil | 20 | 536 | 142 | 145 |
| 7 | 5, zweiter Teil | 21 | 512 | 146 | 149 |
| 8 | — | 5 | 0 / deaktiviert | — | — |

4.031 RGB-Pixel, 29 Universen mit bis zu 170 RGB-Pixeln pro Universe,
Universe 138 unbenutzt. Universezahlen sind die unveränderten numerischen
Art-Net-Adressen aus dem ESP-Backup; keine zusätzliche 0-/1-basierte Verschiebung.
Alle sieben Strips: WS2812B, GRB, nicht umgekehrt. Die bestätigte Aufteilung
ist 536+512; zusammen 1.048 und damit 24 mehr als im alten 1024er-Backup.
OUT6 benötigt vier Universen (letztes U145: 26 Pixel / 78 Bytes), OUT7 ebenfalls
vier (letztes U149: 2 Pixel / 6 Bytes). Der Sender muss U149 mitsenden,
sonst wird kein vollständiges Bild veröffentlicht. Keine Kette wird physisch
mit einer anderen verkettet. Gemeinsames FPS-Limit 30; Helligkeit zur Inbetriebnahme **8/255**.
Der Nutzer verlangte gleiche LED-Zahlen/Universen; ein ESP-Helligkeitswert
ist in diesem Backup nicht enthalten und wurde nicht erfunden.

Die logische Zuordnung ist damit definiert. Ob die physischen Kabelnummern
den PJRC-Ausgängen entsprechen, muss weiterhin einzeln beobachtet werden.

## Build und bestehende Rückfallstände

Aktives Projekt: `firmware/teensy41_artnet`.

```powershell
& .\firmware\teensy41_artnet\build.ps1 -NativeTests
```

Der Build verwendet eine eigene Python-Umgebung und kurze, getrennte Caches
unter `C:/codex-build/artnet5-rx32`. `-VenvPath`, `-CacheRoot`, `-Python` und
`-Environments` sind überschreibbar. Native Tests benötigen `g++` oder `-Cxx`.
Standardmäßig entstehen `teensy41_safe` und `teensy41_octo_identify_rx32`.
Keine Uploadaktion im Buildskript.

FastLED-Tag 3.10.4, QNEthernet 0.37.0, Teensy-Plattform 6.0.0 und
Teensyduino 1.62 bleiben gepinnt. Der SHA-geprüfte RX32-Patch gilt ausdrücklich
auch im Octo-Environment. FastLED bleibt auf dem modernen Channel/ObjectFLED-Pfad.
`lib_ignore = OctoWS2811` bleibt gesetzt. Globale Bibliotheken werden nicht verändert.

Die historischen HEX/ELF-Dateien behalten ihre Namen; insbesondere ist
`teensy41_unwired_bench_rx32.hex` **kein Octo-Image**. Das alte Flashwerkzeug
verlangt nun ausdrücklich die Bestätigung eines unverdrahteten GPIO2–8-Aufbaus.
Für das neue Image existiert `tools/flash_octo_verified.py`; es prüft Manifest,
Dateihashes, frisch erkannte USB-Seriennummer und HalfKay-Identität und sendet
nach dem Upload weder ARM noch TEST. Ein Bootstatus mit LED-Initialisierung
wird als Fehler gemeldet.

Die ESP-Quelldateien und deren bestehende uncommittete Änderungen bleiben
unverändert am ursprünglichen Ort. Es wurde kein ESP neu geflasht.

## USB-Kommandos des neuen Builds

115200 Baud, jeweils Zeilenende. Beim Öffnen der USB-Verbindung muss DTR gesetzt
sein, damit Teensy Antworten sendet. 134 Baud ist beim Teensy ein Bootloader-
Trigger und darf nicht für normale Statusabfragen benutzt werden.

| Kommando | Wirkung |
|---|---|
| `STATUS` | Profil, Pins, Längen, Universen, Zustand, Empfangs- und DMA-Zähler |
| `TEST 1` bis `TEST 7` | Nur gewählter Ausgang, 30 Sekunden; übrige konfigurierte Strips vollständig schwarz |
| `TEST ALL` | Alle konfigurierten Ausgänge parallel, 30 Sekunden |
| `TEST ALL 600` | Lokaler Paralleltest bis zehn Minuten |
| `STOP` | Ausgabe stoppen, auf vorige DMA warten, Schwarz übertragen, Abschluss quittieren |
| `ARM` | Art-Net-Ausgabe mit dem korrigierten ESP4031-Profil starten; vollständige Bilder erforderlich |
| `CRASH` | CrashReport nur bei gestoppter Ausgabe |
| `CONFIG WS2812B GRB L1 L2 L3 L4 L5 L6 L7 L8` | Abweichende Testketten konfigurieren, 0 = unbenutzt |

Die Beispielnamen L1…L8 sind durch vollständige angeschlossene LED-Zahlen zu
ersetzen, jeweils 0–1200. Mindestens ein Ausgang muss aktiv sein. Alle sechs
RGB-Farbfolgen sind als gemeinsamer Testparameter möglich. Nach erster
Treiberinitialisierung benötigt eine geänderte Länge/Farbfolge einen Neustart
(`REBOOT_REQUIRED`). Abweichende Testkonfigurationen erlauben kein ARM
(`PROFILE_MISMATCH`); nach Neustart gilt wieder das ESP-Buildprofil.

TEST und ARM setzen gestoppte Ausgabe voraus. Nach STOP die Antwort
`OK STOP BLACK_DMA_COMPLETE` abwarten; ein unmittelbar vor Initialisierung
ausgeführtes STOP meldet `NO_OUTPUT_INITIALIZED`. Bei noch nicht versendeter
STOP-Bestätigung wird ein neuer Start mit `STOP_ACK_PENDING` abgewiesen.

TEST zeigt pro Farbphase n Blinkimpulse an den ersten min(n, LED-Zahl) Pixeln,
n = physischer Ausgang. Die Farbphasen sind Rot, Grün, Blau. Bei Ablauf
der gewählten Dauer (1–600 Sekunden) erfolgt ebenfalls ein Schwarztransfer.
OUT8 ist im aktuellen Profil deaktiviert. Für seine Identifikation muss eine
passende bekannte Testkette angeschlossen und vor erster Initialisierung per
CONFIG eingetragen werden; bestehende Ketten nicht fälschlich als Länge 0 angeben.

Im Art-Net-Modus führen Linkverlust oder etwa eine Sekunde ohne vollständige
Frames zu STOP. Die eingehenden vollständigen Bilder werden gepuffert; das
neueste wartende Bild ersetzt ältere und wird getrennt von Paketverlust gezählt.
Während DMA aktiv ist, werden Ausgabepuffer nicht verändert. Kein Aufholburst.
Der frühere feste Senderfilter 10.0.0.125 wurde nicht auf den neuen Rechner
übertragen. Aktuell wird ein aktiver Art-Net-Sender im LAN vorausgesetzt;
mehrere gleichzeitige Sender werden nicht gemischt oder priorisiert.

## Grenzen der aktuellen Stufe

Der neue Build ist die Hardware-/Empfangsintegration mit festen ESP-Werten.
Webinterface, HTTP-Konfigurationsänderungen und EEPROM-/Dateisystem-Persistenz
sind noch ausstehende Schritte des Gesamtports. Konfiguration im Flash bedeutet
hier kompiliertes Profil, keine abgeschlossene Laufzeit-Konfigurationsverwaltung.

Alle Status-FPS sind interne Empfangs-/Transferzähler. Auch ein erfolgreicher
Build oder USB-Status bestätigt weder optische FPS noch Kabelbelegung.
Die vollständige Hardwareabnahme steht in `OCTO_ADAPTER_ABNAHME.md`.
