# STATUS

Stand: 2026-09-10 Australia/Sydney

## Alter ESP .251: USB-Flash mit LED-Profil und WLAN-Updater, 2026-09-10

- COM6 aktiv identifiziert: ESP32-D0WD rev1.0, MAC `e8:68:e7:0d:38:a4`.
  Stimmt mit dem urspruenglichen .251-Flashprotokoll ueberein; nicht das zuletzt
  dokumentierte Ersatzboard. Vollstaendiges 4-MB-Backup vor dem Flash angelegt.
- Neues Profil `esp32-wifi-esp251` erbt das aktuelle Extensionboard-Profil:
  WS2812B 285 LEDs GPIO32, APA102 1024 LEDs GPIO18/19, 4 MHz, 30 FPS,
  1309 Pixel, 9 Universes, Output-Startuniverses 0 und 2.
- OTA-Firmwareupload unter `/update`, erreichbar ueber System im gemeinsamen
  Webmenue. Vollstaendiger Upload und Update-Bibliothekspruefung vor Neustart,
  Token pro Boot gegen blinde Cross-Origin-Uploads; NVS bleibt erhalten.
  Details und lokale Netzwerkgrenzen in `docs/FIRMWARE_UPDATE.md`.
- Build erfolgreich: RAM 61768 Bytes (18.9%), Flash 1165917 Bytes (89.0%).
  USB-Upload erfolgreich, alle geschriebenen Bereiche hashverifiziert.
  Bootlog bestaetigt beide gewuenschten Ausgaenge und Hardwareinitialisierung.
- Bestanden: Firmware-Core, Netzwerk-Bootschutz, flexible und Extensionboard-
  Profile, neuer ESP251-Test mit Art-Net-Pixelgrenzen, Embedded-Webmenue-Browsertest.
- Blocker: Sowohl alter als auch neuer Build melden beim WLAN-Start Brownout.
  Neuer Build bleibt danach stabil in `safeMode=yes`, IP 0.0.0.0, 1309 Pixel.
  Ursache noch offen; physische Kabel-/USB-Port-/Boardversorgungspruefung noetig.
  OTA auf Hardware, WLAN-Art-Net und optische LED-Ausgabe nicht verifiziert.
  Keine Testanimation gestartet; serieller Monitor geschlossen.

## Extensionboard-Profil auf Ersatz-ESP, COM6, 2026-09-08

- Defektes Modul ersetzt. Neuer ESP32-D0WD-V3 Revision 3.1 mit MAC
  `d4:e9:f4:b4:b5:b0` erfolgreich mit `esp32-wifi-extensionboard` geflasht; alle
  Flash-Bereiche per Hash verifiziert.
- WLAN stabil mit `safeMode=no`; aktuelle DHCP-Adresse `http://10.0.0.249/`, Ping 4/4 und
  Web/API HTTP 200. Leerer NVS-Start nutzt den vorgesehenen Standardausgang
  WS2812B an GPIO32 mit 256 LEDs, Start-Universe 0 und 30 FPS. Weitere Ausgänge
  richtet der Nutzer selbst im Webmenü ein.
- Eigenes Hardwareprofil `extensionboard-6ws-3apa`: WS2812B an P14, P27, P26,
  P25, P33 und P32; APA102-Paare P16/P17, P18/P19 und P21/P22. P34/P35 werden
  im Menü als reine Eingänge erklärt und serverseitig abgewiesen. Boot-Pins sowie
  RX/TX bleiben reserviert; P4/P23 sind Reserve.
- `make test` einschließlich neuem Extensionboard-Profiltest und beiden
  Playwright-Varianten bestanden. Geflashte UI enthält Profil, P34/P35-Hinweis und
  die drei APA-Paare; API meldet `hardwareProfile=extensionboard-6ws-3apa`.

## Flexibles 8-Ausgang-Profil auf COM6 2026-09-08

- Neues Profil `esp32-wifi-flex8` auf ESP32-D0WD-V3, MAC-Endung `b5:cc`,
  COM6 erfolgreich geflasht; alle Flash-Bereiche per Hash verifiziert.
- Webmenü unter `http://10.0.0.249/` bietet `+ Ausgang hinzufügen`, Entfernen,
  WS2812B-Pinauswahl und APA102-Data/Clock-Paare. Maximal acht Ausgänge, 1024 LEDs
  je Ausgang und 8192 LEDs insgesamt; GPIO-Doppelbelegungen werden abgewiesen.
- Sichere WS-Pins des gezeigten Erweiterungsboards: 13, 14, 16, 17, 18, 19, 21,
  22, 23, 25, 26, 27, 32 und 33. Eingangs-, UART- und Boot-Strapping-Pins werden
  nicht angeboten.
- APA102-Paare: 18/19, 23/18, 13/14, 25/26, 32/33, 16/17 und 21/22.
- Startzustand absichtlich nur ein WS2812B-Ausgang: GPIO32, 256 LEDs. Die weiteren
  Ausgänge soll der Nutzer selbst über das Menü einrichten und testen.
- Nutzerkonfiguration D32 + D27 mit je 256 WS2812B wurde anschließend angewendet und
  dauerhaft gespeichert. Nach Flash/Neustart wurde sie aus NVS wiederhergestellt;
  der Bootlog bestätigt beide FastLED-Hardwaretreiber und `Configured outputs: 2`.
- Das flexible Profil lädt gespeicherte Konfigurationen nun auch beim Boot. Während
  eines nicht angewendeten Browser-Entwurfs sind Testknöpfe gesperrt und Ausgänge als
  `Entwurf · noch nicht aktiv` markiert.
- Loop über 26 Sekunden geprüft: Er blieb über den 20-s-Zyklus hinaus aktiv und begann
  wieder bei Rot. Die UI zeigt dabei `Loop läuft`; ein Klick auf `Ausgang testen`
  startet weiterhin bewusst einen einmaligen Test und ersetzt den vorherigen Loop.
- Freie Art-Net-Zuordnung ergänzt: Jeder Ausgang hat im Webmenü ein eigenes Feld
  `Art-Net Start-Universe` und beginnt dort bei Kanal 1. Volle Universes verwenden
  170 RGB-Pixel / 510 Kanäle; 511-512 bleiben frei. Sparse Universe-Nummern werden
  ohne Pixelpuffer für die Lücken empfangen. Auto Layout vergibt nicht überlappende
  Universe-Bereiche. Build, Core-, Flexprofil- und Browsertests bestanden; Flash auf
  COM6 per Hash verifiziert. Gespeicherte D32/D27/D13-Konfiguration geladen; API meldet
  fünf aktuell eindeutig belegte Universes.
- Browser-Testanzemeldung zeigt jetzt live Testfarbe, bewegten Laufblock, Output/GPIO,
  Loopzustand und steigende LED-Ausgaben. Einmal, Loop, Blackout und Stop wurden gegen
  den echten Controller geprüft. Der Testzyklus dauert 20 Sekunden: Schwarz, Rot, Grün,
  Blau und abschließendes Schwarz.
- Vollständiger echter Browser-Klicktest: 14 Funktionsgruppen bestanden, darunter
  Navigation, Reconnect, Ausgangstest, Hinzufügen/Entfernen, Auto Layout,
  Anwenden/Speichern/Neu laden, Preview, Diagnose und Controller-Neustart. Die
  gespeicherte Einzel-Ausgang-Konfiguration blieb dabei unverändert.
- Neuer Bootschutz: Nur automatische Panic-, Watchdog- und Brownout-Neustarts halten
  das WLAN zunächst im Sicherheitsmodus. Ein Druck auf EN, Strom aus/an oder ein
  absichtlicher Software-Neustart erlaubt automatisch einen neuen WLAN-Versuch;
  `wifi-retry` über USB ist nur noch eine zusätzliche Rückfallmöglichkeit.
- WLAN-Fehleranzeige: GPIO2 gibt zwei kurze Impulse je zwei Sekunden aus, solange
  keine WLAN-Verbindung besteht. Die feste rote PWR_LED des Erweiterungsboards ist
  nicht softwaresteuerbar; die Anzeige benötigt die GPIO2-LED des ESP-Moduls oder
  eine externe Status-LED an GPIO2.
- Das erste 256er-Testprofil wurde anschließend auf 1024 LEDs je Ausgang / 8192 gesamt
  erweitert. Der gemischte WS2812-/APA102-Betrieb nutzt FastLEDs ESP32-RMT-/SPI-Treiber;
  I2S wurde verworfen, weil dieser Treiber einen einheitlichen Clockless-Chipsatz verlangt.
- Finaler 1024/8192-Build und Flash auf COM6 erfolgreich: RAM 61.888 / 327.680 Bytes
  (18,9 %), Flash 1.155.049 / 1.310.720 Bytes (88,1 %), Hash verifiziert. LED-Puffer
  werden nur fuer tatsaechlich angelegte Pinoptionen dynamisch reserviert. Nach Neustart
  WLAN stabil, safeMode=no, Webmenue bestaetigt die neuen Grenzwerte.
- Verifikation des ersten Builds: PlatformIO-Build erfolgreich (RAM 23,8 %, Flash 88,1 %), alle Core-,
  Bootschutz-, Flexprofil- und Browser-Tests bestanden. Nach Flash `safeMode=no`,
  22 Sekunden stabiler serieller Status; Webmenü und API antworten HTTP 200.

## Aktueller Stand: Controller-Refresh 2026-09-07

### Weiteres Board COM3: Flash, Boot und WLAN erfolgreich

Nach Schliessen des seriellen Monitors erfolgreich dieselbe aktuelle Firmware
`esp32-wifi-ws2812-apa102-1679` auf COM3 geflasht: SUCCESS, Flash-Hash verifiziert,
MAC `d4:e9:f4:84:b5:cc`, rev3.1. RAM 60872 (18.6%), Flash 1130509 (86.3%).
Bewusster Reset danach: beide Ausgaenge 512/1167 korrekt initialisiert, WLAN verbindet,
safeMode=no, IP `10.0.0.249`. 25 Statuszeilen ohne weiteren Reset/USB-Abbruch beobachtet.
GET `/`, `/api/status`, `/api/config`, `/api/test-pattern`: HTTP 200; Web-UI 31727 Bytes.
Nachbartabelle bestaetigt D4-E9-F4-84-B5-CC. Animation aus, Monitor geschlossen.
COM6 / 10.0.0.248 wurde nicht veraendert. Kein optischer LED-/Art-Net-/NVS-Speichertest
auf COM3 in diesem Flash-Schritt. Die Vorgaben 512 WS2812B GPIO23, 1167 APA102
GPIO18 Data / GPIO19 Clock, 30 FPS sind per API bestaetigt.

Vorheriger blockierter Upload-Versuch:

Nutzer hat zusaetzlich COM3 angegeben. Aktiv erkannt: ESP32-D0WD-V3 rev3.1,
MAC `d4:e9:f4:84:b5:cc`. COM6/e0-Board bleibt unangetastet. COM3 direkt am PC
(Position 1-1), laut Nutzer in einem Erweiterungsboard mit eigener Stromversorgung.
Build des gleichen 1679-Profils bestanden; Upload konnte COM3 nicht oeffnen:
PermissionError 13 / WinError 5, Zugriff verweigert. Auch separate Portprobe verweigert.
Arduino-IDE-Prozess `serial-monitor` vorhanden (kein sicherer Handle-Nachweis fuer COM3).
Bei diesem ersten Versuch kein Flash-/Erase-Befehl auf COM3 ausgefuehrt; anschliessend
nach Freigabe des Ports erfolgreich wie oben beschrieben.

### Aktuell: Versorgter USB-Hub, WLAN/API/Neustart erfolgreich

Gleicher ESP und gleiche Firmware, nur USB-Pfad geaendert: CP210x COM6 jetzt
Position 1-4.4, laut Nutzer Hub mit eigenem Netzteil. EIN `wifi-retry` erfolgreich:
WiFi.mode kehrt zurueck, WLAN verbindet, Dienste starten; safeMode=no.
IP `10.0.0.248`, Nachbartabelle bestaetigt MAC `e0:5a:1b:6c:9c:e8`.
Dieser kontrollierte Vergleich spricht deutlich fuer den vorherigen USB-/Versorgungsweg;
Spannungseinbruch/Kabel/Host-Port sind nicht einzeln elektrisch nachgewiesen.

- GET `/`, `/api/status`, `/api/config`, `/api/storage`, `/api/test-pattern`: HTTP 200.
  ESP liefert gemeinsame Weboberflaeche (31727 Bytes), 1679 Pixel / 10 Universen.
- Echte NVS-Konfigurationspersistenz: targetFps kurz auf 29 gesetzt, explizit
  gespeichert, per POST `/api/reboot` neu gestartet und 29 per GET bestaetigt.
  Danach vollstaendige Ausgangskonfiguration wiederhergestellt (512/1167, 30 FPS),
  gespeichert, erneut per API gestartet und 30 bestaetigt. `/api/storage`:
  saved=true, matches=true. Zwei normale WLAN-Neustarts erfolgreich.
- Art-Net-Smoke ausschliesslich Schwarz: 30 Pakete / 3 Frames / 10 Universen,
  Controller-Delta packets=30, framesComplete=3, framesIncomplete=0,
  outputFrames=3, sequenceErrors=0. Keine optische LED-Funktion behauptet.
- Kein Flash und keine Aenderung am Quellcode in diesem Hub-Vergleich. Normale Firmware
  bleibt aktiv, automatische LED-Animation aus, Ausgaenge zuletzt schwarz.
- Abschliessender serieller Monitor: 26 Statuszeilen in ca. 25s, safeMode=no,
  keine Reset-/Brownout-/Panic-/Fehlerzeile, kein COM-Abbruch. Monitor geschlossen.
- Restpunkt: GET `/api/storage` kann bei noch nie angelegtem Konfigurations-Namespace
  weiterhin eine nicht-fatale Preferences-NOT_FOUND-Zeile loggen; nach obigem ersten
  Speichern nicht mehr aufgetreten. Der Boot-Default-Fall selbst ist bereits korrigiert.

Weiter offen: optische LEDs, Langzeit-/Lastmessungen und sonstige historische
Prompt-Restpunkte. Die folgenden Abschnitte beschreiben den VORHERIGEN Fehlerzustand.

### Vorher: WLAN-Start isoliert, USB-Recovery verifiziert

Board `e0:5a:1b:6c:9c:e8`, ESP32-D0WD-V3 rev3.0, COM6:
- Manueller Download-Modus erfolgreich; Chip/MAC ausgelesen, 30/30 Portproben
  in 15 Sekunden vorhanden.
- Diagnosefirmware ohne LED-Treiber/Webserver gestartet: fortlaufende Uptime,
  radio=0, Heap 310780 Bytes. Der einzelne serielle Befehl `w` fuehrt zu
  `[power-diag] BEFORE WiFi.mode(WIFI_STA)` und unmittelbar zum USB-Abbruch.
  `AFTER WiFi.mode` wurde nicht erreicht; Router-Verbindung wurde nicht gestartet.
  Nach Reset wieder stabil radio=0. LED-Code/UI sind fuer diesen Ausfall nicht notwendig.
- Normale 1679-Firmware mit persistentem WLAN-Bootschutz auf COM6 geflasht,
  Hash verifiziert. NVS-Marker `led-boot/wifi-pending` wird vor Funkstart committed.
  Nach unterbrochenem Start bleibt beim naechsten Boot das WLAN aus. Kein automatischer
  Retry-Loop; `wifi-retry` + Zeilenumbruch erlaubt einen expliziten neuen Versuch.
- Echter Boot: beide Ausgaenge 512/1167 initialisiert; safeMode=yes, IP=0.0.0.0,
  Heap 267036, 25 fortlaufende Statuszeilen. Fehlende gespeicherte Konfiguration
  wird als normaler Default-Fall geloggt statt Preferences-NOT_FOUND.
- Expliziter WLAN-Retry reproduzierte USB-Ausfall. Nach Wiederanmeldung:
  40 Sekunden / 40 Statuszeilen safeMode=yes, KEIN Leseabbruch. Der erste zu fruehe
  Wiederverbindungsversuch hatte noch einen USB-Abbruch waehrend der Wiederanmeldung.
- Netzwerkdienste werden erst bei bestehender Verbindung gestartet; Safe Mode startet
  weder Art-Net-Sockets noch Webserver. Normaler USB-Flash bleibt moeglich.
- Finaler Reflash (gleiche MAC): SUCCESS, Hash verifiziert, RAM 60872/327680 (18.6%),
  Flash 1130509/1310720 (86.3%). Anschliessend bewusster Reset und 45s Monitor:
  genau ein Boot-Header, 45 Statuszeilen safeMode=yes, keine Panic-/Brownout-Meldung,
  kein USB-Leseabbruch. Monitor geschlossen; normale Firmware aktiv, NICHT Diagnose.
- Neu getestete Bootschutz-Regressionen: Reset waehrend Funkstart, persistenter Schutz,
  expliziter Retry, exakter/begrenzter Befehlsparser, Speicher-/Schreibfehler,
  WiFi.mode-Fehler, normaler Verbindungs-Timeout. Native Core- und fuenf JS-Suites bestehen.
- Finale Build-Matrix bestanden: esp32-wifi, esp32-rmii-ethernet, esp32-w5500 und
  esp32-wifi-ws2812-apa102-1679. Gemeinsame UI-Generierung und git diff --check bestanden.
  Bestehende I2S-Deprecation-Warnungen stammen aus dem Arduino-Framework; kein neuer
  Buildfehler. Ethernet-Hardware und Browser-Runtime wurden in diesem Schritt nicht getestet.

NICHT als vollstaendig repariert markieren: WLAN-Funkstart scheitert weiterhin auch
ohne Anwendungstreiber. Versorgung/USB-Pfad bzw. Funkinitialisierung sind noch zu
unterscheiden; kein elektrischer Nachweis eines Spannungseinbruchs auf diesem Board.
Naechster externer Vergleich: bekannte stabile USB-Versorgung (z.B. aktiv versorgter
Daten-USB-Hub oder anderer Rechner), danach EIN `wifi-retry`. Keine gleichzeitige
unabgesicherte Versorgung ueber USB und GPIO. Brownout-Schutz bleibt unveraendert.
Keine IP/API/LED-Optik oder NVS-Konfigurationspersistenz ueber die API als bestanden
markiert. Keine Aenderung am alten LAN-ESP `10.0.0.251`.

### Vorherige Diagnose (chronologisch, durch obigen Stand ergaenzt)

Neues Vergleichsboard (2026-09-07): Nach USB-Portwechsel COM6 / Position 1-1 aktiv
als ESP32-D0WD-V3 revision v3.0 erkannt, MAC `e0:5a:1b:6c:9c:e8`. Normale Firmware
`esp32-wifi-ws2812-apa102-1679` erfolgreich hochgeladen, Flash-Hash verifiziert.
Nach automatischem Reset ist der COM6-Zugriff instabil: zeitweise keine Portliste,
teils CP210x als vorhanden gemeldet, Oeffnen wiederholt FileNotFoundError (WinError 2).
Auch Oeffnen mit vorab deaktiviertem DTR/RTS schlug fehl. Noch kein Boot-Log dieses
Boards, daher insbesondere kein nachgewiesener Brownout auf dem Vergleichsboard.
Gezielte Suche nach seiner MAC im erreichbaren Bereich 10.0.0.1..254 brachte keine
IP-Zuordnung. Flash abgeschlossen; Boot-/WLAN-/Persistenztest weiter offen.
Erneuter Recheck nach Nutzerbestaetigung: COM6-Oeffnen weiterhin WinError 2.
PnP meldet CP210x als OK (Treiber 11.4.0.393, USB-Position 1-1), aber
Win32_SerialPort liefert keinen Port und die serielle Registry-Zuordnung verschwindet
wieder. Damit ist der Windows-USB/COM-Zugriff aktuell nicht nutzbar; die Ursache ist
nicht abschliessend bestimmt. Kein erneuter Flash, kein Zugriff auf den alten LAN-ESP.
Folgende Brownout-Protokolle betreffen ausschliesslich das vorherige Board d4:e9:… .

Isolierter Brownout-Test (2026-09-07, Nutzer: direkt am PC, anderes Kabel, ohne LEDs):
`esp32-power-diagnostic` gebaut und auf COM6 / MAC `d4:e9:f4:84:b5:cc` geflasht;
Flash-Hash verifiziert. Diese Firmware bindet keine LED-Treiber/Web-API ein und
wuerde acht Sekunden Baseline vor dem ersten expliziten WLAN-Aufruf protokollieren.
25s Monitor: wiederholte Brownout-Meldungen, kein erster `[power-diag]`-Marker.
Separater ungefilterter 3s Boot-Log bestaetigt Neustarts direkt nach dem Bootloader.
Damit sind die LED-Treiber und unsere Controller-Oberflaeche nicht notwendig, um
den Fehler auszuloesen. Ursache innerhalb Versorgung/Board noch nicht elektrisch
gemessen; Brownout-Schutz unveraendert aktiv. Vergleich mit anderem ESP am selben
USB-Anschluss/Kabel ist der naechste sinnvolle physische Test.
Anschliessend normale `esp32-wifi-ws2812-apa102-1679`-Firmware erfolgreich erneut
gebaut und auf demselben Board wiederhergestellt (Hash verifiziert; 86.1% Flash).
Nachkontrolle: 8 Brownout-Meldungen in 4s. Diagnose-Firmware ist nicht mehr aktiv;
serieller Monitor wurde geschlossen. Keine NVS-Konfiguration im Test gespeichert.

USB-Recheck nach weiteren Kabelwechseln (2026-09-07): COM6 wieder sichtbar als
CP210x (USB-Position 1-2). Laut Nutzer keine LEDs angeschlossen. 18s Boot-Monitor
zeigt weiterhin wiederholte Brownout-Resets vor einer IP-Meldung. Damit ist die
USB-Datenverbindung wieder hergestellt, stabiler Firmwarebetrieb aber weiterhin
nicht erreicht. `10.0.0.251` wurde zuvor per Nachbartabelle als alter ESP
`e8:68:e7:0d:38:a4` identifiziert; diese IP ist kein Nachweis fuer das neue Board.

Dieser Abschnitt ist aktueller als die historische Prompt-Tabelle und die folgenden
Arbeitsprotokolle. Umsetzung und lokale Verifikation sind erfolgt; die echte
Netzwerk-/Persistenz-/LED-Verifikation ist aktuell durch fehlenden COM-Zugriff und
fehlende IP-Zuordnung blockiert; Brownouts wurden nur am vorherigen Board belegt.

- Gemeinsame Weboberflaeche aus `web/index.html`, `web/styles.css`, `web/app.js`.
  PlatformIO erzeugt `firmware/include/WebUi.generated.h` automatisch.
- Blau/Violett-Glasdesign, fuenf Bereiche, echte Controller-Ausgaenge statt Demo-Werten,
  konsistente Offline-Anzeige, Eingaben bleiben beim Polling erhalten.
- Stop sendet einmal Schwarz und gibt Art-Net frei; Blackout haelt Schwarz bis zur
  Freigabe. Einzel-Ausgangstests und gemeinsame Tests sind verfuegbar.
- FastLED nutzt konfigurierte LED-Anzahlen; Routing ohne neue Slice-Vektoren je Frame.
- Art-Net: feste 170-Pixel-Universe-Slots, Reihenfolgepruefung pro Universe,
  Timeout/Recovery, begrenztes Burst-Receiving und Ziel-FPS-Begrenzung.
  Kein ArtSync und kein Multi-Sender-Merging; Sammlung bleibt best-effort.
- PC-Sender-Sequenzen auf 1..255 ohne doppelte 1 beim Umlauf korrigiert.
- Konfiguration kann angewendet und explizit in NVS gespeichert werden. Boot-Restore
  fuer das aktuelle 1679-Profil implementiert. Echte Persistenz ueber Reboot noch offen.
- Mapping bleibt explizit PC-seitig; POST /api/mappings liefert 501 statt eines
  irrefuehrenden Erfolgs ohne Auswirkung auf die Ausgabe.
- Frisch bestanden: nativer C++-Build und Core-Regressionen, fuenf JS-Suites,
  Desktop- und Embedded-Browsertests inklusive Apply/Save/Fehler/Offline/Mobile,
  Shared-UI-Generierungscheck, git diff --check.
- Finaler Build `esp32-wifi-ws2812-apa102-1679`: bestanden;
  RAM 60,840 / 327,680 Bytes (18.6%), Flash 1,128,153 / 1,310,720 Bytes (86.1%).
- COM6 nach Freigabe eindeutig erkannt: ESP32-D0WD-V3 revision v3.1,
  MAC `d4:e9:f4:84:b5:cc`.
- Upload auf COM6 erfolgreich, Flash-Hash verifiziert, Hard Reset ausgefuehrt.
- Monitor 115200: Profil und 512 WS2812 GPIO23 + 1167 APA102 GPIO18/19 bestaetigt.
  Danach wiederholt `Brownout detector was triggered`, Neustartschleife, keine IP
  im beobachteten Boot. Test-Autostart ist deaktiviert. Versorgung muss zuerst
  stabilisiert werden; keine Hardwarefunktion als optisch bestanden markiert.
- Naechster Schritt: ESP nur ueber verlaessliches USB ohne LED-Verbindungen testen;
  danach Boot/IP pruefen, NVS-Speichern/Reboot/Restore mit Rueckstellung der
  Ausgangskonfiguration testen, Stop/Blackout und Art-Net am echten Board verifizieren.
- Details: `docs/CONTROLLER_REFRESH.md`. Browser-Screenshots in
  `output/playwright/*fixture.png` zeigen ausdruecklich simulierte Testdaten.

## Regeln

- Funktionen werden nur als fertig markiert, wenn sie gebaut und getestet wurden.
- Hardwarefunktionen ohne angeschlossene Hardware werden als geplant oder simuliert markiert.
- `Xorcery5 SLIM.epe` war vor Beginn bereits geaendert und wurde nicht bearbeitet.

## Fortschritt nach Prompt

| Prompt | Status | Build | Tests | Notizen |
|---|---|---|---|---|
| 01 Master | Abgeschlossen | Bestanden | Bestanden | Struktur, Architektur, nativer Mock-Build. |
| 02 ESP32 Firmware | Teilweise, fuer naechsten Prompt freigegeben | Native bestanden; ESP32 WiFi/RMII/W5500 bestanden; Upload esp32-wifi auf COM6 bestanden | Core bestanden; Boot/Monitor bestanden; WiFi/API/Art-Net RX/Web-CORS bestanden; Hardware-LED offen | Projektlokales PlatformIO eingerichtet. Pflichtmodule kompiliert und im ESP32-Loop verdrahtet. ESP32-D0WD-V3 auf COM6 geflasht, WiFi verbunden, API, CORS und Art-Net-Empfang ueber WiFi getestet. |
| 03 Pixelblaze Mapping | Abgeschlossen fuer Offline-Kompatibilitaet | Bestanden | Bestanden | Neutraler Validator, Beispiele, Pixelblaze-Array-Import/Export, Pixelblaze-Generator-Preservation und MariMapper-CSV-Import/Export. |
| 04 Web UI | Teilweise, fuer naechsten Prompt freigegeben | Web-Tests bestanden; ESP32 WiFi/RMII/W5500 bestanden; esp32-wifi auf COM6 geflasht | Playwright erweitert bestanden; echte ESP32-GET/POST/API und CORS bestanden; dauerhafte Reboot-Persistenz offen | UI hat Seiten, Connect, Output-Editoren, Save fuer Outputs/Mappings, Mapping-Presets, Preview-Reconnect und Statusdiagnose. Firmware validiert und speichert Config/Outputs/Mappings im RAM. |
| 05 Processing Visualizer | Teilweise, fuer naechsten Prompt freigegeben | Processing-Core-Tests bestanden; Processing-3-CLI-Build bestanden; volle lokale Suite bestanden; ESP32 WiFi/RMII/W5500 bestanden | Preview, Direct Art-Net, Mapping, Stats, Playback-Core, Hot-Reload-Core und ESP32 Controller-Preview bestanden | Sketch hat UDP-Receiver fuer Preview 6455 und Art-Net 6454. ESP32 sendet Preview an Processing nach `POST /api/preview/target`. Processing-4-CLI, Sketch-UI fuer Playback/Hot-Reload und Performance-Langlauf offen. |
| 06 Test Tool | Abgeschlossen fuer Simulation | Bestanden | Report erzeugt | Art-Net Generator schreibt JSON/CSV; Senden optional. |
| 07 Automated Tests | Abgeschlossen lokal | Bestanden | Bestanden | Firmware, Mapping, Web, Processing-Protokoll, Playwright. HIL separat offen. |
| 08 Performance/Hardware | Teilweise, fuer Abschluss mit Hardware offen | Native bestanden; ESP32 WiFi/RMII/W5500 bestanden | Matrix, Tooltests, Simulationsreports und kurzer ESP32-WiFi-API-Benchmark bestanden | Theoretische Matrix erweitert, Hardware-Benchmark-Tool hinzugefuegt, ESP32-WiFi API-Messung dokumentiert; echte LED-Signal-, RMII-, W5500- und Teensy-Messungen offen. |
| 09 Integration/Roadmap | Abgeschlossen dokumentiert | Bestanden | Doku geprueft | README und Architektur-/Hardware-/Teensy-Doku erstellt. |

## Momentan Offen

- APA102/WS2812B Hardware-in-the-loop Tests durchfuehren.
- Langzeittests und echte Benchmarkwerte ergaenzen.
- Nutzerfreigabe: Nicht auf Hardware-LED-Test warten, sondern nach belegtem Web/API-Stand mit dem naechsten offenen Prompt fortfahren. Hardware-LED bleibt trotzdem nicht als bestanden markiert.
- Prompt 04 offen: dauerhafte Flash-/Dateisystem-Persistenz ueber Reboot fuer Config/Outputs/Mappings; `POST /api/reboot` wurde nicht destruktiv ausgeloest; echte UDP-Live-Preview mit Receiver nicht verifiziert.
- Prompt 05 offen: Processing 4 GUI ist installiert, aber keine Processing-4-CLI `processing-java.exe` gefunden; `.pde` baut und laeuft mit Processing 3.5.4 CLI; Playback-/Hot-Reload-Bedienung im Sketch und Runtime-Performance-Langlauf bleiben offen.
- Prompt 08 offen: echte APA102/WS2812B-Signaltests, RMII/W5500/Teensy-Laufzeitbenchmarks, CPU-Load-Messung und Langzeittests fehlen weiterhin; ESP32-WiFi-API-Kurzbenchmark ist gemessen, aber kein LED-Hardwaretest.
- Pixelblaze-/MariMapper-Kompatibilitaet ist lokal per Tooling getestet; optische Layout-Richtigkeit importierter Fremdmaps auf der echten Installation bleibt ein separater Hardware-/Mappingtest.

## Aktueller Zusatzstand 2026-08-18

- TouchDesigner-Artnet-Mapper-Projekt ausgewertet:
  - Relevante Logik: Pixelblaze-Koordinatenarray, Pixelblaze-JS-Generator als nicht auszufuehrende Quelle, MariMapper-CSV mit `index,x,y,z` bzw. `index,tx,ty,tz`.
  - Uebernommene Grenze: Luecken in MariMapper-Indizes werden nicht komprimiert, sondern als `[0,0,0]`-Platzhalter erhalten.
- Implementiert in diesem Projekt:
  - `tools/mapping-utils.js`: Pixelblaze-Array-Import mit Kommentar-/Trailing-Comma-Toleranz, Pixelblaze-Generator-Erkennung ohne Ausfuehrung, MariMapper-CSV-Import, Pixelblaze-/MariMapper-Export.
  - `tools/mapping-convert.js`: `--format auto|neutral|pixelblaze|marimapper` und `--export neutral|pixelblaze|marimapper`.
  - `tests/test_mapping_schema.js`: Tests fuer Pixelblaze-Import/Export, Generator-Preservation, MariMapper-CSV, fehlende Indizes, Metadaten und Fehlerfaelle.
  - `docs/mapping/PIXELBLAZE_MARIMAPPER_COMPATIBILITY.md`: Workflow und ESP32-Performancegrenze dokumentiert.
- Nicht implementiert auf dem ESP32:
  - Kein Runtime-Parsen grosser Pixelblaze-/MariMapper-Dateien.
  - Keine JavaScript-Ausfuehrung/Pixelblaze-Script-Engine.
  - Begruendung: grosse Mapping-Dateien und Scriptlogik wuerden Heap und Art-Net/LED-Timing riskieren; Mapping-Konvertierung bleibt PC-seitig.
- Verifikation:
  - `node tests/test_mapping_schema.js`: bestanden.
  - `node tools/mapping-convert.js --format marimapper ...`: Smoke-Test bestanden.
  - `node tools/mapping-convert.js --format neutral --export pixelblaze ...`: Smoke-Test bestanden.
  - Vollstaendiges `make check`: bestanden, inklusive Playwright-UI-Tests und PlatformIO-Build `esp32-wifi-ws2812-apa102-1679`.

## Verifikation

Bestanden:

```sh
NODE_PATH="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules" make test benchmark NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"
```

Prompt-04-Zusatzverifikation:

```sh
node --check web/app.js
node tests/test_web_calculations.js
NODE_PATH="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules" /mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe tests/test_web_ui_playwright.js
NODE_PATH="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules" make test NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"
.\.venv-platformio\Scripts\platformio.exe run -e esp32-wifi
.\.venv-platformio\Scripts\platformio.exe run -e esp32-rmii-ethernet -e esp32-w5500
.\.venv-platformio\Scripts\platformio.exe run -e esp32-wifi --target upload --upload-port COM6
curl http://10.0.0.246/api/status
curl http://10.0.0.246/api/config
curl http://10.0.0.246/api/outputs
curl http://10.0.0.246/api/mappings
curl http://10.0.0.246/api/artnet/stats
curl http://10.0.0.246/api/performance
curl -X POST http://10.0.0.246/api/test-pattern
curl -X POST http://10.0.0.246/api/config
curl -X POST http://10.0.0.246/api/outputs
curl -X POST http://10.0.0.246/api/mappings
curl -X OPTIONS -D - http://10.0.0.246/api/reboot
curl -H "Content-Type: application/json" -X POST --data @/tmp/default-config.json http://10.0.0.246/api/config
```

Prompt-02-Zusatzverifikation:

```powershell
make build/test_firmware_core
.\build\test_firmware_core.exe
.\.venv-platformio\Scripts\platformio.exe run -e esp32-wifi -e esp32-rmii-ethernet -e esp32-w5500
.\.venv-platformio\Scripts\platformio.exe device list
.\.venv-platformio\Scripts\platformio.exe run -e esp32-wifi --target upload --upload-port COM6
Invoke-WebRequest -UseBasicParsing -Uri 'http://10.0.0.246/api/status'
Invoke-WebRequest -UseBasicParsing -Uri 'http://10.0.0.246/api/config'
Invoke-WebRequest -UseBasicParsing -Uri 'http://10.0.0.246/api/outputs'
Invoke-WebRequest -UseBasicParsing -Uri 'http://10.0.0.246/api/performance'
Invoke-WebRequest -UseBasicParsing -Uri 'http://10.0.0.246/api/artnet/stats'
Invoke-WebRequest -UseBasicParsing -Method POST -Uri 'http://10.0.0.246/api/test-pattern'
curl -D - http://10.0.0.246/api/status
curl -X OPTIONS -D - http://10.0.0.246/api/status
```

Prompt-05-Zusatzverifikation:

```sh
node tests/test_processing_protocol.js
node tests/test_processing_visualizer.js
NODE_PATH="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules" make test NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"
make processing-build
which processing-java
"C:\Program Files (x86)\processing-3.5.4\processing-java.exe" --sketch="C:\Users\jimmy\Documents\~ pROJECTs ~\Schrank-LED\processing\ArtNetMixedLedVisualizer" --output="C:\Users\jimmy\Documents\~ pROJECTs ~\Schrank-LED\build\processing-visualizer-p3" --force --build
"C:\Program Files (x86)\processing-3.5.4\processing-java.exe" --sketch="C:\Users\jimmy\Documents\~ pROJECTs ~\Schrank-LED\processing\ArtNetMixedLedVisualizer" --output="C:\Users\jimmy\Documents\~ pROJECTs ~\Schrank-LED\build\processing-visualizer-p3-run" --force --run
curl -H "Content-Type: application/json" -X POST --data "{\"host\":\"10.0.0.173\",\"port\":6455}" http://10.0.0.246/api/preview/target
```

Prompt-08-Zusatzverifikation:

```sh
NODE_PATH="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules" make test benchmark NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"
.\.venv-platformio\Scripts\platformio.exe run -e esp32-wifi -e esp32-rmii-ethernet -e esp32-w5500
NODE_PATH="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules" "/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe" tools/hardware-benchmark.js --host 10.0.0.246 --scenario mixed_4500_30 --duration 3 --report reports/hardware-benchmark-esp32-wifi-mixed-4500-30fps-3s
```

Erkannt:

- `COM6`: Silicon Labs CP210x USB to UART Bridge, `VID_10C4&PID_EA60`, plausibel fuer ESP32-Devboard.
- `COM12`: USB Serial Port, `VID_04D8&PID_00DF`, weniger typisch fuer ESP32.

Nicht als fertig markiert:

- APA102-Testausgabe auf echten LEDs.
- WS2812B-Testausgabe auf echten LEDs.
- Preview-Streaming zu einem echten Preview-Empfaenger, weil kein Preview-Ziel konfiguriert/getestet wurde.

Hardware-Ergebnis 2026-08-03:

- Upload: `esp32-wifi` auf `COM6` erfolgreich.
- Board: ESP32-D0WD-V3 rev 3.0, MAC `08:b6:1f:37:d8:20`.
- Erster Monitorlauf zeigte Boot und Firmware-Start, aber dauerhafte `WiFiUdp.endPacket(): could not send data: 118`, weil Preview ohne Netzwerkverbindung gesendet wurde.
- Fix: Preview-Frames werden nur noch bei `network.connected()` gesendet.
- Zweiter Monitorlauf nach Reflash: Boot erfolgreich, Firmware meldet Profil `ESP32 WiFi fallback`, 3 APA102-Outputs und 6 WS2812B-Outputs konfiguriert, `Configured outputs: 9`, `IP: 0.0.0.0`, stabile Statuszeilen ohne UDP-Fehler.
- Monitor-Log: `reports/esp32-monitor-com6-after-preview-fix.log`.

WiFi/API/Art-Net-Ergebnis 2026-08-03:

- Lokale Secret-Datei: `firmware/include/WifiSecrets.h`, gitignored; Vorlage: `firmware/include/WifiSecrets.example.h`.
- WiFi verbunden, Monitor meldete `IP: 10.0.0.246`.
- Monitor-Log: `reports/esp32-monitor-com6-wifi-preview-disabled.log`.
- HTTP API ueber echtes ESP32-WiFi bestanden:
  - `GET /api/status`
  - `GET /api/config`
  - `GET /api/outputs`
  - `GET /api/performance`
  - `GET /api/artnet/stats`
  - `POST /api/test-pattern` mit HTTP 202
- Art-Net RX ueber WiFi bestanden: 27 ArtDMX-Universen an `10.0.0.246:6454` gesendet; danach meldete `/api/status` `packets:27`, `framesComplete:1`, `framesIncomplete:0`.
- Preview-Default-Broadcast verursachte nach WiFi-Verbindung `WiFiUdp.endPacket(): could not send data: 12`; behoben durch deaktiviertes Senden bis ein Preview-Ziel explizit gesetzt wird.

Webinterface-Verbindung 2026-08-03:

- Web-UI lokal erweitert: Controller-IP-Feld, Connect-Button, Test-Pattern-Button und Statusanzeige.
- Firmware-Webserver erweitert: CORS-Header fuer `GET`, `POST`, `OPTIONS`; `OPTIONS /api/status` liefert HTTP 204.
- Build nach CORS/UI-Aenderung: `esp32-wifi` erfolgreich, RAM 15.7 Prozent, Flash 79.4 Prozent.
- Upload nach CORS/UI-Aenderung: `esp32-wifi` auf `COM6` erfolgreich.
- Flash-Ziel verifiziert: esptool erkannte auf `COM6` `ESP32-D0WD-V3 rev 3.0`, MAC `08:b6:1f:37:d8:20`. `COM12` ist weiterhin ein separates USB-Serial-Geraet.
- API nach Reflash erreichbar: `GET http://10.0.0.246/api/status` HTTP 200 mit `Access-Control-Allow-Origin: *`.
- CORS-Preflight verifiziert: `OPTIONS http://10.0.0.246/api/status` HTTP 204 mit `Access-Control-Allow-Methods: GET,POST,OPTIONS`.

Prompt-04-Web-UI-Ergebnis 2026-08-03:

- UI erweitert um Seiten fuer Dashboard, Art-Net, Outputs, APA102, WS2812B, Mapping, Preview, Performance, Logs und System.
- Output Planner bleibt tabellarisch und zeigt Typ, Pixelzahl, Startpixel, Pins, geschaetzte Framezeit, theoretische FPS, Universe-Bereich, Datenrate und Warnungen.
- APA102-/WS2812B-Editoren koennen Output-Werte lokal aendern; Playwright verifiziert, dass Aenderungen sichtbar in die Tabelle durchlaufen.
- Output-Speichern aus der UI ist per Playwright gegen gemockte API verifiziert.
- Mapping-Ansicht hat lokale Presets fuer Linear, Matrix, Zigzag und Ring; Playwright verifiziert das Laden eines Zigzag-Mappings.
- Mapping-Speichern aus der UI ist per Playwright gegen gemockte API verifiziert.
- Preview-Ansicht hat Rate-Limit-Eingabe, Reconnect und Pixel-Hover; Playwright verifiziert Reconnect gegen gemockte API.
- Firmware-Webserver:
  - `GET /api/status`: HTTP 200 auf ESP32-WiFi verifiziert.
  - `GET /api/config`: HTTP 200 auf ESP32-WiFi verifiziert.
  - `GET /api/outputs`: HTTP 200 auf ESP32-WiFi verifiziert.
  - `GET /api/mappings`: HTTP 200 auf ESP32-WiFi verifiziert, liefert aktuell leere Liste.
  - `GET /api/artnet/stats`: HTTP 200 auf ESP32-WiFi verifiziert.
  - `GET /api/performance`: HTTP 200 auf ESP32-WiFi verifiziert.
  - `POST /api/test-pattern`: HTTP 202 auf ESP32-WiFi verifiziert.
  - `POST /api/config`: HTTP 200 auf ESP32-WiFi verifiziert; `targetFps` wurde im RAM geaendert und per `GET /api/config` zurueckgelesen.
  - `POST /api/outputs`: HTTP 200 auf ESP32-WiFi verifiziert; Test-Outputs wurden im RAM geaendert und per `GET /api/outputs` zurueckgelesen.
  - `POST /api/mappings`: HTTP 200 auf ESP32-WiFi verifiziert; Test-Mapping `bench-line` wurde im RAM gespeichert und per `GET /api/mappings` zurueckgelesen.
  - Ungueltiges JSON fuer `POST /api/config`: HTTP 400 auf ESP32-WiFi verifiziert.
  - `OPTIONS /api/reboot`: HTTP 204 auf ESP32-WiFi verifiziert; `POST /api/reboot` nicht ausgeloest.
- Nach RAM-Persistenztests wurde die Default-Config per `POST /api/config` wiederhergestellt und `targetFps:60` per `GET /api/config` bestaetigt.
- Builds nach Prompt-04-Aenderungen:
  - Native Core: bestanden.
  - Vollstaendige lokale `make test`: bestanden.
  - `esp32-wifi`: bestanden, RAM 15.8 Prozent, Flash 80.7 Prozent.
  - `esp32-rmii-ethernet`: bestanden, RAM 15.8 Prozent, Flash 83.5 Prozent.
  - `esp32-w5500`: bestanden, RAM 9.8 Prozent, Flash 51.7 Prozent.
- Prompt 04 bleibt teilweise offen, weil Config/Output/Mapping nur im RAM und nicht ueber Reboot persistent gespeichert sind, `POST /api/reboot` nicht ausgeloest wurde und echte UDP-Live-Preview mit Receiver nicht verifiziert ist.

Prompt-05-Processing-Visualizer-Ergebnis 2026-08-03:

- Neue testbare Kernmodule:
  - `processing/src/artnet.js`: ArtDMX-Decoder, Testpacket-Erzeugung und Direct-Art-Net-Frame-Assembler.
  - `processing/src/mappingLoader.js`: neutrales Mapping laden, validieren und per Fill/Contain/None normalisieren.
  - `processing/src/statistics.js`: empfangene FPS, gerenderte FPS, verlorene Frames, unvollstaendige Frames und Durchsatz.
- Neuer Test: `tests/test_processing_visualizer.js`.
- `Makefile` fuehrt `tests/test_processing_visualizer.js` jetzt in `make test` aus.
- `Makefile` hat jetzt `processing-build` und `processing-run` fuer die vorhandene Processing-3.5.4-CLI.
- Processing-Sketch erweitert um sichtbare Modi fuer Controller Preview, Direct Art-Net und Playback, Quality/Balanced/Performance-Anzeige, Achsen, Bounding Box, Zoom, Glow und Demo-Pixelzahlen.
- Processing-Sketch erweitert um echte UDP-Receiver:
  - Controller Preview auf UDP `6455`.
  - Direct Art-Net auf UDP `6454`.
- Firmware erweitert um `POST /api/preview/target`, damit der ESP32 das Processing-Ziel zur Laufzeit setzen kann.
- Bugfix: `PreviewStreamer` speichert den Zielhost jetzt als `std::string`; vorher wurde ein kurzlebiger JSON-Puffer-Pointer gespeichert.
- Playback-Core und Hot-Reload-Core in `processing/src/playback.js` hinzugefuegt und getestet.
- Tests bestanden:
  - `node tests/test_processing_protocol.js`
  - `node tests/test_processing_visualizer.js`
  - vollstaendiges `make test`
- Processing-Installationen gefunden:
  - `C:\Program Files\Processing\Processing.exe` (Processing 4 GUI, keine `processing-java.exe` in diesem Ordner gefunden).
  - `C:\Program Files (x86)\processing-3.5.4\processing-java.exe`.
  - `C:\Program Files (x86)\processing-2.2.1-\processing-java.exe`.
- Processing-CLI-Build: `processing-3.5.4\processing-java.exe --build` fuer `processing/ArtNetMixedLedVisualizer` erfolgreich mit Ausgabe `Finished.`.
- Processing-Makefile-Build: `make processing-build` erfolgreich mit Ausgabe `Finished.`.
- Runtime-Smoke-Test mit Processing 3.5.4 CLI:
  - Lokaler Preview-Test: Windows-Node sendete SLPV-Chunks an `127.0.0.1:6455`; Processing loggte `received frame source=preview:1234 frameId=101 pixels=12`.
  - Lokaler Direct-Art-Net-Test: Windows-Node sendete 27 Universen an `127.0.0.1:6454`; Processing loggte `received frame source=direct-artnet frameId=1 pixels=4500`.
  - ESP32 Controller-Preview: `POST /api/preview/target` auf `10.0.0.173:6455` lieferte HTTP 200; 27 langsame Art-Net-Universen an `10.0.0.246:6454` ergaben `/api/status` mit `packets:27`, `framesComplete:1`; Processing loggte `received frame source=preview:0 frameId=1 pixels=4500`.
- Builds nach Preview-Ziel-Fix:
  - Native Core: bestanden.
  - Vollstaendige lokale `make test`: bestanden.
  - `esp32-wifi`: bestanden, RAM 15.8 Prozent, Flash 80.8 Prozent; auf `COM6` geflasht.
  - `esp32-rmii-ethernet`: bestanden, RAM 15.8 Prozent, Flash 83.6 Prozent.
  - `esp32-w5500`: bestanden, RAM 9.8 Prozent, Flash 51.8 Prozent.
- Prompt 05 bleibt teilweise offen, weil Processing-4-CLI nicht gefunden wurde, Playback/Hot-Reload noch nicht als Bedienung im `.pde`-Sketch umgesetzt sind und echte Runtime-Performance-Langlaeufe nicht gemessen wurden.

Prompt-08-Performance-Hardware-Ergebnis 2026-08-03:

- `tools/performance-matrix.js` erweitert:
  - APA102-Zielkonfigurationen enthalten jetzt Gesamtpixel und laengste Ausgangskette getrennt.
  - WS2812B-Zielkonfigurationen enthalten jetzt Gesamtpixel und laengste Ausgangskette getrennt.
  - Mixed-Zielkonfigurationen `1500 APA102 + 2000 WS2812B`, `1500 APA102 + 3000 WS2812B` und `2000 APA102 + 3000 WS2812B` bei 30/40/50/60 FPS hinzugefuegt.
  - CSV-Spalten sind stabil und markieren alle theoretischen Zeilen mit `measured:false`.
- Neues Tool `tools/hardware-benchmark.js`:
  - sendet Art-Net an einen Controller,
  - liest vorher/nachher `/api/status`,
  - liest `/api/performance`,
  - dokumentiert HTTP-Latenzen und abgeleitete Paket-/Frame-Deltas,
  - hat einen lokalen Dry-Run-Test in `tests/test_hardware_benchmark_tool.js`.
- Firmware-API erweitert: `/api/status` und `/api/performance` geben jetzt zusaetzlich `frameTimeUs`, `outputTimeUs` und `lastFrameLatencyMs` aus. Diese Source-Aenderung wurde gebaut, aber nicht erneut auf COM6 geflasht.
- Echte ESP32-WiFi-API-Kurzmessung:
  - Report: `reports/hardware-benchmark-esp32-wifi-mixed-4500-30fps-3s.json`.
  - Ziel: `10.0.0.246`.
  - Szenario: `mixed_4500_30`, 4500 Pixel, 27 Universen, 30 FPS, 3 Sekunden.
  - Gesendet: 2430 Pakete / 90 Frames.
  - Controller-Delta: 646 empfangene Pakete, 2 vollstaendige Frames, 0 unvollstaendige Frames.
  - Geschaetzter Paketverlust: 1784 Pakete.
  - Completion Ratio: 0.0222.
  - Web-Latenzen: `/api/status` vorher 1347.84 ms, `/api/status` nachher 77.06 ms, `/api/performance` nachher 66.88 ms.
  - Interpretation: ESP32-WiFi ist fuer den vollen 4500-Pixel-Mixed-Load bei 30 FPS in dieser Messung nicht ausreichend stabil.
- Dokumentation aktualisiert:
  - `docs/performance/BENCHMARK_RESULTS.md`
  - `docs/architecture/HARDWARE_OPTIONS.md`
- Builds/Tests nach Prompt-08-Aenderungen:
  - Vollstaendige lokale `make test benchmark`: bestanden.
  - `esp32-wifi`: bestanden, RAM 15.8 Prozent, Flash 80.8 Prozent.
  - `esp32-rmii-ethernet`: bestanden, RAM 15.8 Prozent, Flash 83.7 Prozent.
  - `esp32-w5500`: bestanden, RAM 9.8 Prozent, Flash 51.9 Prozent.
- Prompt 08 bleibt teilweise offen, weil keine echte APA102-/WS2812B-Ausgabe, keine Oszilloskop-/Logic-Analyzer-Signalmessung, keine RMII-/W5500-Hardwarelaufmessung, kein Teensy-Runtime-Benchmark, keine CPU-Load-Messung und kein Langzeittest verifiziert wurden.

Live-Visualisierung 2026-08-04:

- Processing-Visualizer wurde ueber Processing-3.5.4-Build direkt mit Java gestartet.
- UDP-Ports auf Windows verifiziert:
  - `0.0.0.0:6454` Direct Art-Net
  - `0.0.0.0:6455` Controller Preview
- Processing-Log verifiziert:
  - `ArtNetMixedLedVisualizer ready previewPort=6455 artNetPort=6454`
  - wiederholte `received frame source=preview:0 ... pixels=4500`
- ESP32 unter `10.0.0.246` war erreichbar.
- 9 langsame vollstaendige Art-Net-Frames an ESP32 `10.0.0.246:6454` gesendet.
- ESP32-Status danach: `packets:916`, `framesComplete:12`, `framesIncomplete:0`.
- Processing sah Preview-Frame-IDs bis `frameId=12` mit `pixels=4500`.
- Ergebnis: Live-Visualisierung ueber ESP32 Controller Preview funktioniert. Echte physische LED-Ausgabe bleibt davon getrennt und weiterhin nicht als Hardware-LED-Test bestanden markiert.

Mapping-Visualisierung 2026-08-04:

- Mapping-Datei `Mappings/schrankwand -onlyu.txt` gelesen.
- Format: JSON-Liste mit 1179 `[x,y,z]`-Punkten; 25 davon sind `[0,0,0]`.
- Processing-Visualizer laedt das Mapping automatisch und normalisiert die Koordinaten.
- Visualizer startet mit 3D-Mapping-Ansicht; Taste `v` schaltet zwischen Mapping/3D und Grid-Ansicht.
- `make processing-build` nach Mapping-Aenderung bestanden.
- Laufender Visualizer meldete: `loaded mapping Mappings/schrankwand -onlyu.txt points=1179`.
- UDP-Ports `6454` und `6455` waren offen.
- Testmuster per `tools/visualizer-test-patterns.js --host 10.0.0.246` gesendet: aus, weiss, rot, gruen, blau, Farb-Bloecke, Lauflicht, weiss final.
- ESP32-Status danach: `packets:5179`, `framesComplete:122`, `framesIncomplete:0`.
- Processing sah Preview-Frames bis `frameId=122` mit `pixels=4500`.
- Hinweis: Das Mapping hat 1179 Punkte, der ESP32-Frame hat 4500 Pixel. Der Visualizer rendert deshalb aktuell die ersten 1179 Pixel auf die Schrankwand-Koordinaten. Fuer eine vollstaendige 4500-Pixel-Geometrie wird ein Mapping mit 4500 Punkten benoetigt.

Output-0-16x16-Matrix-Test 2026-08-04:

- Nutzer hat an APA102 Output 0 eine 16x16-Testmatrix mit 256 LEDs angeschlossen.
- Visualizer erweitert um automatisch generiertes `output0-16x16-serpentine` Mapping mit 256 Punkten.
- Neues Tool `tools/output0-matrix16-animation.js` hinzugefuegt.
- `make processing-build` nach 16x16-Aenderung bestanden.
- Visualizer neu gestartet; Log: `loaded generated mapping output0-16x16-serpentine points=256`.
- UDP-Ports `6454` und `6455` offen.
- Animation an ESP32 `10.0.0.246` gesendet:
  - aus
  - weiss
  - Row-Scan
  - Column-Scan
  - Checker
  - Diagonal
  - Orbit
  - Plasma
  - weiss final
- ESP32-Status danach: `packets:13028`, `framesComplete:472`, `framesIncomplete:0`, `frameTimeUs:27114`, `outputTimeUs:27114`.
- Test nutzt Serpentine/Zickzack-Reihenannahme. Falls physische Reihen alternierend falsch herum laufen, naechster Test mit `--wiring straight`.

Output-0-16x16-Matrix-Optimierung 2026-08-04:

- Dediziertes PlatformIO-Profil `esp32-wifi-matrix16` nutzt jetzt nur noch 256 Pixel / 2 Art-Net-Universen fuer den Testaufbau.
- Firmware-Build `esp32-wifi-matrix16`: bestanden.
- Firmware-Upload auf `COM6`: bestanden.
- ESP32-Konfiguration danach verifiziert:
  - `pixelCount:256`
  - `universeCount:2`
  - Output 0 APA102 aktiv, `dataPin:23`, `clockPin:18`, `pixelCount:256`, `spiHz:4000000`
- Visualizer fuer 16x16-Test angepasst:
  - Direct-Art-Net-Erwartung auf 256 Pixel / 2 Universen gesetzt.
  - 16x16-Grid nutzt jetzt die Mapping-Koordinaten statt linearer Pixelindex-Anordnung.
  - Konsolenlogging gedrosselt, damit Processing bei Preview-Frames nicht ausgebremst wird.
- Testsender `tools/output0-matrix16-animation.js` erweitert um `--brightness`.
- Gedimmter Hardware-/Visualizer-Vergleich:
  - Befehl: `node tools/output0-matrix16-animation.js --host 10.0.0.246 --wiring serpentine --fps 24 --gap 0 --universes 2 --brightness 0.35`
  - Nutzerbefund korrigiert: physische Matrix sah bereits vorher korrekt aus; 35 Prozent war ein angenehmer gedimmter Vergleich, aber nicht der eigentliche Fix.
  - ESP32-Status danach: `packets:4038`, `framesComplete:2019`, `framesIncomplete:0`, `frameTimeUs:6189`, `outputTimeUs:6189`.
  - Processing-Visualizer neu gestartet; Log bestaetigt `loaded generated mapping output0-16x16-serpentine points=256` und Preview-Frames mit `pixels=256`.
- Hellerer FPS-Test mit 70 Prozent Helligkeit:
  - 30 FPS: bestanden; ESP32 danach `packets:5384`, `framesComplete:2692`, `framesIncomplete:0`, `frameTimeUs:6246`, `outputTimeUs:6246`.
  - 45 FPS: bestanden; ESP32 danach `packets:6730`, `framesComplete:3365`, `framesIncomplete:0`, `frameTimeUs:6200`, `outputTimeUs:6200`.
  - 60 FPS: bestanden; ESP32 danach `packets:8076`, `framesComplete:4038`, `framesIncomplete:0`, `frameTimeUs:6251`, `outputTimeUs:6251`.
  - Nutzerwunsch: 70 Prozent Helligkeit als sinnvoller naechster Betriebspunkt testen/verwenden.
  - Interpretation: Bei 256 APA102-Pixeln ist der ESP32-Output mit ca. 6.2 ms pro Frame nicht am 30-FPS-Limit; 60 FPS sind in diesem Testaufbau controllerseitig ohne unvollstaendige Frames gelaufen. Visuelle Langzeitstabilitaet und Stromversorgung bleiben separat zu beobachten.
- Schwarzwert-/Langlauf-Test:
  - Beobachtung: Das leichte Blau in dunklen Bereichen kam vom Testmuster selbst (`[0,0,8]` bis `[0,0,20]` als sichtbarer Hintergrund), nicht von einem belegten Firmware-Fehler.
  - `tools/output0-matrix16-animation.js` erweitert um `--background`; Standard ist jetzt echtes Schwarz `0`.
  - `tools/output0-matrix16-animation.js` erweitert um `--duration-scale` fuer laengere Pattern-Laeufe.
  - Langer Testlauf: `node tools/output0-matrix16-animation.js --host 10.0.0.246 --wiring serpentine --fps 60 --gap 0 --universes 2 --brightness 0.70 --background 0 --duration-scale 3`
  - ESP32-Status danach: `packets:12112`, `framesComplete:6056`, `framesIncomplete:0`, `frameTimeUs:6219`, `outputTimeUs:6219`.
  - Lokal verifiziert nach Tool-Aenderung: `make test NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"` bestanden.

FastLED-Sketch-Player 2026-08-04:

- Vorlage geprueft: `SooO00_Smooth_Teensy4-FINAL.ino.cpp` ist ein Teensy4/OctoWS2811/FastLED-Sketch mit 24x32 sichtbarer Matrix und drei Animationen: `noise_noise2`, `rotating_blob`, `waves_animation`.
- Neuer Art-Net-Player: `tools/fastled-sketch-player.js`.
  - Spielt visuell nachgebildete Varianten der drei FastLED-Animationen.
  - Standardmodus `fit16` skaliert den 24x32-Look auf die aktuelle 16x16-Testmatrix.
  - Unterstuetzt `--fps`, `--brightness`, `--seconds`, `--pattern-seconds`, `--crossfade`, `--wiring`.
  - `Makefile`-Target `fastled-sketch-player` hinzugefuegt.
- Verifikation:
  - Dry-Run: bestanden.
  - `make test NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"`: bestanden.
  - Live-Lauf: `node tools/fastled-sketch-player.js --host 10.0.0.246 --mode fit16 --wiring serpentine --fps 60 --brightness 0.70 --seconds 90 --pattern-seconds 25 --crossfade 6`
  - ESP32-Status danach: `packets:22859`, `framesComplete:11426`, `framesIncomplete:0`, `frameTimeUs:6063`, `outputTimeUs:6063`.
- Hinweis: Das ist ein Art-Net-Player fuer die ESP32-Testkette, kein 1:1-Kompilat des Teensy/OctoWS2811-Codes. Die visuelle Logik ist nachgebildet, damit sie auf der aktuellen 16x16-APA102-Matrix und im Visualizer abgespielt werden kann.
- Nachjustierung nach Nutzerfeedback:
  - Bewegung war zu schnell; neuer Parameter `--speed` trennt interne Animationsgeschwindigkeit von der realen Pattern-/Crossfade-Zeit.
  - Orange-Drift kam aus Gruenanteilen in nachgebauten Patterns; `rotating_blob` und `waves_animation` wurden auf Rot/Magenta/Blau ohne Gruenbeimischung angepasst.
  - `Makefile`-Target `fastled-sketch-player` nutzt jetzt langsamere Defaults: `--speed 0.35`, `--seconds 300`, `--pattern-seconds 75`, `--crossfade 18`.
  - Korrektur: Patternwechsel laufen jetzt in echten Sekunden; `--speed` verlangsamt nur die Bewegung.
  - Verifikation: `make test NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"` bestanden.
  - Live-Test: `node tools/fastled-sketch-player.js --host 10.0.0.246 --mode fit16 --wiring serpentine --fps 60 --brightness 0.70 --speed 0.35 --seconds 120 --pattern-seconds 35 --crossfade 10`.
  - ESP32-Status danach: `packets:54470`, `framesComplete:27230`, `framesIncomplete:0`, `frameTimeUs:6103`, `outputTimeUs:6103`.
- Endlosmodus / Bedienung:
  - `tools/fastled-sketch-player.js` unterstuetzt jetzt `--loop`; alternativ bedeutet `--seconds 0` ebenfalls Endloslauf.
  - `Makefile`-Targets hinzugefuegt:
    - `fastled-sketch-player-loop`: Endloslauf im Vordergrund, stoppbar mit `Ctrl+C`.
    - `fastled-sketch-player-start`: Endloslauf im Hintergrund, PID in `reports/fastled-sketch-player.pid`, Log in `reports/fastled-sketch-player.log`.
    - `fastled-sketch-player-stop`: stoppt den Hintergrundplayer per PID.
  - Hintergrundlauf gestartet mit 100 Prozent Helligkeit: PID `13482`.
  - ESP32-Status kurz nach Start: `fps:60`, `packetsPerSecond:120`, `framesIncomplete:0`, `frameTimeUs:6126`, `outputTimeUs:6126`.

Player-GUI 2026-08-04:

- Neue Windows-Startdatei: `SchrankLED-Player-GUI.cmd`.
  - Start per Doppelklick aus dem Projektordner.
  - Startet lokalen Server auf `http://127.0.0.1:8765`.
  - Oeffnet automatisch den Browser.
- Neuer lokaler Control-Server: `tools/player-control-server.js`.
  - Startet/stoppt `tools/fastled-sketch-player.js`.
  - Speichert GUI-Einstellungen in `reports/player-gui-settings.json`.
  - Schreibt Lauf-Log nach `reports/player-control-server.log`.
  - Fragt ESP32-Status ueber `/api/status` ab.
- Neue GUI-Seite: `web/player-control.html`.
  - Start/Stop/Restart.
  - Regler fuer Helligkeit und Bewegung.
  - Felder fuer FPS, Pattern-Dauer, Crossfade, ESP-IP, Mapping und Wiring.
  - Live-Anzeige fuer Player/ESP-Status, Frames, Incomplete Frames und Output-Zeit.
- Verifikation:
  - `tools/player-control-server.js --self-test`: bestanden.
  - Syntaxcheck fuer `tools/player-control-server.js` und `tools/fastled-sketch-player.js`: bestanden.
  - Lokaler API-Test auf Port `8767`: Start erzeugte Player-PID `13744`; ESP meldete `fps:59`, `packetsPerSecond:119`, `framesIncomplete:0`; Stop beendete PID `13744`.
  - `make test NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"`: bestanden.
- Nach Testabschluss liefen keine `player-control-server`- oder `fastled-sketch-player`-Prozesse mehr.
- Ueberarbeitung nach Nutzerfeedback:
  - Ursache fuer wirkungslose Regler: GUI speicherte Werte, aber der laufende Player bekam keine Live-Updates.
  - `tools/fastled-sketch-player.js` akzeptiert jetzt Live-Settings ueber stdin als JSON-Zeilen.
  - `tools/player-control-server.js` haelt stdin des Players offen und sendet Aenderungen aus `/api/settings` live an den laufenden Prozess.
  - GUI-Regler fuer Helligkeit und Bewegung senden jetzt waehrend des Ziehens live; kein Apply-Button noetig.
  - GUI verhindert, dass der automatische Refresh gerade bearbeitete Regler ueberschreibt.
  - Playlist-Checkboxen hinzugefuegt.
  - Player unterstuetzt `--playlist` mit Pattern-Liste.
  - Pattern-Katalog fuer GUI/Player:
    - `noise_noise2`
    - `rotating_blob`
    - `waves_animation`
  - Hinweis: Drei zwischenzeitlich nachgebaute Xorcery-artige Patterns wurden auf Nutzerwunsch wieder entfernt, weil sie nicht dem gewuenschten echten Xorcery/Pixelblaze-Code entsprachen.
  - API-Live-Test auf Port `8767`: Live-Update bestaetigte `live settings ...` ohne Player-Neustart.
  - ESP32-Status waehrend Live-Test: `framesIncomplete:0`; Stop beendete Testprozess.
  - Gespeicherte GUI-Defaults wieder auf 100 Prozent Helligkeit, Speed `0.35`, 60 FPS und volle Playlist gesetzt.
  - `make test NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"`: bestanden.
- Live-Fix nach weiterem Nutzerfeedback:
  - Problem: Start/Stop uebernahm Reglerwerte, aber Live-Regler kamen in der Windows-GUI nicht zuverlaessig beim laufenden Player an.
  - `tools/fastled-sketch-player.js` liest jetzt zusaetzlich zu stdin eine `--control-file` mehrfach pro Sekunde.
  - `tools/player-control-server.js` schreibt bei jeder `/api/settings`-Aenderung `reports/player-live-settings.json` und versucht weiterhin stdin.
  - Dadurch sind Helligkeit, Speed, FPS, Pattern-Dauer, Crossfade und Playlist nicht mehr vom stdin-Verhalten des Windows-Prozesses abhaengig.
  - Direkter Player-Test: Laufender Player uebernahm Aenderung aus `reports/player-live-settings-test.json` ohne Neustart.
  - GUI-Server-Test: `/api/settings` schrieb Live-Datei; laufender Player PID `14835` uebernahm live Helligkeit, Speed, FPS und Playlist.
  - Testdatei wieder entfernt; gespeicherte GUI-Defaults erneut auf 100 Prozent, Speed `0.35`, 60 FPS und volle Playlist gesetzt.
  - `make test NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"`: bestanden.
- Xorcery-Auswahl-Korrektur:
  - Nutzer stellte klar: Die drei nachgebauten Xorcery-Patterns sollen aus der Auswahl raus.
  - Entfernt aus `tools/fastled-sketch-player.js`: `xorcery_smooth_rectangles`, `xorcery_blue_violet_glass`, `xorcery_warm_pulse_no_orange`.
  - Entfernt aus `tools/player-control-server.js` Pattern-Katalog.
  - `reports/player-gui-settings.json` und `reports/player-live-settings.json` auf die drei FastLED-Patterns zurueckgesetzt.
  - `tools/fastled-sketch-player.js --dry-run --playlist noise_noise2,rotating_blob,waves_animation,xorcery_smooth_rectangles`: bestanden; unbekanntes Xorcery-Pattern wird ignoriert.
  - `tools/player-control-server.js --self-test`: bestanden.
  - `make test NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"`: bestanden.
- Xorcery Enhanced aus Nutzerbeispiel:
  - Nutzer lieferte konkreten Pixelblaze-Code `Xorcery 2D/3D - Enhanced Edition`.
  - Pattern wurde kurzzeitig als `xorcery_enhanced` portiert und live getestet.
  - Nutzerfeedback: Look passt nicht, weil die originalen Pixelblaze-Regler zum Feintuning fehlen.
  - `xorcery_enhanced` wurde wieder aus `tools/fastled-sketch-player.js`, GUI-Pattern-Katalog und gespeicherten GUI-/Live-Settings entfernt.
  - Aktuelle GUI/Player-Playlist enthaelt wieder nur `noise_noise2`, `rotating_blob`, `waves_animation`.
  - Neuer Plan fuer spaeter: Xorcery nicht als statischen Port einbauen, sondern mit passenden Pattern-spezifischen Reglern/Controls neu implementieren.
  - `tools/fastled-sketch-player.js --dry-run --playlist noise_noise2,rotating_blob,waves_animation,xorcery_enhanced`: bestanden; unbekanntes `xorcery_enhanced` wird ignoriert.
  - `tools/player-control-server.js --self-test`: bestanden.
- Lokal verifiziert:
  - `make processing-build`: bestanden.
  - `make test NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"`: bestanden.
- Hinweis: `make test` ohne explizites `NODE=...` nutzt lokal Node.js 10.24.0 und scheitert am Playwright-Test, weil Playwright Node.js 18+ verlangt. Mit dem gebuendelten Node-Runtime-Pfad bestehen die Tests.

Schrankwand GC-6612pro Player-Profil 2026-08-06:

- Neue Windows-Startdatei: `SchrankLED-Player-GUI-Schrankwand.cmd`.
  - Startet die Player-GUI auf `http://127.0.0.1:8766`.
  - Nutzt ein eigenes Profil `schrankwand`, damit die bisherige Testmatrix-GUI auf Port `8765` unveraendert bleibt.
  - Zielcontroller: `10.0.0.244`, Art-Net UDP Port `6454`.
  - Start-Universe: `1`, passend zur GC-6612pro-Default-Anzeige `Start Universe 0001`.
  - Mapping-Datei: `Mappings\schrankwand -onlyu.txt`.
- GC-6612pro-Controller-Logik aus Handbuch geprueft:
  - Controller unterstuetzt 12 Ports.
  - Ein RGB-Art-Net-Universe entspricht 170 Pixeln.
  - Controller ist laut Screenshot auf individuelle Portlaengen gesetzt: Port 1 = 3 Universes, Port 2 = 2 Universes, Port 3 = 3 Universes.
- Port-Aufteilung fuer das neue Profil:
  - Port 1: 509 LEDs.
  - Port 2: 272 LEDs.
  - Port 3: 402 LEDs.
  - Gesamt logisch: 1183 LEDs.
  - Art-Net-Ausgabe gemaess Controller-Screenshot: Port 1 Universes 1-3, Port 2 Universes 4-5, Port 3 Universes 6-8.
  - Gesamt-Ausgabe: 8 Universes = 1360 RGB-Pixel-Slots.
  - Nicht genutzte Slots innerhalb der Ports bleiben schwarz, damit Port 2 und Port 3 korrekt an ihren GC-6612pro-Portgrenzen beginnen.
- `tools\fastled-sketch-player.js` erweitert:
  - Neuer Modus `schrankwand_onlyu`.
  - Neue Parameter `--mapping-file`, `--pixel-count`, `--port-pixels`, `--port-universes`, `--start-universe`.
  - Art-Net-Paketbau mappt Universe-Slots jetzt bei Portprofilen auf die richtigen logischen Pixel und sendet Padding schwarz.
- `tools\player-control-server.js` erweitert:
  - Neues Profil ueber `--profile schrankwand` oder `SCHRANK_PLAYER_PROFILE=schrankwand`.
  - Eigene Settings-Dateien: `reports\player-gui-settings-schrankwand.json` und `reports\player-live-settings-schrankwand.json`.
  - Default fuer Schrankwand: Host `10.0.0.244`, Start-Universe `1`, Modus `schrankwand_onlyu`, Portpixel `509,272,402`, `portUniverses=3,2,3`.
- `web\player-control.html` erweitert:
  - Mapping-Auswahl enthaelt jetzt `Schrankwand OnlyU 1183`.
- Mapping-Stand:
  - `Mappings\schrankwand -onlyu.txt` ist lokal lesbar und enthaelt 1179 Mapping-Punkte.
  - Die Controller-Portsumme ist 1183 LEDs; 4 logische LEDs haben damit aktuell keinen Mapping-Punkt und bleiben im Schrankwand-Modus schwarz.
  - Dieser Punkt ist offen, falls wirklich alle 1183 LEDs raeumlich korrekt animiert werden sollen.
- Lokal verifiziert:
  - Syntaxcheck `tools\fastled-sketch-player.js`: bestanden.
  - Syntaxcheck `tools\player-control-server.js`: bestanden.
  - `tools\player-control-server.js --self-test`: bestanden.
  - `SCHRANK_PLAYER_PROFILE=schrankwand tools\player-control-server.js --self-test`: bestanden.
  - Dry-Run: `tools\fastled-sketch-player.js --dry-run --host 10.0.0.244 --start-universe 1 --mode schrankwand_onlyu --pixel-count 1183 --mapping-file "Mappings\schrankwand -onlyu.txt" --port-pixels 509,272,402 --port-universes 3,2,3 --playlist noise_noise2,rotating_blob,waves_animation --brightness 1 --speed 0.35`: bestanden.
  - Dry-Run meldete: `pixels=1183`, `universes=8`, `startUniverse=1`, `mappedPixels=1179`, `requestedPixels=1183`, `artnetSlots=1360`.
- Noch nicht hardwareverifiziert:
  - Ob der GC-6612pro auf `10.0.0.244` die 8 Universes live annimmt.
  - Ob am GC-6612pro tatsaechlich weiter `Start Universe = 1` und Portlaengen `3,2,3` gesetzt sind.
  - Ob Port 1/2/3 physisch exakt mit 509/272/402 LEDs bestueckt und in der erwarteten Reihenfolge verdrahtet sind.

Schrankwand GC-6612pro Flacker-Korrektur 2026-08-06:

- Nutzer meldete: Ausgabe flackert nur und wird nicht richtig angezeigt.
- Befund im Log:
  - Laufender Schrankwand-Player wurde mit kaputter gespeicherter Portliste gestartet: `portPixels:[0]`, `portUniverses:[0]`.
  - Dadurch sendete der Player nur 7 durchgehende Universes statt der Controller-Struktur Port 1 = Universe 1-3, Port 2 = Universe 4-5, Port 3 = Universe 6-8.
- Korrekturen:
  - Laufende falsche Player-/GUI-Prozesse gestoppt.
  - `reports\player-gui-settings-schrankwand.json` und `reports\player-live-settings-schrankwand.json` auf `portPixels=[509,272,402]`, `portUniverses=[3,2,3]`, `startUniverse=1` gesetzt.
  - Default-FPS fuer Schrankwand-Settings auf `25` reduziert und Helligkeit auf `0.7`, um den ersten GC-6612pro-Test stabiler zu machen.
  - `tools\player-control-server.js`: Portlisten-Sanitizing repariert; `[0]` faellt jetzt auf die Default-Portlisten zurueck.
  - `tools\fastled-sketch-player.js`: ArtDMX-Laenge auf exakt `510` DMX-Kanaele gesetzt, also 170 RGB-Pixel pro Universe.
- Lokal verifiziert:
  - Syntaxcheck `tools\fastled-sketch-player.js`: bestanden.
  - Syntaxcheck `tools\player-control-server.js`: bestanden.
  - `tools\player-control-server.js --self-test`: bestanden.
  - `SCHRANK_PLAYER_PROFILE=schrankwand tools\player-control-server.js --self-test`: bestanden; prueft jetzt auch `[0]`-Fallback auf `509,272,402` und `3,2,3`.
  - Dry-Run: `tools\fastled-sketch-player.js --dry-run --host 10.0.0.244 --start-universe 1 --mode schrankwand_onlyu --pixel-count 1183 --mapping-file "Mappings\schrankwand -onlyu.txt" --port-pixels 509,272,402 --port-universes 3,2,3 --playlist noise_noise2,rotating_blob,waves_animation --brightness 0.7 --speed 0.35 --fps 25`: bestanden.
  - Dry-Run meldete: `universes=8`, `startUniverse=1`, `dmxLength=510`, `fps=25`, `portUniverses=3,2,3`.
- Weiter offen:
  - Hardware-Retest mit GC-6612pro nach Neustart der neuen GUI.
  - Falls es weiterhin flackert, naechster gezielter Test: `startUniverse=0` gegenpruefen, weil manche Art-Net-Tools 1-basiert anzeigen, aber 0-basiert senden.

Schrankwand Ein-Farben-Test 2026-08-06:

- Nutzer fragte nach einfachem Ein-Farben-Test zum Pruefen.
- `tools\fastled-sketch-player.js` erweitert um feste Testpatterns:
  - `test_black`
  - `test_solid_red`
  - `test_solid_green`
  - `test_solid_blue`
  - `test_solid_white`
- `tools\player-control-server.js` Pattern-Katalog erweitert, sodass die Testfarben in der GUI als Checkboxen auswählbar sind.
- Schrankwand-Settings auf stabilen Rot-Dauertest gesetzt:
  - Playlist: `test_solid_red`
  - FPS: `25`
  - Brightness: `0.7`
  - PatternSeconds: `600`
  - Crossfade: `0`
  - Portstruktur weiter `509,272,402` und `3,2,3`, Start-Universe `1`.
- Lokal verifiziert:
  - Syntaxcheck `tools\fastled-sketch-player.js`: bestanden.
  - Syntaxcheck `tools\player-control-server.js`: bestanden.
  - `tools\player-control-server.js --self-test`: bestanden.
  - `SCHRANK_PLAYER_PROFILE=schrankwand tools\player-control-server.js --self-test`: bestanden.
  - Dry-Run Rot: `firstPixel=179,0,0`, `universes=8`, `dmxLength=510`, `fps=25`: bestanden.

Schrankwand harter Art-Net-Farbtest 2026-08-06:

- Nutzer meldete: Auch feste Farben flackern in allen Farben.
- Schlussfolgerung: Fehler liegt wahrscheinlich nicht im Pattern, sondern in Art-Net/Controller-Handshake, Universe-Index, DMX-Laenge, Netzwerkkarte oder Sequencing.
- Laufender falscher Player wurde gestoppt.
- `tools\fastled-sketch-player.js` erweitert:
  - `--bind <ip>` zum Binden auf die lokale Art-Net-Netzwerkkarte, hier `10.0.0.173`.
  - `--dmx-length 510|512` zum Testen von exakt 170 RGB-Pixeln oder vollem DMX-Paket.
  - `--sequence-mode increment|fixed` zum Testen, ob der Controller mit Sequenznummern flackert.
- Neue Diagnose-Datei: `SchrankLED-ArtNet-Farbtest.cmd`.
  - Menue fuer Rot, Gruen, Blau, Weiss, Schwarz.
  - Standardtest: Start-Universe `1`, DMX-Laenge `510`, `5 FPS`, lokale Bind-IP `10.0.0.173`.
  - Diagnosevarianten:
    - Rot mit Start-Universe `0`.
    - Rot mit DMX-Laenge `512`.
    - Rot mit `1 FPS`.
    - Rot mit fixer Art-Net Sequence.
- Lokal verifiziert:
  - Syntaxcheck `tools\fastled-sketch-player.js`: bestanden.
  - Dry-Run fuer Farbtest-Standard: `host=10.0.0.244`, `bind=10.0.0.173`, `startUniverse=1`, `dmxLength=510`, `fps=5`, `patterns=test_solid_red`, `firstPixel=179,0,0`: bestanden.

Schrankwand Farbtest-Ergebnis / weitere Eingrenzung 2026-08-06:

- Nutzer meldete: Farbtests flackern weiterhin; Option `8` mit `1 FPS` macht gar nichts; LEDs zeigen weiterhin verschiedene Farben.
- `tools\fastled-sketch-player.js` korrigiert:
  - Art-Net Sequence `0` wird jetzt wirklich als `0` gesendet und nicht mehr auf `1` ersetzt.
  - Irrefuehrende negative Mapping-Warnung bei kleinen Diagnose-Pixelzaehlungen entfernt.
- `SchrankLED-ArtNet-Farbtest.cmd` erweitert:
  - `A`: Nur Universe 1 / Anfang Port 1, Rot, 5 FPS, Sequence fixed.
  - `B`: Nur Universe 1 / Anfang Port 1, Schwarz, 5 FPS, Sequence fixed.
  - `C`: Nur Universe 1 / Anfang Port 1, Rot, Start-Universe 0.
  - `D`: Broadcast `10.0.0.255`, Rot, Start-Universe 1.
- Lokal verifiziert:
  - Syntaxcheck `tools\fastled-sketch-player.js`: bestanden.
  - Dry-Run `A`: `pixels=170`, `universes=1`, `startUniverse=1`, `dmxLength=510`, `sequenceMode=fixed`, `bind=10.0.0.173`, `firstPixel=179,0,0`: bestanden.
- Interpretation:
  - Wenn `A` und `B` auf Hardware nicht eindeutig rot/schwarz fuer die ersten 170 LEDs ergeben, ist der Fehler sehr wahrscheinlich nicht Mapping oder Animation, sondern Controller-Output-Konfiguration, LED-IC-Type/Timing, physisches Signal/GND oder Controller-Modus.

Schrankwand Farbtest A-D ohne Wirkung 2026-08-06:

- Nutzer meldete: `A`, `B`, `C`, `D` machen gar nichts; Ausgabe bleibt wie vorher.
- Lokale Pruefung:
  - `Artnetominator.exe` war wieder aktiv und belegte UDP `10.0.0.173:6454`.
  - `ArtnetSettingTools.exe` war ebenfalls aktiv.
  - Ping/ARP zum Controller `10.0.0.244` ist grundsaetzlich vorhanden; ARP zeigt MAC `18-c0-4d-b7-bd-f4`.
  - ArtPoll vom PC erhielt keine Antwort vom GC-6612pro.
- Zusaetzlicher Test:
  - 10 Sekunden gesendet: Broadcast `10.255.255.255`, Start-Universe `0`, DMX-Laenge `512`, nur Universe 0, Rot, Sequence fixed.
  - Sendeprozess lief lokal fehlerfrei durch.
- `SchrankLED-ArtNet-Farbtest.cmd` erweitert:
  - `E`: klassische Broadcast-Variante ohne Bind-Zwang, Universe `0`, DMX `512`, nur 1 Universe Rot.
- Aktueller wahrscheinlichster Bereich:
  - Nicht GUI, Mapping oder Animation.
  - Zu pruefen am GC-6612pro: Online-/Art-Net-Preview-Modus aktiv, Settings wirklich gespeichert, korrekter konkreter LED-IC-Typ/Timing, Controller-Ausgang nicht im Offline/SD/Auto-Modus, Art-Net-Sender-Konflikte geschlossen.

GC-6612pro Re-Test nach Verkabelungscheck 2026-08-07:

- Nutzer hat Verkabelung geprueft und Controller `10.0.0.244` wieder eingeschaltet.
- Netzwerk:
  - Ping `10.0.0.244`: erfolgreich, ca. 1-2 ms.
  - ARP MAC: `18-c0-4d-b7-bd-f4`.
  - UDP 6454 war frei.
  - ArtPoll weiterhin ohne Antwort vom GC.
- Kontrolltest Sender gegen ESP:
  - Gleicher Art-Net-Testsender an ESP `10.0.0.246` zeigte erste 20 Pixel rot.
  - Damit ist der Sender als Fehlerquelle ausgeschlossen.
- GC-Tests:
  - Universe 1, erste 20 Pixel rot: keine eindeutige stabile Ausgabe.
  - Universe-Scan 0-4: Nutzer sah LEDs am Controller blinken.
  - Nach IC-Umstellung auf vom Nutzer genanntes `UCS2803`/vermutlich `UCS2903` und 25 Hz:
    - Test Universe 1 rot, Universe 2 gruen, Universe 3 blau, danach schwarz.
    - Nutzer sah Rot flackern; bei anderen LEDs gruen; die meisten gingen aus, aber einzelne blieben an.
  - Danach langer Blackout ueber Universe 0-15 gesendet.
  - Port-1-Test mit niedriger Helligkeit ueber Universes 1-3:
    - Rot/Gruen/Blau/Schwarz jeweils 8 s.
    - Nutzer meldete wieder Flackern; danach alles rot haengen geblieben und ein paar LEDs gruen.
- Interpretation:
  - Art-Net erreicht den GC und beeinflusst den Ausgang.
  - Universe-Zuordnung ist nicht der Hauptfehler.
  - Farbreihenfolge allein ist ebenfalls unwahrscheinlich, weil Schwarz nicht sauber loescht.
  - Wahrscheinlichster Fehler: falscher konkreter IC-Type/Timing am GC-6612pro oder Signal/GND/TTL-Anschlussproblem am LED-Ausgang.

ESP32 WS2812B Output-0 Testprofil 2026-08-06:

- Nutzer hat den ESP wieder zum Flashen angeschlossen und moechte WS2812 konfigurieren.
- Sichtbare serielle Ports:
  - `COM6`: Silicon Labs CP210x USB to UART Bridge, plausibler ESP32.
  - `COM12`: USB Serial Port, nicht als ESP32 verwendet.
- Firmware erweitert:
  - Neues PlatformIO-Environment `esp32-wifi-ws2812-output0`.
  - Neues Build-Flag `LED_PROFILE_WS2812_OUTPUT0`.
  - Default-Konfig fuer dieses Profil: 256 Pixel, Output 0, `WS2812B`, Datenpin `GPIO23`, kein Clock-Pin, ColorOrder `GRB`, Start-Universe `0`, Target-FPS `30`.
  - `firmware\src\LedOutputs.cpp`: echte FastLED-Ausgabe fuer WS2812B Output 0 auf `GPIO23` implementiert.
- Lokal verifiziert:
  - `make test NODE="C:\Users\jimmy\.cache\codex-runtimes\codex-primary-runtime\dependencies\node\bin\node.exe"`: bestanden.
  - PlatformIO `6.1.19` verfuegbar.
  - Build `pio run -e esp32-wifi-ws2812-output0`: bestanden; `firmware.bin` erzeugt.
  - Upload `pio run -e esp32-wifi-ws2812-output0 -t upload --upload-port COM6`: bestanden.
  - Serieller Monitor `COM6` mit 115200 Baud:
    - Boot erfolgreich.
    - `WS2812B output configured id=0 pixels=256 data=23`
    - `WS2812B output 0 hardware enabled on data=23 colorOrder=GRB`
    - WiFi verbunden.
    - IP: `10.0.0.246`
- Anschluss-Hinweis:
  - WS2812 DIN an ESP32 `GPIO23`.
  - LED-GND und ESP32-GND gemeinsam verbinden.
  - LED-5V extern versorgen; nicht die LED-Kette ueber den ESP32 speisen.
- Noch offen:
  - Hardware-Ausgabe mit angeschlossenen WS2812 LEDs testen.

ESP32 WS2812B Hardware-Farbtest 2026-08-06:

- Nutzer bestaetigte nach Testsendung: WS2812-Strip wird rot.
- Damit hardwareverifiziert:
  - ESP32 auf `COM6` erfolgreich geflasht.
  - Firmware `esp32-wifi-ws2812-output0` bootet.
  - WiFi-IP `10.0.0.246` erreichbar fuer Art-Net-Daten.
  - WS2812B Output 0 auf `GPIO23` gibt Daten aus.
  - WS2812-Timing/FastLED-Ausgabe auf `GPIO23` funktioniert grundsaetzlich.
- Gesendeter bestaetigter Test:
  - Art-Net an `10.0.0.246`, Universes `0` und `1`, 256 Pixel, Rot ca. 70 Prozent.
- Danach gesendet:
  - Aus, Gruen, Blau, Weiss, Aus als Sequenz.
  - Ergebnis der Folgefarben wurde noch nicht vom Nutzer bestaetigt.

ESP32 WS2812B Reflash mit Boot-Schwarz 2026-08-06:

- Nutzer hatte den ESP zwischenzeitlich abgesteckt; `COM6` war verschwunden.
- Erklaerung: WS2812 halten die zuletzt empfangene Farbe, daher blieb der Strip rot, obwohl der ESP nicht mehr erreichbar war.
- Firmware angepasst:
  - Nach `FastLED.addLeds<WS2812B, 23, GRB>` wird jetzt `FastLED.clear(true)` ausgefuehrt, damit der Strip beim Booten zuerst schwarz gesetzt wird.
- ESP wieder erkannt:
  - `COM6`: Silicon Labs CP210x USB to UART Bridge.
- Upload:
  - `pio run -e esp32-wifi-ws2812-output0 -t upload --upload-port COM6`: bestanden.
- Serieller Bootlog:
  - `WS2812B output configured id=0 pixels=256 data=23`
  - `WS2812B output 0 hardware enabled on data=23 colorOrder=GRB`
  - WiFi verbunden, IP `10.0.0.246`.
- Hardware-Test nach Reflash:
  - Schwarz gesendet, danach Gruen gesendet.
  - ESP-Webstatus antwortete danach:
    - `packets=160`
    - `framesComplete=80`
    - `framesIncomplete=0`
    - `outputTimeUs=10376`
- Noch offen:
  - Nutzerbestaetigung, ob die LEDs nach dem Reflash tatsaechlich von Rot auf Gruen gewechselt haben.

ESP32 WS2812B 16x16 Volltest 2026-08-07:

- Nutzer meldete nach 10-LED-Test: alles gut.
- Nutzer hat 16x16-Matrix an Output 0 und moechte Volltest.
- Gesendet an ESP `10.0.0.246`, Universes `0` und `1`, 256 Pixel:
  - Rot full fuer ca. 3 s.
  - Gruen full fuer ca. 3 s.
  - Blau full fuer ca. 3 s.
  - Weiss full kurz fuer ca. 2 s.
  - Aus fuer ca. 3 s.
- Status danach:
  - Ping erfolgreich, 0 Prozent Verlust.
  - `/api/status`: `packets=596`, `framesComplete=298`, `framesIncomplete=0`, `outputTimeUs=10338`.
- Interpretation:
  - ESP/Firmware/Art-Net-Verarbeitung blieb stabil.
  - Optische Nutzerbestaetigung fuer Vollweiss/Aus steht noch aus.

GC-6612pro Detail-Recherche WS2812/Flicker 2026-08-07:

- Online- und Handbuch-Recherche zum GC-6612pro / Club-Lights-12-Pro Controller durchgefuehrt.
- Relevante Hersteller-/Haendlerangaben:
  - GC-6612pro unterstuetzt SPI/TTL- und DMX512-Ausgabe, 12 Ports, Art-Net online preview.
  - Laut Handbuch sind u. a. `TM1812`, `WS2811`, `UCS2903`, `UCS9812`, `SM17512` typische unterstuetzte SPI-ICs.
  - Ausgangspin laut Handbuch: Pin 1 = GND, Pin 2 = `DA/D+`, Pin 3 = `D-`.
  - Fuer einfache einadrige WS281x-/UCS-/TM-SPI-LEDs ist in der Regel GND + `DA/D+` relevant; `D-` ist fuer RS485/differentielle DMX- bzw. passende Differential-Ausgabe.
  - Eine RGB-DMX-Universe entspricht 170 Pixeln; Portlaengen 1/2/3 entsprechen 170/340/510 RGB-Pixeln.
  - Start-Universe ist im Handbuch default `1`.
- Community-/Forenhinweise:
  - Art-Net-Flackern kann durch mehrere Sender/Nodes auf denselben Universen entstehen; deshalb fuer Tests ArtNetominator, TouchDesigner, Madrix/Resolume etc. geschlossen halten, sofern sie senden.
  - Ein dokumentierter GC-6612pro/Club-Lights-12-Pro WS2815-Fall wurde durch schlechte Masseverbindung geloest.
  - Allgemeine WS281x-Faelle mit buntem Flackern bei eigentlich festen Farben deuten haeufig auf Timing-/IC-Type-Mismatch, schwaches/gestoertes Datensignal, fehlende gemeinsame Masse oder Stromversorgungsprobleme.
- Konsequenz fuer unser Setup:
  - Da unser Blackout am GC-Controller nicht zuverlaessig alle Pixel loescht und feste Farben bunt/flackernd erscheinen, ist der Fehler aktuell wahrscheinlicher im Controller-IC-Type/Timing oder in Signal/GND/Versorgung als im Mapping.
  - `DMX512 250K/500K` ist fuer normale WS2812/WS2811-Style-Pixel nicht passend.
  - Wenn kein explizites `WS2812` im Controller-Menue vorhanden ist, ist `UCS2903` der erste Kandidat; danach systematisch `TM1812`, `TM1814`, `TM1914`, `User_0` testen.
  - RGB-Sortierung erst nach stabiler Schwarz/Rot/Gruen/Blau-Ausgabe feinjustieren; Color-Order erklaert keine haengenden Restpixel bei Blackout.
- Status:
  - Art-Net zum GC-6612pro `10.0.0.244` beeinflusst LEDs sichtbar.
  - GC-6612pro LED-Ausgabe ist noch nicht stabil verifiziert.
  - Naechster sinnvoller Test: alle anderen Art-Net-Sender schliessen, Controller auf `UCS2903`, RGBW aus, RGB order `RGB`, Speed `25Hz`, Start Universe `1`, Port1=3, Port2=2, Port3=3, speichern, Controller power-cyclen, danach Low-Level-Test Rot/Gruen/Blau/Schwarz auf Universes 1-8 senden.

GC-6612pro Low-Level-Test erneut gesendet 2026-08-07:

- Vor dem Test:
  - Ping zu `10.0.0.244`: erfolgreich, ca. 2 ms.
  - UDP `6454` war durch `MadMapper.exe` auf `10.5.0.2:6454` belegt; moeglicher Stoerfaktor, falls MadMapper parallel Art-Net sendet.
- Werkzeug korrigiert:
  - `tools\fastled-sketch-player.js` nutzte bei `--mode test_solid_red/green/blue` ohne explizite Playlist vorher weiter die Default-Playlist ab `test_black`.
  - Korrigiert: Wenn `--mode` einem Patternnamen entspricht, wird dieses Pattern als alleinige Playlist verwendet.
  - Dry-Run `--mode test_solid_red`: bestanden, erstes Pixel `179,0,0`.
- Gesendeter Test an GC-6612pro:
  - Ziel: `10.0.0.244:6454`
  - Bind: `10.0.0.173`
  - Start-Universe: `1`
  - Universes: `1-8`
  - Portstruktur: Port1 `509` Pixel / `3` Universes, Port2 `272` Pixel / `2` Universes, Port3 `402` Pixel / `3` Universes.
  - DMX-Laenge: `510`
  - FPS: `25`
  - Sequenz: increment
  - Reihenfolge: Schwarz 4 s, Rot 8 s bei 20 Prozent, Gruen 8 s bei 20 Prozent, Blau 8 s bei 20 Prozent, Schwarz 8 s.
- Noch offen:
  - Optische Rueckmeldung vom Nutzer, ob Farben und Blackout am GC-Controller jetzt stabil korrekt waren.

GC-6612pro 1-Minuten-Pattern-Test 2026-08-07:

- Gesendet an `10.0.0.244:6454`:
  - Mode: `schrankwand_onlyu`
  - Mapping: `Mappings\schrankwand -onlyu.txt`
  - Playlist: `waves_animation`
  - Dauer: 60 s
  - Helligkeit: 20 Prozent
  - FPS: 25
  - Speed: 0.18
  - Start-Universe: 1
  - Universes: 1-8
  - Portstruktur: 509/272/402 Pixel auf 3/2/3 Universes.
- Sendergebnis:
  - Player lief 60 s ohne lokalen Fehler durch.
  - Mapping-Datei lieferte 1179 Punkte fuer 1183 konfigurierte Pixel; 4 Pixel bleiben daher absichtlich schwarz.
- Noch offen:
  - Optische Nutzerbestaetigung, ob das Pattern am GC-Controller stabil und korrekt sichtbar war.

TouchDesigner MCP lokale Installation 2026-08-07:

- Nutzer bat um lokale Installation gemaess `https://github.com/8beeeaaat/touchdesigner-mcp/blob/main/docs/installation.md`.
- Geprueft:
  - TouchDesigner laeuft lokal.
  - Node.js `v24.13.0`, npm/npx `11.6.2`.
  - Aktuelles Release laut GitHub API: `v2.0.0`.
- Lokal installiert:
  - Release-Asset `touchdesigner-mcp-td.zip` heruntergeladen und entpackt nach:
    `tools\touchdesigner-mcp\touchdesigner-mcp-td-v2.0.0`
  - Vorhandene Import-Datei:
    `tools\touchdesigner-mcp\touchdesigner-mcp-td-v2.0.0\mcp_webserver_base.tox`
  - `modules\` liegt daneben; Ordnerstruktur muss fuer TouchDesigner intakt bleiben.
- Codex-Konfiguration:
  - Backup erstellt:
    `C:\Users\jimmy\.codex\config.toml.bak-touchdesigner-mcp-20260807-221749`
  - In `C:\Users\jimmy\.codex\config.toml` eingetragen:
    `[mcp_servers.touchdesigner] command = "npx", args = ["-y", "touchdesigner-mcp-server@latest", "--stdio"]`
- Verifiziert:
  - `npx -y touchdesigner-mcp-server@latest --stdio` startet ohne lokalen Installationsfehler.
- Noch offen / Nutzeraktion:
  - `mcp_webserver_base.tox` in TouchDesigner importieren, ideal als `/project1/mcp_webserver_base`.
  - Danach sollte `http://127.0.0.1:9981` erreichbar sein.
  - Codex neu starten bzw. Task neu laden, damit der neue MCP-Server in dieser Umgebung verfuegbar wird.
- Dokumentation:
  - `docs\TOUCHDESIGNER_MCP_LOCAL_SETUP.md` angelegt.

ESP32 neue Mixed-Output-Konfiguration Blocker 2026-08-17:

- Nutzer/Koordinator meldete neue ESP32-Hardwarekonfiguration:
  - 1 x WS2812-Ausgang auf `GPIO23`.
  - 2 x APA102-Ausgaenge auf `GPIO18` und `GPIO19`, jeweils 512 LEDs.
  - Insgesamt 1167 APA102-LEDs.
- Projektzustand geprueft:
  - `platformio.ini` enthaelt bestehende ESP32-WiFi-Profile und lokales PlatformIO ist vorhanden.
  - `firmware\src\ConfigManager.cpp` enthaelt aktuell u. a. ein WS2812-Testprofil auf `GPIO23`.
  - `firmware\src\LedOutputs.cpp` treibt APA102 hardwareseitig aktuell nur fuer Output 0 mit `data=23`, `clock=18`, maximal 500 Pixeln.
  - Diese alte APA102-Hardcodierung kollidiert mit der neuen Vorgabe `WS2812 auf GPIO23`.
- Kritische Ambiguitaeten:
  - APA102 benoetigt pro Ausgang mindestens Data und Clock. Die Angabe `APA102-Ausgaenge auf GPIO18 und GPIO19` nennt nur zwei GPIOs fuer zwei Ausgaenge; es ist offen, ob dies Data-Pins, Clock-Pins oder ein Data/Clock-Paar fuer nur einen Ausgang sein sollen.
  - `2 x 512 LEDs` ergibt 1024 APA102-Pixel, passt aber nicht zu `insgesamt 1167 APA102-LEDs`. Offen ist, ob 1167 die echte APA102-Gesamtzahl ist und wie sie auf die zwei Ausgaenge verteilt werden soll.
- Port-/Toolchain-Check:
  - Projektlokales PlatformIO: `PlatformIO Core, version 6.1.19`.
  - Windows-Serielliste und PlatformIO sehen aktuell `COM12`:
    `USB Serial Port (COM12)`, Hardware ID `USB VID:PID=04D8:00DF SER=0002575727`.
  - `COM12` ist ein sichtbarer USB-Serial-Port, aber nicht eindeutig als alter CP210x-ESP32 erkannt.
- Ergebnis:
  - Kein Firmware-Flash ausgefuehrt.
  - Keine LED-Hardwaretests ausgefuehrt.
  - Grund: Ein Flash mit falscher APA102 Data/Clock-Zuordnung oder falscher Pixelaufteilung waere ein potenziell falscher Hardwaretest.
- Benoetigte Nutzerentscheidung:
  - Fuer APA102 Output A: Data-GPIO und Clock-GPIO.
  - Fuer APA102 Output B: Data-GPIO und Clock-GPIO.
  - Exakte Pixelaufteilung: entweder `512 + 512` oder Summe `1167` mit konkreter Aufteilung, z. B. `512 + 655`.
  - Bestaetigung, ob `COM12` der neu angeschlossene ESP32 ist.

ESP32 Mixed WS2812B+APA102 Profil umgesetzt, Upload blockiert 2026-08-17:

- Praezisierte verbindliche Konfiguration vom Koordinator:
  - WS2812B: 1 Ausgang, Data `GPIO23`, 512 LEDs.
  - APA102: 1 Ausgang, insgesamt 1167 LEDs.
  - APA102 `GPIO18` und `GPIO19` sind ein Data/Clock-Paar.
  - Projektueblich/sichere Annahme fuer dieses Profil: `GPIO18 = APA102 Data`, `GPIO19 = APA102 Clock`.
- Firmware angepasst:
  - Neues PlatformIO-Environment:
    `esp32-wifi-ws2812-apa102-1679`
  - Neues Build-Flag:
    `LED_PROFILE_WS2812_APA102_1679`
  - Neue Controller-Konfiguration:
    - `pixelCount = 1679`
    - `startUniverse = 0`
    - `universeCount = 10`
    - `targetFps = 30`
    - Output 0: `WS2812B`, 512 Pixel, StartPixel 0, Data `GPIO23`, ColorOrder `GRB`, StartUniverse 0.
    - Output 1: `APA102`, 1167 Pixel, StartPixel 512, Data `GPIO18`, Clock `GPIO19`, ColorOrder `BGR`, SPI 4 MHz, StartUniverse 4.
  - `LedOutputs.cpp` erweitert:
    - APA102 Output 1 hardwareseitig auf `data=18`, `clock=19`, maximal 1200 Pixel.
    - WS2812B Output 0 bleibt auf `data=23`, maximal 600 Pixel.
    - FastLED-Frameausgabe so angepasst, dass pro fertigem Controller-Frame nur einmal `FastLED.show()` ausgefuehrt wird.
    - Output-Adapter werden jetzt beim Anzeigen anhand ihrer Output-ID zugeordnet.
  - Bootlog-Profiltext erweitert:
    `Profile: ESP32 WiFi WS2812B GPIO23 + APA102 data=18 clock=19`
- Lokal verifiziert:
  - `make test NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"`: bestanden.
  - PlatformIO-Build:
    `.venv-platformio/Scripts/pio.exe run -e esp32-wifi-ws2812-apa102-1679`: bestanden.
  - Build-Speicher:
    - RAM: 59472 Bytes / 327680 Bytes = 18.1 Prozent.
    - Flash: 1082841 Bytes / 1310720 Bytes = 82.6 Prozent.
- Port-/ESP32-Pruefung:
  - Sichtbarer Port: `COM12`.
  - Windows meldet:
    `USB Serial Port (COM12)`, PNP `USB\VID_04D8&PID_00DF&MI_00\7&18683CED&0&0000`.
  - Aktive Espressif-Erkennung:
    `.venv-platformio/Scripts/pio.exe pkg exec -p tool-esptoolpy -- esptool.py --chip auto --port COM12 --baud 115200 chip_id`
  - Ergebnis:
    `Failed to connect to Espressif device: Invalid head of packet (0xF8): Possible serial noise or corruption.`
- Ergebnis:
  - Kein Upload/Flash ausgefuehrt.
  - Kein serieller Boot-Monitor nach Flash moeglich.
  - Keine echten LED-Hardwaretests ausgefuehrt.
  - Grund: `COM12` konnte aktiv nicht eindeutig als ESP32/Espressif-ROM identifiziert werden.
- Naechster sicherer Schritt:
  - Nutzer soll pruefen, ob wirklich der neue ESP32 an `COM12` haengt.
  - Falls es ein ESP32 mit BOOT-Taste ist: BOOT gedrueckt halten, kurz EN/RESET druecken oder USB neu verbinden, dann BOOT loslassen und esptool-Chip-ID erneut versuchen.
  - Erst nach erfolgreicher Chip-ID/ESP32-Erkennung wird geflasht.

ESP32 COM12 Recheck 2026-08-17:

- Nutzer/Koordinator bat um erneuten aktiven Check nach vermutetem BOOT/RESET-Schritt.
- Ausgefuehrt:
  `.venv-platformio/Scripts/pio.exe pkg exec -p tool-esptoolpy -- esptool.py --chip auto --port COM12 --baud 115200 chip_id`
- Ergebnis:
  - Port: `COM12`
  - esptool v4.11.0
  - Fehler erneut:
    `Failed to connect to Espressif device: Invalid head of packet (0xF8): Possible serial noise or corruption.`
- Ergebnis:
  - Kein Flash ausgefuehrt.
  - Kein serieller Boot-Monitor gestartet.
  - Grund: `COM12` wurde erneut nicht eindeutig als ESP32-Bootloader erkannt.
- Naechste physische Aktion:
  - ESP32 vom USB trennen.
  - BOOT-Taste gedrueckt halten.
  - USB wieder einstecken oder EN/RESET kurz druecken.
  - BOOT weitere 2 Sekunden halten, dann loslassen.
  - Danach Port erneut pruefen; falls ein anderer COM-Port erscheint, diesen verwenden.

ESP32 Mixed WS2812B+APA102 Flash auf COM6 2026-08-17:

- Nutzer/Koordinator nannte `COM6` als richtigen neuen ESP-Port.
- Aktive ESP32-Pruefung:
  - Befehl:
    `.venv-platformio/Scripts/pio.exe pkg exec -p tool-esptoolpy -- esptool.py --chip auto --port COM6 --baud 115200 chip_id`
  - Ergebnis:
    - Chip erkannt: `ESP32-D0WD (revision v1.0)`
    - Features: WiFi, BT, Dual Core, 240 MHz
    - Crystal: 40 MHz
    - MAC: `e8:68:e7:0d:38:a4`
  - Bewertung: `COM6` eindeutig als ESP32 erkannt.
- Upload:
  - Befehl:
    `.venv-platformio/Scripts/pio.exe run -e esp32-wifi-ws2812-apa102-1679 -t upload --upload-port COM6`
  - Ergebnis: bestanden.
  - Flash geschrieben/verifiziert:
    - Bootloader/Partitions/Boot-App und Firmware.
    - Firmware: 1089424 Bytes, komprimiert 644422 Bytes.
    - Hash verifiziert.
  - PlatformIO-Ergebnis:
    `esp32-wifi-ws2812-apa102-1679 SUCCESS 00:02:56.633`
- Serieller Monitor:
  - Erster Monitorlauf nach Upload: keine Ausgabe innerhalb 18 s.
  - Danach per esptool hard reset und erneut Monitor `COM6`, 115200 Baud gestartet.
  - Bootlog verifiziert:
    - `Schrank LED mixed Art-Net controller boot`
    - `Profile: ESP32 WiFi WS2812B GPIO23 + APA102 data=18 clock=19`
    - `WS2812B output configured id=0 pixels=512 data=23`
    - `WS2812B output 0 hardware enabled on data=23 colorOrder=GRB`
    - `APA102 output configured id=1 pixels=1167 data=18 clock=19`
    - `APA102 output 1 hardware enabled on data=18 clock=19`
    - `Connecting WiFi SSID: [PRIVATE WIFI VALUE REMOVED]`
    - `WiFi connected.`
    - `Configured outputs: 2`
    - `IP: 10.0.0.251`
    - Wiederholte Statuszeilen: `frames=0`, `pixels=1679`, Heap ca. 215920-215928.
- Ergebnis:
  - Firmware ist auf ESP32 `COM6` geflasht.
  - Boot und erwartete Output-Konfiguration sind seriell verifiziert.
  - WiFi ist verbunden, aktuelle IP `10.0.0.251`.
- Noch offen:
  - Keine echten LED-Farb-/Signaltests auf den angeschlossenen WS2812B-/APA102-Ketten durchgefuehrt.
  - Keine Art-Net-Pakete an `10.0.0.251` gesendet/verifiziert.
  - Keine optische Nutzerbestaetigung fuer WS2812B `GPIO23` oder APA102 `GPIO18/19`.

ESP32 stromarmer API-Hardwaretest 2026-08-17:

- Ziel:
  - Einfache, kontrollierte Animationen fuer angeschlossene LEDs.
  - Outputs getrennt sichtbar machen.
  - Stromarm bleiben: keine Vollweiss-Flaeche, keine dauerhaft voll eingeschalteten 1679 LEDs.
- Firmware erweitert:
  - Neue Dateien:
    - `firmware\include\HardwareTest.h`
    - `firmware\src\HardwareTest.cpp`
  - `POST /api/test-pattern` startet jetzt eine zeitlich begrenzte Testsequenz.
  - `POST /api/test-pattern` mit Body `{"action":"stop"}` oder `{"action":"off"}` stoppt.
  - `GET /api/test-pattern` liefert Status, z. B. `{"active":true,"phase":"ws2812-red-chase"}`.
  - Testsequenz:
    - ca. 1 s Blackout.
    - WS2812B-only: sparse rotes Lauflicht auf Output 0, `GPIO23`, 512 LEDs.
    - ca. 1 s Blackout.
    - APA102-only: sparse gruenes Lauflicht auf Output 1, `GPIO18=data`, `GPIO19=clock`, 1167 LEDs.
    - ca. 1 s Blackout.
    - kurzer gemischter sparse Test: WS2812B blau, APA102 orange.
    - finaler Blackout, danach automatisch `stopped`.
  - Helligkeit pro aktivem Pixel: Kanalwert 18/255, ca. 7 Prozent; gleichzeitig leuchtet nur ein kleiner Anteil der LEDs.
  - Sicherheitsfix:
    - Start direkt nach Web-Request konnte durch alten Loop-Zeitstempel sofort stoppen; `HardwareTest::update()` behandelt `nowMs < startMs_` jetzt als elapsed 0.
- Lokal verifiziert:
  - `make test NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"`: bestanden.
  - Neuer nativer Test `testHardwareTestSequence`: Start/Stop, WS-Phase, APA-Phase und Frame-Clear verifiziert.
  - PlatformIO-Build:
    `.venv-platformio/Scripts/pio.exe run -e esp32-wifi-ws2812-apa102-1679`: bestanden.
  - Build-Speicher nach finalem Fix:
    - RAM: 59496 Bytes / 327680 Bytes = 18.2 Prozent.
    - Flash: 1084809 Bytes / 1310720 Bytes = 82.8 Prozent.
- Upload:
  - Befehl:
    `.venv-platformio/Scripts/pio.exe run -e esp32-wifi-ws2812-apa102-1679 -t upload --upload-port COM6`
  - Ergebnis: bestanden.
  - ESP32 erkannt auf `COM6`: `ESP32-D0WD`, MAC `e8:68:e7:0d:38:a4`.
  - Firmware-Hash verifiziert.
- Serieller Monitor nach Upload:
  - Boot erfolgreich.
  - Profil:
    `ESP32 WiFi WS2812B GPIO23 + APA102 data=18 clock=19`
  - Outputs:
    - `WS2812B output configured id=0 pixels=512 data=23`
    - `WS2812B output 0 hardware enabled on data=23 colorOrder=GRB`
    - `APA102 output configured id=1 pixels=1167 data=18 clock=19`
    - `APA102 output 1 hardware enabled on data=18 clock=19`
    - `Configured outputs: 2`
  - WiFi verbunden, IP `10.0.0.251`.
- API-Testlauf:
  - `POST http://10.0.0.251/api/test-pattern` gestartet.
  - Phasen per `GET /api/test-pattern` beobachtet:
    - `blackout`
    - `ws2812-red-chase`
    - `apa102-green-chase`
    - `blackout`
    - `mixed-sparse`
    - `final-blackout`
    - danach `stopped`
  - ESP blieb danach per API erreichbar.
  - Status danach:
    - `pixelCount=1679`
    - `universeCount=10`
    - `heapFree=211252`
    - `heapMinFree=205368`
    - letzter `outputTimeUs=34607`
- Noch offen:
  - Optische Nutzerbestaetigung, ob WS2812B-Lauflicht sichtbar war.
  - Optische Nutzerbestaetigung, ob APA102-Lauflicht sichtbar war.
  - Erst nach Nutzerbestaetigung LED-Hardwarefunktion als bestanden markieren.

ESP32 stromarmer Hardwaretest Loop-Modus 2026-08-17:

- Nutzer/Koordinator bat um dauerhaften Loop zum Verdrahten und Beobachten.
- Firmware erweitert:
  - `HardwareTest::startLoop(uint32_t nowMs)` hinzugefuegt.
  - `POST /api/test-pattern` mit `{"action":"loop"}` oder `{"action":"start-loop"}` startet Loop-Modus.
  - `POST /api/test-pattern` mit `{"action":"stop"}` oder `{"action":"off"}` stoppt weiter verlaesslich.
  - `GET /api/test-pattern` meldet nun zusaetzlich `"loop":true/false`.
  - Fuer Profil `LED_PROFILE_WS2812_APA102_1679` startet der Loop beim Boot automatisch.
- Loop-Sequenz:
  - Blackout.
  - WS2812B-only rotes sparse Lauflicht, `GPIO23`, 512 LEDs.
  - Blackout.
  - APA102-only gruenes sparse Lauflicht, `GPIO18=data`, `GPIO19=clock`, 1167 LEDs.
  - Blackout.
  - Mixed sparse: WS2812B blau, APA102 orange.
  - Final Blackout.
  - Danach Wiederholung.
- Sicherheit:
  - Kanalhelligkeit bleibt 18/255, ca. 7 Prozent.
  - Sparse Pixel, kein Vollweiss, keine Flaeche mit allen LEDs an.
- Serial-Logging auf Nutzerwunsch erweitert:
  - Knappe Logzeilen bei Loop-Start, Stop und jedem Phasenwechsel.
  - Prefix: `[hardware-test]`
  - Beispiel:
    - `[hardware-test] loop-starting`
    - `[hardware-test] loop started`
    - `[hardware-test] blackout`
    - `[hardware-test] ws2812-red-chase`
    - `[hardware-test] apa102-green-chase`
    - `[hardware-test] mixed-sparse`
    - `[hardware-test] final-blackout`
  - Kein zusaetzlicher Dauer-Spam pro Frame.
  - Bestehende 1-Hz-Statuszeilen bleiben aktiv.
- Serielle Monitorparameter:
  - Firmware: `Serial.begin(115200)`.
  - Verwendeter Monitorbefehl:
    `.venv-platformio/Scripts/pio.exe device monitor -e esp32-wifi-ws2812-apa102-1679 --port COM6 --baud 115200`
  - PlatformIO-Monitor bestaetigte:
    `Terminal on COM6 | 115200 8-N-1`
  - Keine Flow-Control-Option gesetzt/verwendet.
- Verifikation:
  - `make test NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"`: bestanden.
  - Neuer Loop-Status im nativen Firmware-Test verifiziert.
  - PlatformIO-Build:
    `.venv-platformio/Scripts/pio.exe run -e esp32-wifi-ws2812-apa102-1679`: bestanden.
  - Build-Speicher:
    - RAM: 59496 Bytes / 327680 Bytes = 18.2 Prozent.
    - Flash: 1085401 Bytes / 1310720 Bytes = 82.8 Prozent.
  - Upload:
    `.venv-platformio/Scripts/pio.exe run -e esp32-wifi-ws2812-apa102-1679 -t upload --upload-port COM6`: bestanden.
  - Serieller Monitor nach Upload:
    - Boot erfolgreich.
    - Outputs erneut korrekt:
      - WS2812B Output 0, 512 Pixel, Data `GPIO23`.
      - APA102 Output 1, 1167 Pixel, Data `GPIO18`, Clock `GPIO19`.
    - WiFi verbunden, IP `10.0.0.251`.
    - Loop auto-startete und loggte Phasenwechsel.
    - Zyklus-Wiederholung verifiziert: `final-blackout` -> `blackout` -> `ws2812-red-chase`.
  - API-Verifikation:
    - `GET /api/test-pattern`: `{"active":true,"phase":"ws2812-red-chase","loop":true}`
    - Stop getestet: `POST {"action":"stop"}` -> `{"active":false,"phase":"stopped","loop":false}`
    - Loop wieder gestartet: `POST {"action":"loop"}` -> `{"active":true,"phase":"loop-starting","loop":true}`
    - Danach `GET`: `{"active":true,"phase":"blackout","loop":true}`
- Aktueller Stand:
  - Loop laeuft nach erneutem API-Start wieder auf `10.0.0.251`.
- Noch offen:
  - Optische Nutzerbestaetigung fuer WS2812B und APA102.
  - LED-Hardwarefunktion weiterhin nicht als bestanden markieren, bis der Nutzer sie bestaetigt.

ESP32 20-Sekunden Output-Stats Serial-Logging 2026-08-17:

- Nutzer bat um Serial-Stats alle 20 Sekunden:
  - Wieviele LEDs pro Kanal/Ausgang eingestellt sind.
  - Wieviele Updates es in den letzten 20 Sekunden pro Kanal gab.
  - Frames pro Sekunde.
- Firmware erweitert:
  - `OutputRuntimeStats` in `firmware\include\LedOutputs.h`.
  - `LedOutputManager` zaehlt `totalUpdates` pro Output, wenn ein Output in `show()` tatsaechlich aktualisiert wurde.
  - `main.cpp` loggt alle ca. 20 Sekunden einen kompakten `[output-stats]` Block.
- Serial-Logformat:
  - Controller-Zeile:
    `[output-stats] windowMs=<ms> frames20s=<frames> fps20s=<fps> fps1s=<snapshot> outputs=<count>`
  - Pro Output:
    `[output-stats] id=<id> type=<APA102|WS2812B> leds=<pixelCount> updates20s=<count> updatesPerSec=<rate> totalUpdates=<count>`
- Port-Hinweis waehrend Flash:
  - Nutzer vermutete ESP auf `COM12`.
  - PlatformIO-Portliste:
    - `COM6`: Silicon Labs CP210x USB to UART Bridge, VID:PID `10C4:EA60`.
    - `COM12`: USB Serial Port, VID:PID `04D8:00DF`.
  - `COM12` aktive Pruefung:
    - `esptool.py --port COM12 chip_id`
    - Ergebnis: `Could not open COM12`, PermissionError/Zugriff verweigert.
  - `COM6` aktive Pruefung:
    - `esptool.py --port COM6 chip_id`
    - Ergebnis: `ESP32-D0WD (revision v1.0)`, MAC `e8:68:e7:0d:38:a4`.
  - Deshalb sicher auf `COM6` geflasht.
- Verifikation:
  - Firmware-Core-Test nach Fix: bestanden.
  - Gesamter Testlauf mit Playwright vor finalem Fix: bestanden; ein frueherer Playwright-Lauf hatte einmalig Timeout beim Laden lokaler HTML-Datei, danach erneut bestanden.
  - PlatformIO-Build:
    `.venv-platformio/Scripts/pio.exe run -e esp32-wifi-ws2812-apa102-1679`: bestanden.
  - Build-Speicher:
    - RAM: 59528 Bytes / 327680 Bytes = 18.2 Prozent.
    - Flash: 1087189 Bytes / 1310720 Bytes = 82.9 Prozent.
  - Upload auf `COM6`:
    `.venv-platformio/Scripts/pio.exe run -e esp32-wifi-ws2812-apa102-1679 -t upload --upload-port COM6`: bestanden.
  - Serieller Monitor:
    - Befehl:
      `.venv-platformio/Scripts/pio.exe device monitor -e esp32-wifi-ws2812-apa102-1679 --port COM6 --baud 115200`
    - Monitor bestaetigte:
      `Terminal on COM6 | 115200 8-N-1`
    - Firmware: `Serial.begin(115200)`.
    - Flow-Control: nicht gesetzt/verwendet.
  - Boot/Loop:
    - Boot erfolgreich, WiFi verbunden, IP `10.0.0.251`.
    - Loop auto-startete weiter.
    - Phasenwechsel wurden geloggt.
- Beobachtete 20s-Stats im Monitor:
  - Erster Block:
    - `windowMs=20000`
    - `frames20s=400`
    - `fps20s=20.00`
    - `outputs=2`
    - Output 0 WS2812B: `leds=512`, `updates20s=400`, `updatesPerSec=20.00`, `totalUpdates=400`
    - Output 1 APA102: `leds=1167`, `updates20s=400`, `updatesPerSec=20.00`, `totalUpdates=400`
  - Zweiter Block:
    - `windowMs=20006`
    - `frames20s=400`
    - `fps20s=19.99`
    - Output 0 WS2812B: `updates20s=400`, `updatesPerSec=19.99`, `totalUpdates=800`
    - Output 1 APA102: `updates20s=400`, `updatesPerSec=19.99`, `totalUpdates=800`
- Noch offen:
  - Optische LED-Bestaetigung durch Nutzer.

ESP32 Rot-Kanal Diagnose vorbereitet, Flash blockiert 2026-08-17:

- Nutzer meldete optischen Befund:
  - Er sieht alles bis auf Rot.
  - Keine roten LEDs sichtbar.
- Interpretation:
  - Da der Loop und andere Farben sichtbar sind, ist mindestens ein LED-Ausgang grundsaetzlich aktiv.
  - Ob Rot als Farbkanal elektrisch fehlt, zu dunkel ist oder durch Color-Order/Verkabelung auf eine andere Farbe abgebildet wird, ist noch nicht eindeutig.
- Firmware vorbereitet:
  - Test-Loop in `HardwareTest.cpp` erweitert auf klare Einzelkanal-Diagnose:
    - `ws2812-red-chase`
    - `ws2812-green-chase`
    - `ws2812-blue-chase`
    - Blackout
    - `apa102-red-chase`
    - `apa102-green-chase`
    - `apa102-blue-chase`
    - Blackout
    - `mixed-sparse`
    - `final-blackout`
    - Wiederholung
  - Rote Diagnosephasen nutzen Kanalwert 36/255, weiterhin sparse und stromarm.
- Lokal verifiziert:
  - Firmware-Core-Test: bestanden.
  - PlatformIO-Build `esp32-wifi-ws2812-apa102-1679`: bestanden.
  - Build-Speicher:
    - RAM: 59528 Bytes / 327680 Bytes = 18.2 Prozent.
    - Flash: 1087457 Bytes / 1310720 Bytes = 83.0 Prozent.
- Port-Check vor Flash:
  - `COM12` sichtbar, aber erneut kein ESP32:
    `Failed to connect to Espressif device: Invalid head of packet (0xF8)`.
  - `COM6` ist der bisherige ESP32-Port, aber aktuell gesperrt:
    - `esptool.py --port COM6 chip_id`: `PermissionError(13, Zugriff verweigert)`.
    - Windows `mode COM6`: `Geraet COM6 ist momentan nicht verfuegbar.`
  - Kein offensichtlicher `python`/`pio`/`platformio`/`arduino`/`putty`/`serial` Prozess gefunden.
- Ergebnis:
  - Neue Rot-/RGB-Diagnose-Firmware noch nicht geflasht.
  - Aktuell geflashter Loop laeuft weiterhin ueber WiFi/API:
    `GET http://10.0.0.251/api/test-pattern` meldete `{"active":true,"phase":"mixed-sparse","loop":true}`.
- Naechster Schritt:
  - Nutzer soll alle eventuell offenen seriellen Monitore/Arduino Serial Monitor/PlatformIO Monitor schliessen oder ESP kurz USB trennen und neu verbinden.
  - Danach `COM6` erneut aktiv pruefen und die vorbereitete Diagnose-Firmware flashen.

ESP32 Rot-/RGB-Diagnose geflasht 2026-08-17:

- Nutzer meldete: Port ist frei.
- Aktiver Portcheck:
  - `COM6` antwortete mit `ESP32-D0WD (revision v1.0)`, MAC `e8:68:e7:0d:38:a4`.
  - `COM12` war zuvor sichtbar, aber kein ESP32 (`Invalid head of packet 0xF8`).
- Upload:
  - `.venv-platformio/Scripts/pio.exe run -e esp32-wifi-ws2812-apa102-1679 -t upload --upload-port COM6`
  - Ergebnis: bestanden.
  - Build-Speicher:
    - RAM: 59528 Bytes / 327680 Bytes = 18.2 Prozent.
    - Flash: 1087457 Bytes / 1310720 Bytes = 83.0 Prozent.
- Serieller Monitor:
  - Befehl:
    `.venv-platformio/Scripts/pio.exe device monitor -e esp32-wifi-ws2812-apa102-1679 --port COM6 --baud 115200`
  - Monitorparameter:
    `COM6 | 115200 8-N-1`.
  - Boot erfolgreich, WiFi verbunden, IP `10.0.0.251`.
  - Loop auto-startete.
- Neue Diagnosephasen im Monitor verifiziert:
  - `ws2812-red-chase`
  - `ws2812-green-chase`
  - `ws2812-blue-chase`
  - `apa102-red-chase`
  - `apa102-green-chase`
  - `apa102-blue-chase`
  - `mixed-sparse`
  - `final-blackout`
  - Zyklus wiederholte danach wieder mit `ws2812-red-chase`.
- 20s-Stats weiter stabil:
  - Erster Block: `frames20s=400`, `fps20s=20.00`, beide Outputs `updates20s=400`.
  - Zweiter Block: `frames20s=400`, `fps20s=19.99`, beide Outputs `updates20s=400`.
- Noch offen:
  - Nutzer muss optisch rueckmelden, welche Farben in welchen Phasen sichtbar sind.
  - Besonders wichtig:
    - Bei `ws2812-red-chase`: sieht er rot, dunkel oder eine andere Farbe?
    - Bei `apa102-red-chase`: sieht er rot, dunkel oder eine andere Farbe?
    - Gruen/Blau pro Ausgang im Vergleich.
  - Rot-Fehler noch nicht als behoben markiert.

ESP32 WS2812B RGB optisch bestaetigt 2026-08-17:

- Nutzer meldete:
  - `ws2812-red-chase`: sichtbar.
  - `ws2812-green-chase`: sichtbar.
  - `ws2812-blue-chase`: sichtbar.
- Bewertung:
  - WS2812B Output 0 auf `GPIO23` ist optisch fuer R/G/B grundsaetzlich bestaetigt.
  - Der gemeldete Rot-Ausfall betrifft damit nicht den WS2812B-Ausgang oder ist dort nicht mehr reproduzierbar.
- Noch offen:
  - APA102 Output 1 auf `GPIO18=data`, `GPIO19=clock`: optische Rueckmeldung fuer `apa102-red-chase`, `apa102-green-chase`, `apa102-blue-chase`.

ESP32 Mixed-RGB Diagnose statt uneindeutigem Mixed-Sparse 2026-08-18:

- Nutzer meldete:
  - APA102 Rot/Gruen/Blau offenbar sichtbar.
  - Wenn beide Ausgaenge laufen, wirkt WS2812 gelb, waehrend APA blau ist.
- Einordnung:
  - Der vorherige `mixed-sparse`-Test war absichtlich uneindeutig: WS2812 wurde blau gesendet, APA102 orange.
  - Dadurch war die optische Interpretation fuer "beide gleichzeitig" schwer.
- Firmware angepasst:
  - `mixed-sparse` ersetzt durch klare gleichfarbige Mixed-Phasen:
    - `mixed-red-chase`: WS2812B und APA102 gleichzeitig rot.
    - `mixed-green-chase`: WS2812B und APA102 gleichzeitig gruen.
    - `mixed-blue-chase`: WS2812B und APA102 gleichzeitig blau.
  - Helligkeit bleibt stromarm/sparse:
    - Rot-Diagnose: 36/255.
    - Gruen/Blau: 18/255.
  - Cycle-Laenge auf 59000 ms erweitert.
- Lokal verifiziert:
  - Firmware-Core-Test: bestanden.
  - PlatformIO-Build `esp32-wifi-ws2812-apa102-1679`: bestanden.
  - Build-Speicher:
    - RAM: 59528 Bytes / 327680 Bytes = 18.2 Prozent.
    - Flash: 1087641 Bytes / 1310720 Bytes = 83.0 Prozent.
- Port/Upload:
  - `COM6` aktiv als `ESP32-D0WD (revision v1.0)`, MAC `e8:68:e7:0d:38:a4`, erkannt.
  - Upload auf `COM6`: bestanden.
- Serieller Monitor:
  - `COM6 | 115200 8-N-1`.
  - Boot erfolgreich, WiFi `10.0.0.251`.
  - Neue Mixed-Phasen im Monitor verifiziert:
    - `mixed-red-chase`
    - `mixed-green-chase`
    - `mixed-blue-chase`
    - `final-blackout`
    - Wiederholung ab `ws2812-red-chase`.
  - 20s-Stats weiter stabil:
    - `frames20s=400`
    - `fps20s=19.99`
    - beide Outputs `updates20s=400`
- Noch offen:
  - Optische Rueckmeldung fuer die neuen Mixed-Phasen:
    - Bei `mixed-red-chase`: sehen beide Ausgaenge rot?
    - Bei `mixed-green-chase`: sehen beide Ausgaenge gruen?
    - Bei `mixed-blue-chase`: sehen beide Ausgaenge blau?

ESP32 Mixed-RGB optisch bestaetigt 2026-08-18:

- Nutzer meldete zu den neuen Mixed-Phasen:
  - Alle sind jetzt gleich.
  - `mixed-red-chase`: beide Ausgaenge rot.
  - `mixed-green-chase`: beide Ausgaenge gruen.
  - `mixed-blue-chase`: beide Ausgaenge blau.
- Bewertung:
  - WS2812B Output 0 auf `GPIO23` ist optisch fuer R/G/B bestaetigt.
  - APA102 Output 1 auf `GPIO18=data`, `GPIO19=clock` ist optisch fuer R/G/B bestaetigt.
  - Gleichzeitige Ausgabe auf beiden Ausgaengen ist optisch fuer R/G/B bestaetigt.
  - Der vorherige Eindruck `WS2812 gelb / APA blau` im Mixed-Test war durch den alten uneindeutigen `mixed-sparse`-Test erklaerbar, weil dort absichtlich unterschiedliche Farben gesendet wurden.
- Weiterhin nicht abgedeckt:
  - Langzeitstabilitaet.
  - Volle Helligkeit / hohe Stromlast.
  - Art-Net Live-Input statt internem HardwareTest-Loop.
  - Vollstaendige Mapping-/Layout-Verifikation.

ESP32 Art-Net Live-Test 2026-08-18:

- Ziel:
  - Art-Net Live-Input auf dem ESP32 `10.0.0.251` testen.
  - Interner HardwareTest-Loop vorher stoppen, damit Art-Net verarbeitet wird.
- Vorbereitung:
  - `GET /api/test-pattern`: Loop war aktiv.
  - `POST /api/test-pattern {"action":"stop"}`: Loop erfolgreich gestoppt.
  - Status danach: `active=false`, `phase=stopped`, `loop=false`.
- Erster Live-Sendeversuch:
  - `tools\fastled-sketch-player.js` mit `--pixel-count 1679` im normalen `fit16`-Mode verwendet.
  - Befund: Tool ignoriert `--pixel-count` in diesem Mode und sendete nur 256 Pixel / 2 Universes.
  - Dieser Versuch testete damit nicht den gesamten 1679-Pixel-Bereich und wurde nicht als vollstaendiger Live-Test gewertet.
- Korrigierter Live-Test:
  - Eigener Node-Art-Net-Sender direkt im Terminal.
  - Ziel: `10.0.0.251:6454`.
  - Start-Universe: `0`.
  - Universes: `0-9`.
  - Pixel: `1679`.
  - DMX-Laenge: 510.
  - Stromarm/sparse:
    - Rot: 24/255.
    - Gruen/Blau: 18/255.
    - Nur kleine Lauflicht-Segmente aktiv.
  - Sequenz:
    - WS2812B-only Rot/Gruen/Blau.
    - APA102-only Rot/Gruen/Blau.
    - Beide Ausgaenge gemeinsam Rot/Gruen/Blau.
    - Finaler Blackout.
- Wichtiger Sender-Fix:
  - Paralleles UDP-Burst-Senden ueber 10 Universes erzeugte zwar viele Pakete, aber zu wenige vollstaendige Frames.
  - Danach sequenziell gesendet, mit ca. 2 ms Abstand zwischen Universes und ca. 12 FPS.
- Verifizierte Status-Deltas beim sequenziellen Test:
  - Vorher:
    - `packets=6591`
    - `framesComplete=34`
    - `framesIncomplete=0`
  - Nachher:
    - `packets=9839`
    - `framesComplete=358`
    - `framesIncomplete=0`
    - `fps=12`
    - `packetsPerSecond=126`
  - Delta:
    - `packets=3248`
    - `framesComplete=324`
    - `framesIncomplete=0`
- Bewertung:
  - Art-Net Live-Input an `10.0.0.251` funktioniert lokal/verarbeitungstechnisch.
  - ESP verarbeitet 10 Universes / 1679 Pixel bei sequenzieller Sendung stabil ohne unvollstaendige Frames im Testfenster.
- Noch offen:
  - Optische Nutzerbestaetigung, was waehrend des Art-Net-Live-Tests auf WS2812B und APA102 sichtbar war.
  - Langzeit-Art-Net-Test.
  - Mapping/Layout-Verifikation mit echter Schrankwand- bzw. Zielmapping-Ausgabe.

ESP32 Art-Net Live-Test optisch bestaetigt 2026-08-18:

- Nutzer bat um erneuten Art-Net-Live-Test.
- Ablauf erneut gesendet:
  - HardwareTest-Loop gestoppt.
  - WS2812B-only Rot/Gruen/Blau.
  - APA102-only Rot/Gruen/Blau.
  - Beide Ausgaenge gemeinsam Rot/Gruen/Blau.
  - Blackout.
- Verifizierte Status-Deltas beim erneuten Test:
  - `packets` Delta: 4328.
  - `framesComplete` Delta: 432.
  - `framesIncomplete` Delta: 0.
  - FPS waehrend Test: ca. 12.
- Nutzer bestaetigte optisch:
  - Alles hat geklappt.
  - Beide Ausgaenge wurden einzeln richtig angezeigt.
  - Beide Ausgaenge zusammen wurden richtig angezeigt.
- Bewertung:
  - Art-Net Live-Input auf ESP32 `10.0.0.251` ist fuer WS2812B Output 0 und APA102 Output 1 technisch und optisch bestaetigt.
  - RGB-Farben sind einzeln und gemeinsam korrekt bestaetigt.
- Weiterhin offen:
  - Langzeit-Art-Net-Test.
  - Mapping/Layout-Verifikation.
  - Tests mit hoeherer Helligkeit bzw. realer Stromlast nur vorsichtig und separat.

ESP32 Onboard-Webmenue / Live-Status 2026-08-18:

- Nutzerwunsch:
  - Basic-Konfiguration/Steuerung direkt ueber ein Webmenue am ESP.
  - Liveview bzw. Live-Status im Webmenue.
- Implementiert:
  - Firmware liefert nun direkt eine HTML-Oberflaeche unter:
    - `http://10.0.0.251/`
    - `http://10.0.0.251/live`
  - Webmenue nutzt die vorhandenen ESP-APIs:
    - `GET /api/status`
    - `GET /api/outputs`
    - `GET /api/test-pattern`
    - `POST /api/test-pattern`
    - `POST /api/preview/target`
    - `POST /api/reboot`
  - Anzeige:
    - Pixelzahl, FPS, Art-Net Frames, Heap.
    - Output-Tabelle mit Typ, LED-Anzahl, Startpixel, Data-Pin, Clock-Pin, SPI.
    - Art-Net Universes und Paket-/Frame-Status.
    - Canvas-basierte Live-Statusvorschau fuer Testphase und Art-Net-Aktivitaet.
  - Steuerung:
    - Hardware-Testloop starten.
    - Einmaligen Test starten.
    - Test stoppen, damit Art-Net Live wieder verarbeitet wird.
    - Preview-Ziel Host/Port setzen.
    - ESP per Webmenue rebooten.
- Bewusste Begrenzung:
  - Pins, LED-Zahlen und Output-Typen werden im Webmenue vorerst nur angezeigt, nicht live geaendert.
  - Grund: Die aktuelle Firmware kann Konfigurationsdaten per API im RAM aendern, initialisiert FastLED-Ausgaenge danach aber nicht sicher neu. Runtime-Pin-/Output-Aenderungen waeren fuer die Hardware riskant bzw. irrefuehrend.
  - Die Canvas-Liveview ist eine Status-/Aktivitaetsvorschau aus den ESP-APIs, keine optische Messung der echten LED-Ausgabe.
- Verifikation:
  - Native Firmware-Core-Tests bestanden:
    - `All firmware core tests passed`
  - ESP32-Build `esp32-wifi-ws2812-apa102-1679` bestanden:
    - RAM: 18.2 Prozent.
    - Flash: 83.6 Prozent.
  - COM6 aktiv als ESP32-D0WD revision v1.0, MAC `e8:68:e7:0d:38:a4`, erkannt.
  - Upload auf COM6 bestanden.
  - Serieller Monitor:
    - `COM6 | 115200 8-N-1`.
    - Boot erfolgreich.
    - Profil korrekt: `ESP32 WiFi WS2812B GPIO23 + APA102 data=18 clock=19`.
    - Outputs korrekt geloggt:
      - WS2812B Output 0: 512 LEDs, Data GPIO23, GRB.
      - APA102 Output 1: 1167 LEDs, Data GPIO18, Clock GPIO19.
    - WiFi-IP: `10.0.0.251`.
    - Hardware-Testloop aktiv, ca. 20 FPS.
    - 20s-Stats stabil: beide Outputs je 400 Updates pro 20s.
  - HTTP-Verifikation:
    - `GET http://10.0.0.251/`: HTTP 200, HTML 8071 Bytes.
    - `GET /api/status`: HTTP 200, `pixelCount=1679`, `universeCount=10`, `fps=20`.
    - `GET /api/outputs`: HTTP 200, erwartete WS2812B-/APA102-Konfiguration.
    - `GET /api/test-pattern`: HTTP 200, Loop aktiv.
    - `POST /api/test-pattern {"action":"stop"}`: HTTP 202, danach `active=false`, `phase=stopped`.
    - `POST /api/test-pattern {"action":"loop"}`: HTTP 202, danach Loop wieder aktiv.
- Weiterhin offen:
  - Browser-Sichtpruefung durch Nutzer auf `http://10.0.0.251/`.
  - Falls echte Pixel-fuer-Pixel-Liveview gewuenscht ist: separaten Frame-Snapshot-/Downsample-Endpunkt implementieren und Performance/Heap testen.
  - Sichere Runtime-Neuinitialisierung fuer geaenderte Output-Pins/LED-Zahlen, falls Konfiguration spaeter wirklich editierbar sein soll.

ESP32 Onboard-Webmenue editierbare Runtime-Konfiguration 2026-08-18:

- Nutzerwunsch:
  - Webmenue soll flexibler werden.
  - Pixelzahl, Ausgaenge und Konfiguration sollen ueber die Weboberflaeche aenderbar sein.
- Implementiert:
  - Webmenue um Abschnitt `Konfiguration` erweitert.
  - Editierbar im RAM:
    - Gesamtpixel.
    - Start-Universe.
    - Ziel-FPS als Config-Wert.
    - Pro Output: aktiv/inaktiv, Typ, ID, Startpixel, LED-Zahl, Data-Pin, Clock-Pin, ColorOrder, Reverse, SPI Hz, Start-Universe, Output-FPS.
  - `Apply` sendet die neue Konfiguration an `POST /api/config`.
  - `Neu laden` verwirft lokale Formularaenderungen und liest die ESP-Konfiguration neu.
  - `/api/config` und `/api/outputs` fuehren jetzt nicht mehr nur eine RAM-Zuweisung aus, sondern rufen einen Runtime-Reconfigure-Pfad auf:
    - Hardware-Test wird gestoppt.
    - Aktueller Frame wird schwarz ausgegeben.
    - Konfiguration wird validiert.
    - Logical-/Final-Buffer werden auf neue Pixelzahl resized.
    - Art-Net UniverseAssembler wird passend zu Start-Universe/Universe-Count neu aufgebaut.
    - OutputRouter/LedOutputManager werden neu initialisiert.
    - HardwareTest bekommt die neue Config.
    - Stats-Fenster wird zurueckgesetzt.
  - FastLED-Controller werden mit sicheren Maximalgroessen registriert:
    - WS2812B GPIO23: maximal 600 LEDs.
    - APA102 GPIO18/19: maximal 1200 LEDs.
  - Damit sind Pixelzahl-/Startpixel-/Enable-Aenderungen innerhalb dieser vorbereiteten Kanaele zur Laufzeit moeglich.
- Bewusste Schutzgrenzen:
  - Aktuell akzeptierte echte Hardwarebindungen:
    - WS2812B: Output ID 0, Data GPIO23, ColorOrder GRB, `spiHz=0`, maximal 600 LEDs.
    - APA102: Output ID 1, Data GPIO18, Clock GPIO19, ColorOrder BGR, `spiHz=4000000`, maximal 1200 LEDs.
  - Andere Pins, falsche ColorOrder oder falscher SPI-Speed werden vom ESP mit HTTP 400 abgelehnt.
  - Aenderungen sind nicht persistent; nach Reboot wird wieder das Firmware-Profil geladen. Persistentes Speichern ist bewusst noch offen.
  - Die Art-Net-Verteilung bleibt ein zusammenhaengender logischer Pixelbereich ab `startUniverse`; echte per-Output-Universe-Routing-Logik ist noch nicht separat implementiert.
- Verifikation:
  - Native Firmware-Core-Tests bestanden:
    - `All firmware core tests passed`
  - ESP32-Build `esp32-wifi-ws2812-apa102-1679` bestanden:
    - RAM: 18.2 Prozent.
    - Flash: 84.2 Prozent.
  - COM6 aktiv per esptool als ESP32-D0WD revision v1.0, MAC `e8:68:e7:0d:38:a4`, erkannt.
  - Upload auf COM6 bestanden.
  - Serieller Monitor:
    - `COM6 | 115200 8-N-1`.
    - Boot erfolgreich.
    - WiFi-IP `10.0.0.251`.
    - Output-Config korrekt geloggt:
      - WS2812B Output 0: 512 LEDs, Data GPIO23.
      - APA102 Output 1: 1167 LEDs, Data GPIO18, Clock GPIO19.
    - 20s-Stats stabil: ca. 19.65 FPS, beide Outputs je 393 Updates pro 20s im beobachteten Fenster.
  - HTTP-Verifikation:
    - `GET http://10.0.0.251/`: HTTP 200, HTML 13196 Bytes, Konfigurationsformular enthalten.
    - `GET /api/config`: HTTP 200 mit `pixelCount=1679`, `universeCount=10`, zwei Outputs.
    - `POST /api/config` mit unveraenderter gueltiger Config: HTTP 200.
    - Runtime-Test: `pixelCount=1600` und APA102 `pixelCount=1088` wurde angenommen; danach erfolgreich auf `pixelCount=1679`, APA102 `1167` zurueckgestellt.
    - Fehlerfall falsche WS2812B ColorOrder `RGB`: HTTP 400, Config blieb unveraendert.
    - Fehlerfall falscher WS2812B Data-Pin `22`: HTTP 400, Config blieb unveraendert.
  - Endzustand nach Tests:
    - Config wiederhergestellt auf 1679 Pixel.
    - HardwareTest aktuell gestoppt (`active=false`, `phase=stopped`), dadurch Art-Net Live frei.
- Weiterhin offen:
  - Browser-Sichtpruefung des Formulars durch Nutzer.
  - Persistentes Speichern/Laden der Konfiguration im ESP-Flash.
  - Echte freie Pin-/Typ-Auswahl wuerde weitere compile-time FastLED-Controller oder eine andere Treiberabstraktion erfordern.
  - Echte Pixel-fuer-Pixel-Liveview per Frame-Snapshot-/Downsample-Endpunkt ist noch nicht implementiert.

ESP32 Webmenue WS2812-Reload-Fix 2026-08-18:

- Nutzerbefund:
  - WS2812 LED-Zahl konnte im Webmenue angewendet werden, erschien nach `Neu laden` aber wieder als urspruenglicher Wert.
- Analyse:
  - Direkter API-Test zeigte: `POST /api/config` speichert die geaenderte WS2812-LED-Zahl im laufenden ESP-RAM korrekt.
  - Beispieltest: WS2812 `pixelCount=400` wurde angenommen und danach von `GET /api/config` sowie `GET /api/outputs` korrekt zurueckgegeben.
  - Problem lag damit in der Weboberflaeche/Reload-Logik, nicht im Runtime-Reconfigure-Pfad.
- Fix:
  - Webmenue rendert nach `Apply` jetzt direkt aus der bestaetigten Server-Antwort von `POST /api/config`.
  - Button `Neu laden` liest nun explizit `GET /api/config` und rendert daraus, statt nur den normalen Status-Polling-Zyklus zu triggern.
- Verifikation:
  - Native Firmware-Core-Tests bestanden: `All firmware core tests passed`.
  - ESP32-Build `esp32-wifi-ws2812-apa102-1679` bestanden.
  - COM6 aktiv als ESP32-D0WD erkannt, MAC `e8:68:e7:0d:38:a4`.
  - Upload auf COM6 bestanden.
  - `GET http://10.0.0.251/` liefert neue Seite mit `reloadConfigForm`.
  - Runtime-Test nach Flash:
    - WS2812 `pixelCount=400` per API angewendet.
    - Direktes `GET /api/config` zeigte WS2812 `pixelCount=400`.
    - Danach wieder auf WS2812 `pixelCount=512` zurueckgestellt.
- Endzustand:
  - Config wiederhergestellt auf WS2812 `512`, APA102 `1167`, Gesamt `1679` Pixel.

ESP32 Webmenue Auto-Layout fuer groessere WS2812-Zaehlen 2026-08-18:

- Nutzerbefund:
  - Verkleinern der WS2812-LED-Zahl funktionierte.
  - Vergroessern, z. B. WS2812 von 512 auf 712, funktionierte nicht, weil dabei auch Gesamtpixel, APA102-Startpixel und Universe-Anzahl angepasst werden muessen.
- Ursache:
  - Firmware hatte WS2812 GPIO23 bisher auf maximal 600 LEDs begrenzt.
  - Webformular uebernahm einzelne Output-Laengen, zog aber die fortlaufende Layout-Geometrie nicht automatisch nach.
  - Bei WS2812 712 muss APA102 von Startpixel 512 auf Startpixel 712 verschoben werden; Gesamtpixel werden 712 + 1167 = 1879; Art-Net Universes werden 12.
- Implementiert:
  - WS2812 GPIO23 Runtime-Maximum von 600 auf 1024 LEDs erhoeht.
  - Gesamtpixel Runtime-Sicherheitslimit von 1800 auf 2400 erhoeht.
  - Webmenue-Maximum fuer Gesamtpixel auf 2400 erhoeht.
  - Button `Auto Layout` hinzugefuegt.
  - Auto Layout setzt aktive Outputs fortlaufend:
    - Output 0 startet bei Pixel 0.
    - Output 1 startet direkt nach Output 0.
    - Gesamtpixel = letztes belegtes Pixelende.
    - Universe-Anzahl = ceil(Gesamtpixel / 170).
    - Output-Start-Universe wird aus Startpixel / 170 abgeleitet.
  - `Apply` nutzt diese Auto-Layout-Normalisierung automatisch auch ohne vorherigen Klick auf `Auto Layout`.
- Verifikation:
  - Native Firmware-Core-Tests bestanden: `All firmware core tests passed`.
  - ESP32-Build `esp32-wifi-ws2812-apa102-1679` bestanden:
    - RAM: 18.6 Prozent.
    - Flash: 84.3 Prozent.
  - COM6 aktiv als ESP32-D0WD erkannt, MAC `e8:68:e7:0d:38:a4`.
  - Upload auf COM6 bestanden.
  - `GET http://10.0.0.251/` liefert neue Seite mit `Auto Layout`, Limit 1024/2400 und `normalizeLayout`.
  - Konkreter API-Test bestanden:
    - WS2812 `pixelCount=712`.
    - APA102 `startPixel=712`, `pixelCount=1167`.
    - Gesamt `pixelCount=1879`.
    - `universeCount=12`.
    - ESP nahm die Config per `POST /api/config` an und gab sie per `GET /api/config` unveraendert zurueck.
  - Danach wieder auf Ausgangszustand zurueckgestellt:
    - WS2812 `512`.
    - APA102 `startPixel=512`, `1167`.
    - Gesamt `1679`, `universeCount=10`.
- Hinweis:
  - Groessere WS2812-Zahlen koennen mehr Strom ziehen und laengere WS2812-Framezeiten erzeugen; optische/Power-Tests separat vorsichtig durchfuehren.

Abschluss-Dokumentation, automatische Tests und GitHub-Vorbereitung 2026-08-18:

- Dokumentation erstellt/aktualisiert:
  - `README.md` mit aktuellem ESP32-Profil, Webmenue und Build-Check.
  - `docs/CODE_MAP.md` mit Moduluebersicht und Datenflussdiagramm.
  - `docs/ESP32_WEB_MENU_RUNTIME_CONFIG.md` mit Webmenue, Runtime-Konfig, API, Auto-Layout, TouchDesigner-Art-Net-Werten und Mermaid-Diagrammen.
  - `docs/BUILD_AND_TEST_CHECKS.md` mit reproduzierbaren Test-/Build-/Flash-/Screenshot-Kommandos.
- Screenshots erstellt:
  - `docs/assets/screenshots/esp-web-menu-runtime-config.png`
  - `docs/assets/screenshots/desktop-web-ui-overview.png`
  - Erzeugbar ueber `tools/capture-doc-screenshots.js`.
- Automatische Tests erweitert:
  - `tests/test_firmware_core.cpp` prueft jetzt zusaetzlich:
    - WebApi JSON-Felder fuer ColorOrder/StartUniverse.
    - Embedded ESP-Webmenue enthaelt Auto Layout.
    - Runtime-Hardwarevalidierung akzeptiert WS2812 712 + APA102 1167 = 1879 Pixel / 12 Universes.
    - Falsche Pins/Farbordnung und Limits werden abgelehnt.
  - `tests/test_esp_web_menu_contract.js` prueft:
    - Webmenue-Source enthaelt Auto Layout und Runtime-Limits.
    - Auto-Layout-Rechnung fuer 712 + 1167 = 1879 / 12 Universes.
  - `Makefile` erweitert:
    - `make firmware-build`
    - `make check`
- Vollstaendiger automatischer Check bestanden:
  - Befehl:
    - `NODE_PATH="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules" make check NODE="/mnt/c/Users/jimmy/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe"`
  - Bestandene Teile:
    - Native Firmware-Core-Tests.
    - Mapping-Schema-Test.
    - Web-Calculation-Test.
    - Processing-Protokoll-Test.
    - Processing-Visualizer-Test.
    - Hardware-Benchmark-Tool-Test.
    - ESP-Webmenue-Contract-Test.
    - Playwright Web-UI-Test.
    - PlatformIO Build `esp32-wifi-ws2812-apa102-1679`.
  - PlatformIO Build-Ergebnis:
    - RAM: 18.6 Prozent.
    - Flash: 84.3 Prozent.
- GitHub-Vorbereitung:
  - `gh` installiert und authentifiziert fuer `JimmyBlunt`.
  - Remote: `JimmyBlunt/Art-Net-LEDController`, Default-Branch `main`.
  - Secrets/temporare Artefakte ausgeschlossen:
    - `firmware/include/WifiSecrets.h`
    - `.pio/`
    - `.venv-platformio/`
    - `build/`
    - Monitor-/Runtime-Logs und PID-Dateien.
  - GeoPix wurde nicht als verschachteltes Git-Repo/Submodule gestaged, da das sonst nicht reproduzierbar waere.
  - GitHub Push:
    - Commit `d32abed` (`Document ESP runtime config and add build checks`) erfolgreich nach `origin/main` gepusht.

Browsergesteuerte ESP-Webmenue-Tests 2026-08-18:

- Nutzerpraezisierung:
  - Tests sollen browsergesteuert sein und echte Oberflaechen-/GUI-Elemente bedienen, wie ein Nutzer sie normalerweise benutzt.
- Umsetzung:
  - Der reine ESP-Webmenue-Contract-Test wurde durch `tests/test_esp_web_menu_playwright.js` ersetzt.
  - Der Test extrahiert die eingebettete ESP-Webseite aus `firmware/src/WebApi.cpp`.
  - Ein lokaler HTTP-Server serviert diese Seite, damit der Browser wie bei `http://10.0.0.251/` arbeitet.
  - ESP-API-Endpunkte werden mit Playwright gemockt.
- Browser-Funktionen, die wirklich bedient werden:
  - Konfigurationsformular laden.
  - WS2812 LED-Zahl von 512 auf 712 setzen.
  - Button `Auto Layout` klicken.
  - Pruefen, dass Gesamtpixel 1879, Universes 12 und APA102 Startpixel 712 werden.
  - Button `Apply` klicken und gepostete `/api/config`-Nutzlast pruefen.
  - Button `Neu laden` klicken und pruefen, dass die Server-Config wieder im Formular steht.
  - Testbuttons `Loop` und `Stop` klicken.
  - Preview-Ziel Host/Port eingeben und `Setzen` klicken.
  - Mobile Viewport pruefen, ob `Apply` sichtbar bleibt.
- Build-Integration:
  - `Makefile` ruft jetzt `tests/test_esp_web_menu_playwright.js` in `make test`/`make check` auf.
  - Doku aktualisiert:
    - `docs/BUILD_AND_TEST_CHECKS.md`
    - `docs/CODE_MAP.md`
- Verifikation:
  - Einzeltest bestanden:
    - `ESP web menu Playwright tests passed`
  - Voller Check bestanden:
    - Native Firmware-Core-Tests.
    - Mapping/Web/Processing/Benchmark-JS-Tests.
    - ESP-Webmenue-Playwright-Test.
    - Desktop-Web-UI-Playwright-Test.
    - PlatformIO Build `esp32-wifi-ws2812-apa102-1679`.

Flexible ESP32-WROOM-Ausgaenge und Konfigurationsbackup 2026-09-11:

- Endgueltiges Hardwareprofil `esp32-wroom-flex-8ws-2apa`:
  - WS2812B wahlweise auf GPIO 25, 26, 27, 14, 19, 18, 5 oder 17.
  - APA102 wahlweise Data/Clock GPIO 32/33 oder GPIO 26/27.
  - GPIO 26/27 duerfen alternativ einzeln fuer WS2812B oder gemeinsam fuer APA102 verwendet werden; doppelte GPIO-Belegung wird abgewiesen.
  - GPIO 5 wird in der UI als Boot-Strapping-Pin gekennzeichnet; GPIO 34/35 werden nicht als Ausgaenge angeboten.
- Systemseite um JSON-Backup/Restore erweitert:
  - Exportiert LED-Typen, Pins, LED-Zahlen, Pixelbereiche, Art-Net-Universes und Ziel-FPS.
  - WLAN-Zugangsdaten, Testmodus und Laufzeitstatistiken werden nicht exportiert.
  - Restore prueft Dateiformat, Version, Zielprofil, erlaubte Pinpaare und GPIO-Kollisionen und speichert danach dauerhaft.
- Verifikation bestanden:
  - Native Extension-Board-Profiltests.
  - Browsergesteuerter Export-/Restore-Test.
  - Embedded-Web-UI neu erzeugt.
  - PlatformIO-Build `esp32-wifi-extensionboard`: RAM 18.9 Prozent, Flash 89.1 Prozent.
- Vor dem Flashen wurde Controller `10.0.0.249` ueber `/api/config` gesichert:
  - Backup: `reports/controller-backup-10.0.0.249-pre-flex-2026-09-11.json`.
  - 2631 Pixel, vier WS2812B-Ausgaenge GPIO 26/27/25/14, Art-Net-Universes 120 bis 137.
  - Diese Belegung ist mit dem neuen Profil kompatibel und kann unveraendert wiederhergestellt werden.
- Flash und Restore abgeschlossen:
  - COM6 wurde vor dem Schreiben als ESP32-D0WD-V3, MAC `d4:e9:f4:b4:b5:b0`, identifiziert.
  - Der serielle Bootlog bestaetigte vor dem Flash exakt die vier zuvor von `10.0.0.249` gesicherten Ausgaenge; damit war die Zuordnung des Backups zum Zielboard eindeutig.
  - `esp32-wifi-extensionboard` erfolgreich auf COM6 geflasht; Flash-Hashes wurden verifiziert.
  - NVS-Konfiguration nach Neustart automatisch wiederhergestellt und ueber `/api/config` mit dem Backup abgeglichen.
  - `/api/config/save` bestaetigt `saved:true, matches:true`.
  - Neues Hardwareprofil aktiv: `esp32-wroom-flex-8ws-2apa`.
  - Ein Brownout trat beim ersten WiFi-Start auf; der anschliessende vorgesehene USB-Recovery-Start war stabil.
  - WiFi und Web-API danach stabil auf `10.0.0.249`, `safeMode=no`.
  - Vom echten ESP ausgelieferte Weboberflaeche enthaelt beide neuen Backup-/Restore-Schaltflaechen.

Flex-Ausgangslimit 1200 Pixel und erstes OTA-Update 2026-09-11:

- Flexibles Profil erlaubt jetzt pro WS2812B- oder APA102-Ausgang 1 bis 1200 Pixel statt maximal 1024.
- Dynamische FastLED-Ausgangspuffer wurden entsprechend auf 1200 Pixel erweitert.
- Weboberflaeche erlaubt 1200 und weist bei 1122 WS2812 auf ca. 33,7 ms Signaldauer sowie praktisch hoechstens etwa 29 FPS hin; empfohlen sind 25 FPS mit Reserve.
- Verifikation bestanden:
  - Native Profilvalidierung akzeptiert 1122 und lehnt 1201 Pixel ab.
  - Browser-Test bestaetigt neues Eingabelimit und Timing-Hinweis.
  - PlatformIO-Build bestanden: RAM 18.9 Prozent, Flash 89.2 Prozent.
- Vor dem Update wurde die zwischenzeitlich erweiterte echte Konfiguration erneut gesichert:
  - `reports/controller-backup-10.0.0.249-pre-ota-1200-limit-2026-09-11.json`.
  - 4007 Pixel auf sechs WS2812B-Ausgaengen.
- USB-Upload auf COM6 startete nicht, weil der Port vor dem Schreiben verschwand; es wurden dabei keine Flashdaten uebertragen.
- Firmware danach erfolgreich ueber den vorhandenen WLAN-Updater auf `10.0.0.249` installiert.
- Nach OTA-Neustart waren alle sechs Ausgaenge und 4007 Pixel unveraendert vorhanden.
- Echter API-Test akzeptierte temporaer 1122 Pixel auf Output 5 und 4105 Gesamtpixel.
- Anschliessend wurde die urspruengliche 4007-Pixel-Konfiguration wiederhergestellt und dauerhaft gespeichert (`saved=true`, `matches=true`).

Ersatz-ESP Wiederherstellung 2026-09-11:

- Ersatzboard auf COM6 eindeutig erkannt: ESP32-D0WD-V3 rev 3.0, MAC `e0:5a:1b:6c:9c:e8`.
- Firmwareprofil `esp32-wifi-extensionboard` erfolgreich per USB auf COM6 geflasht; Schreibvorgang und Hashpruefung bestanden.
- Board stabil per WLAN unter `10.0.0.248` erreichbar; OTA-Endpunkt `/update` liefert HTTP 200.
- Neuestes Backup `reports/controller-backup-10.0.0.249-pre-ota-1200-limit-2026-09-11.json` auf das Ersatzboard eingespielt.
- Wiederhergestellt: 4007 Pixel, 28 belegte Universes, sechs WS2812B-Ausgaenge GPIO 26/27/25/14/19/17.
- Dauerhafte Speicherung und Uebereinstimmung bestaetigt: `saved=true`, `matches=true`.
- Web-API blieb im anschliessenden Stabilitaetscheck erreichbar; kein Brownout des Ersatzboards beobachtet.

Pinout-Korrektur Ersatzboard 2026-09-11:

- Foto `C:\Temp\ESP-Pins.jpeg` erneut gegen das Ersatzboard abgeglichen; GPIO33 ist auf dieser Variante nicht als nutzbarer Header-Pin vorhanden.
- Nicht nutzbares APA102-Paar GPIO32/33 entfernt.
- Bestaetigte APA102-Auswahl ist jetzt:
  - Data GPIO18 / Clock GPIO5.
  - Data GPIO26 / Clock GPIO27.
- Dieselben Pins bleiben alternativ als WS2812B-Ausgaenge verfuegbar; GPIO-Kollisionspruefung verhindert gleichzeitige Nutzung.
- Native Profiltests, Browser-Test und PlatformIO-Build bestanden.
- Korrigierte Firmware erfolgreich per WLAN auf `10.0.0.248` installiert.
- Nach OTA blieben sechs Ausgaenge, 4007 Pixel und alle Universe-Zuordnungen unveraendert; Speicherstatus `saved=true`, `matches=true`.
- Echte ESP-Weboberflaeche zeigt das korrigierte Paar `G18 / Clock G5`.

## LED-Farbreihenfolge auf ESP .251 · 15.09.2026

Flex8-Firmware per WLAN aktualisiert. Unter Ausgänge ist je Ausgang eine der sechs
RGB-Farbreihenfolgen auswählbar; Anwenden übernimmt sie sofort, dauerhaftes Speichern
erhält sie nach Neustart. Native Farb-/Flex8-Tests und beide Browserprüfungen bestanden.
Alle sechs Werte am Gerät per API geprüft; vorhandene APA102-Konfiguration
(GPIO18/19, 256 Pixel, U156, BGR) wiederhergestellt. Keine optische Abnahme.
Details: [LED_COLOR_ORDER.md](docs/LED_COLOR_ORDER.md).
Build-/OTA-/Prüfbelege: `C:/Temp/esp-recovery-20260915/color-order`.
