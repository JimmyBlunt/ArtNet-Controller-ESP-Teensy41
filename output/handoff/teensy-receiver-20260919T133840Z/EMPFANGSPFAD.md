# Teensy: Empfangspfad und Frame-Vollständigkeit

Stand: 19.09.2026. Quellen entsprechen Commit d5d828f; siehe diagnostics/source-origin.json. Die laufende Firmware meldet orbital-port-tests-20260913. Die Zuordnung zum lokalen Build ergibt sich aus dieser Kennung; ein binärer Firmwarevergleich wurde nicht durchgeführt.

## Aktiver Code

Aktive PlatformIO-Umgebung: teensy41_octo_web_rx32 (firmware/teensy41_artnet/platformio.ini:64). Diese baut src/web_main.cpp und src/web_http.cpp. Maßgeblich sind include/runtime_receiver.h und include/artnet_run_policy.h. Die ebenfalls enthaltenen main.cpp, receiver_core.h und octo_identify.cpp gehören zu anderen Varianten und sind NICHT der hier untersuchte aktive Empfangspfad. Alle folgenden Dateipfade sind relativ zu firmware/teensy41_artnet/.

## Aktuell erwartete Universes

Live aus GET /api/config und /api/status vom 19.09.2026, 13:38:40 UTC. Universe-Nummern sind die im Code verwendeten 0-basierten ArtDmx-Adressen.

| Ausgang | Teensy-Pin | Aktive LEDs | Universes | Mindest-Payload letztes Universe |
|---|---:|---:|---|---:|
| OUT1 | 2 | 203 | 120–121 | U121: 100 Bytes (99 genutzt) |
| OUT2 | 14 | 738 | 122–126 | U126: 174 Bytes |
| OUT3 | 7 | 880 | 127–132 | U132: 90 Bytes |
| OUT4 | 8 | 810 | 133–137 | U137: 390 Bytes |
| OUT5 | 6 | 352 | 139–141 | U141: 36 Bytes |
| OUT6 | 20 | 610 | 142–145 | U145: 300 Bytes |
| OUT7 | 21 | 512 | 146–149 | U149: 6 Bytes |
| OUT8 | 5 | deaktiviert | keine | gespeicherte 509 LEDs ab U150 sind inaktiv |

Summe: 4105 aktive LEDs, 29 Universes. Erwartet wird genau die Vereinigung 120–137 und 139–149. Universe 138 sowie 150–152 gehören nicht dazu. Volle Universes benötigen mindestens 510 Bytes. Siehe auch diagnostics/expected-universes.csv.

WICHTIG: OUT6 steht aktuell auf 610 LEDs, nicht auf den zuvor gewünschten 536. U145 benötigt daher 300 statt 78 Bytes. artnet_profile_match=false bestätigt eine Abweichung vom kompilierten Standardprofil; dessen Name ESP4031_SPLIT_20260913 ist keine verlässliche Beschreibung der gespeicherten Konfiguration. Die aktuelle Konfiguration wurde hier nicht verändert.

## Antworten auf die zehn Fragen

1. **Vollständiger Frame:** Für jede konfigurierte Route eines aktivierten Ausgangs muss ein gültiges Paket im selben Frame-Kandidaten eingetroffen sein. Aktuell sind das alle 29 oben genannten Universes. Erst mask_ == expected_ veröffentlicht einen vollständigen Frame. Quelle: include/runtime_receiver.h:20 (Routen), :91–100 (Sammeln/Fertigstellung).

2. **Reservierte/unbenutzte Universes:** Keine pauschale Reservierung wird erwartet. ceil(LED-Anzahl/170) bestimmt die Routen je aktivem Port. Deaktivierte Ports werden mit pixels=0 konfiguriert. Gültige ArtDmx-Pakete außerhalb der Routen zählen als ignored. Ein aktivierter Port wird dagegen auch dann erwartet, wenn keine LED-Kette physisch angeschlossen ist oder alle Pixel schwarz sein sollen. Quelle: src/web_main.cpp:80–90; include/runtime_receiver.h:26–35 und :64.

3. **Payload-Länge:** Kurze letzte Pakete sind erlaubt, sofern sie mindestens alle dort konfigurierten RGB-Bytes enthalten. Die deklarierte Payload muss gerade sein, zwischen 2 und 512 Bytes liegen, und die tatsächliche UDP-Länge muss exakt 18 + Payload betragen. Deshalb muss U121 mit 99 benötigten RGB-Bytes mindestens 100 Bytes Payload haben. Auf 510 oder 512 Bytes aufgefüllte Pakete werden ebenfalls akzeptiert; kopiert wird nur die erforderliche Datenmenge. Quelle: include/runtime_receiver.h:33–35 und :55–66 sowie :92.

4. **Sequence:** Der Receiver verwendet EINEN gemeinsamen Sequence-Zustand für alle Universes und Absender. Bei Sequence 1–255 müssen alle Universes des Kandidaten dieselbe Sequence tragen. Eine neuere Sequence verwirft einen vorhandenen Teilframe. Die Vorwärtsdistanz wird modulo 255 berechnet; 1–127 gilt als neuer, >127 als stale. 255→1 ist zulässig. Doppelte Universes derselben Sequence werden ignoriert; nach Fertigstellung ist diese Sequence versiegelt. Sequence 0 deaktiviert die Sequence-Sortierung: Es werden Universes bis zur vollständigen Maske gesammelt. Ein doppeltes Universe vor Fertigstellung verwirft dann den bisherigen Teilframe und beginnt mit diesem Paket neu. Ein Wechsel zwischen Sequence 0 und ungleich 0 verwirft ebenfalls einen vorhandenen Teilframe. Nach mehr als 1000 ms ohne akzeptiertes Paket wird der Sequence-Zustand zurückgesetzt. Eine pro Paket hochgezählte Sequence kann so die Fertigstellung verhindern. Absender-IP wird beim ingest-Aufruf nicht übergeben. Quelle: include/runtime_receiver.h:68–89, :106–108; src/web_main.cpp:454.

5. **Unvollständig-Zähler:** abandon() erhöht incomplete genau dann, wenn die Sammelmaske nicht leer ist. Auslöser sind Timeout, eine akzeptierte neuere Sequence, Wechsel zwischen Sequence-Modi und ein doppeltes Universe bei Sequence 0. Es wird pro aufgegebenem Teilframe gezählt, nicht pro fehlendem Universe. rejected-Pakete erhöhen diesen Zähler nicht unmittelbar. clear() etwa bei Zustandswechsel löscht ohne incomplete-Erhöhung. Quelle: include/runtime_receiver.h:68–89, :106–110 und :122.

6. **LED-/DMA-Ausgabe:** Ein vollständiger Frame wird zunächst gepuffert. Die Hauptschleife gibt ihn im Zustand RunningArtNet aus, wenn die Run-Policy ActionRender erlaubt, der vorherige DMA-Transfer einschließlich Schutzzeiten frei und der Ausgabezeitpunkt fällig ist. renderArtNet() konsumiert receiver.take(), kopiert die Daten auf die Ports und ruft über submit(false) FastLED.show() auf. Aktuell sind 30 FPS eingestellt; die Periode beträgt 33334 µs. Der längste Port mit 880 LEDs ergibt zusätzlich 26700 µs Wire-Guard. transferReady() beobachtet ObjectFLED-DMA und wartet nach beobachtetem DMA-Ende mindestens 300 µs Latch sowie insgesamt den Wire-Guard. Erst dann steigt der Softwarezähler dma_completed. Tests und Blackout haben eigene Ausgabewege. Quelle: src/web_main.cpp:92–99, :135–165, :472–494; include/artnet_run_policy.h:13–26.

7. **Timeout:** Ein noch unvollständiger Kandidat wird bei strikt mehr als 100 ms seit seinem ersten akzeptierten Paket verworfen. Spätere Pakete verlängern dieses Fenster nicht. expire() läuft im Empfangspfad und in der Hauptschleife. Daneben existiert die oben beschriebene 1000-ms-Sequence-Rücksetzung. Die Run-Policy wertet mehr als 1000 ms ohne vollständigen Frame als Datenverlust und fordert nach zuvor laufender Ausgabe einen Blackout an. Das ist eine andere Bedingung als der 100-ms-Teilframe-Timeout. Quelle: include/runtime_receiver.h:91, :106–108; src/web_main.cpp:457 und :462–470; include/artnet_run_policy.h.

8. **Pro Port oder global:** Controllerweit. Es gibt eine gemeinsame Maske, einen gemeinsamen Sequence-Zustand und einen vollständigen Puffer für alle aktiven Ports. Ein fehlendes oder zu kurzes erwartetes Universe eines einzigen Ports verhindert die Freigabe des gesamten Kandidaten. Quelle: include/runtime_receiver.h:95–99 und :112–122.

9. **ArtSync:** Wird nicht unterstützt. ingest() akzeptiert nur Opcode 0x5000 (ArtDmx); ArtSync 0x5200 fällt unter rejected und löst keine Ausgabe aus. Quelle: include/runtime_receiver.h:55–56.

10. **Feste Maske/Bereich:** Keine fest codierte Erwartung 120–152. configure() erzeugt die Routen aus der aktiven Konfiguration und expected_ dynamisch als (1ULL << routeCount) - 1, mit Sonderfall für 64 Routen. Aktuell 29 Routen → 0x1FFFFFFF. Die Bits stehen für Routenindizes, nicht direkt für Universe-Nummern. Quelle: include/runtime_receiver.h:20–43.

## Bedeutung für „890 pkt/s, aber DMA/s = 0“

Die Paketanzahl steigt vor jeglicher Validierung und enthält daher auch rejected und ignored. Bei 29 erwarteten Universes und 30 FPS wären nominal 870 passende ArtDmx-Pakete/s nötig; etwa 890 Pakete/s allein belegen weder vollständige Universe-Abdeckung noch passende Payload-Längen, Sequence oder Timing.

Zwei konkrete Prüfpunkte sind U145 (aktuell mindestens 300 Bytes) und die controllerweite Sequence. Ob der reale Sender hier abweicht, wurde ohne Paketmitschnitt nicht festgestellt. Insbesondere wäre eine weiterhin auf 536 LEDs zugeschnittene kurze U145-Payload von 78 Bytes zu klein; wenn der Sender hingegen alle Universes mit 510 Bytes auffüllt, verursacht diese Längenänderung keine Zurückweisung.

Ein lokaler C++-Test gegen den unveränderten runtime_receiver.h bestätigt:
- alle 29 Universes, korrekte Mindestlängen, gemeinsame Sequence: 1 vollständiger Frame;
- U145 mit nur 78 Bytes: 1 rejected, 0 vollständige Frames, danach 1 incomplete;
- U149 fehlt: 0 vollständige Frames, danach 1 incomplete;
- Sequence erhöht sich bei jedem Paket: 0 vollständige Frames;
- zusätzlich U138 und U150–152: 4 ignored, trotzdem 1 vollständiger Frame;
- alle erwarteten Universes mit Sequence 0: 1 vollständiger Frame.

Quellen und Ergebnisse: diagnostics/frame_probe.cpp und diagnostics/frame-probe-results.txt. Dieser Test lief nur auf dem Host; er ist kein Mitschnitt des Senders und kein Test der Hardware-DMA.

Beim lesenden Live-Snapshot lagen die kumulativen Zähler bei 48259 Paketen, 626 vollständigen Frames, 1350 rejected, 2505 ignored, 1950 incomplete und 627 DMA-Abschlüssen. Im anschließenden 2-Sekunden-Fenster änderte sich keiner dieser Zähler: Zu diesem Zeitpunkt wurde kein eingehender Verkehr beobachtet. Die Firmware meldete artnet_waiting=true und black_latched=true. Das frühere Symptom mit 890 pkt/s war daher während dieser Abfrage nicht reproduzierbar. Die 627 kumulativen DMA-Abschlüsse zeigen frühere Abschlüsse, sagen aber nichts über die aktuelle Rate oder die sichtbare Funktion jeder LED aus.

Für den nächsten Vergleich sollten unter laufender Zuspielung Universe, Payload-Länge, Sequence und Ankunftszeit erfasst und parallel Deltas von artnet_complete, rejected, ignored, incomplete, stale, duplicates, frames_submitted und dma_completed verglichen werden. Bleibt artnet_complete bei 0, liegt die fehlende Freigabe vor dem DMA-Pfad; steigt complete ohne submit, sind Run-Policy, Zustand und Transferfreigabe zu untersuchen.
