# Teensy 4.1: Art-Net über natives Ethernet und FastLED

Die Firmware ist ein eigenständiger Testcontroller. Sie ersetzt keine ESP-Binärdatei und verwendet weder OctoWS2811 noch eine Octo-Platine. FastLED 3.10.4 wird mit seinem ObjectFLED-Backend gebaut. Ein Hardwareergebnis von 30 FPS ist erst nach USB-, Ethernet- und LED-Messung belegt.

## Reproduzierbarer Build

`./build.ps1 -Python <python.exe> -NativeTests` baut zwei Umgebungen. Ein isolierter Python-Ordner liegt unter `.toolchain`; der dedizierte PlatformIO-Paketcache liegt wegen der Windows-Pfadlängengrenze unter `C:/codex-tools/teensy41-pio`. Keine ESP-Toolchain wird verändert. Buildversionen: PlatformIO 6.1.18, Teensy-Plattform 6.0.0, FastLED 3.10.4, QNEthernet 0.37.0. Native Protokolltests verwenden Zig 0.15.2.

- `teensy41_safe`: USB-Serial und Ethernet-Empfang; keinerlei LED-Pininitialisierung. `ARM` bleibt wirkungslos. Dieses Image eignet sich zur ersten Geräteidentifikation.
- `teensy41_driver_compile`: sieben FastLED-Controller werden als Quellcode instanziiert. Die Fixture-Pins sind ausschließlich Compiler-Testwerte, werden im Programmablauf nie initialisiert und sind keine Verdrahtungsvorgabe.
- `teensy41_hardware`: erfordert `include/hardware_pins.h` mit sieben geprüften Pinbelegungen. Das Beispiel bricht absichtlich mit einer klaren Fehlermeldung ab. Auch dieser Build initialisiert GPIO/FastLED erst bei `ARM`; USB und Ethernet stehen vorher zur Diagnose bereit. Ein zuvor leuchtendes, separat versorgtes Panel wird durch einen Controller-Neustart ohne Datenübertragung nicht zwangsläufig gelöscht; vor einer Betriebsmigration zuerst STOP/Schwarz ausführen.
- `teensy41_unwired_bench`: nach Nutzerbestätigung USB + Ethernet ohne angeschlossene LEDs vorbereitet. Nur an diesem unverdrahteten Testgerät werden Pins 2–8 verwendet. Eigene Kennzeichnung `unwired_bench:true` verhindert Verwechslung im Lasttestskript. Diese Pinbelegung ist keine freigegebene Verkabelung für die Panels.

## Unveränderte logische Adressierung

| Ausgang | LEDs | Startuniversum | Universen | Pixeloffset |
|---|---:|---:|---|---:|
| 1 | 203 | 120 | 120–121 | 0 |
| 2 | 738 | 122 | 122–126 | 203 |
| 3 | 880 | 127 | 127–132 | 941 |
| 4 | 810 | 133 | 133–137 | 1821 |
| 5 | 352 | 139 | 139–141 | 2631 |
| 6 | 680 | 142 | 142–145 | 2983 |
| 7 | 442 | 146 | 146–148 | 3663 |

4105 Pixel, 28 erwartete Universen, U138 wird ignoriert. Pro Universum werden 170 RGB-Pixel/510 Nutzkanäle abgebildet; 512-Byte-Pakete werden ebenfalls akzeptiert, ihre letzten zwei Kanäle gehören nicht zum folgenden Universum. Eingangsreihenfolge RGB, physische Reihenfolge GRB.

Die bisherige 1122er-Kette muss **physisch nach LED 680 geteilt** werden, bevor dieses Sieben-Ausgangsprofil leuchtet. Nur die Konfiguration zu teilen ändert die Verdrahtung nicht. Längste Kette danach: 880 LEDs, nominal 26,4 ms Datenzeit plus 0,3 ms Rücksetzreserve. Dies ist eine Zeitbudgetrechnung und keine Messung der realen DMA-Ausgabe.

## Empfang und Diagnose

Natives Ethernet verwendet zunächst DHCP; es wird keine unbestätigte statische IP übernommen. IP, Linkzustand und Zähler erscheinen im USB-Serial-JSON. Als Bildquelle ist ausschließlich `10.0.0.125` zugelassen. Das Gerät akzeptiert ArtDmx auf UDP 6454. ArtPoll/ArtSync, Webmenü, Fernkonfiguration und persistente Einstellungen gehören noch nicht zu diesem minimalen Mess-Image.

96 UDP-Empfangspakete puffern etwas mehr als drei vollständige Bilder. Die Hauptschleife verarbeitet bis zu 128 Pakete pro Durchgang ohne Paketpausen. Ein vollständiges Bild wird erst nach allen 28 Universen bereitgestellt. Unvollständige Kandidaten verfallen nach 100 ms; Sequenzwechsel verwirft unvollständige Vorgänger. Wiederholte/veraltete Sequenzen und überschrieben wartende Komplettbilder haben eigene Zähler.

Vor jedem neuen FastLED-Aufruf prüft die Firmware den DMA-Busy-Zustand des gepinnten ObjectFLED-Backends. Aktive Übertragungen werden nicht durch Pufferkopien überschrieben; die Schleife empfängt währenddessen weiter UDP. `dma_completed` zählt beobachtete DMA-Abschlüsse, `dma_elapsed_us` deren Zeit seit dem Aufruf. Diese interne FastLED-Schnittstelle wird durch den festgehaltenen Release gebunden und muss bei jedem Bibliotheksupgrade erneut geprüft werden. UDP-Ankunftszeitstempel verhindern, dass lange gepufferte Pakete als frisch eingetroffen gelten.

Art-Net-Sequenz 1–255 muss **für alle Universen eines Bildes identisch** sein. Sequenz 0 unterstützt den vorhandenen Sender, kann jedoch bei Paketverlust und Neuordnung keine garantierte Bildkohärenz liefern. Für aussagekräftige Integritätstests Sequenznummern am Sender aktivieren. Ein zweiter 12.315-Byte-Puffer enthält nur vollständige Empfangsbilder; die LED-Daten liegen separat. Während der Netzwerkannahme entstehen keine pro-Pixel-Allokationen im eigenen Parser.

Serielle Befehle mit Zeilenende:

- `STATUS`: JSON-Telemetrie; ebenfalls ungefähr jede Sekunde, sofern Serial ausreichend freien Puffer meldet.
- `ARM`: nur bei bestätigtem Hardware-Pinout wirksam. Vorher empfangene Bilder werden verworfen; nur neue vollständige Bilder dürfen leuchten. Die erste vollständige Aufnahme muss innerhalb einer Sekunde eintreffen.
- `STOP`: sofort entwaffnen; nach Ablauf der vorherigen LED-Datenübertragung schwarz senden. Netzwerk-Linkverlust oder eine Sekunde ohne vollständiges Bild führt ebenfalls zu STOP. Kein selbstständiges Wiederanlaufen; erneutes ARM erforderlich.
- `CRASH`: gespeicherten Teensy-CrashReport über USB ausgeben; nur zur Fehlerdiagnose außerhalb einer Leistungsmessung.

Erste Hardwarehelligkeit 16/255, zeitliches Dithering aus. Ausgabe maximal 30 FPS, ohne Aufholbursts. `submit_fps` zählt gestartete FastLED-Aufrufe und ist **kein optisch bestätigter LED-FPS-Zähler**. `show_call_us` misst die Dauer des Aufrufs, nicht zwingend die gesamte asynchrone Datenübertragung. UDP-Drops zählen nur die QNEthernet-Socketqueue, nicht alle möglichen Verluste im PHY/Netzwerk.

## Abnahmemessung

1. USB-Geräte-ID und Seriennummer protokollieren; ausschließlich Safe-Image flashen, Status/ARM-Sperre prüfen.
2. Ethernetkit/PHY und DHCP-Adresse prüfen. Schwarze Sequenzbilder 10/20/30/40 FPS jeweils 60 s senden; Hostzählung, vollständige Frames, UDP-Queue-Drops und Loop-Maximum vergleichen.
3. Tatsächliche Pins und Pegelwandler dokumentieren; Hardwarebuild erst danach. Ein Ausgang und dann sieben Ausgänge testen. Erstmal 10 FPS, anschließend 20 und 30 FPS.
4. Bildnummern mit Kameratest/Logic Analyzer prüfen. Zehn Minuten bei 30 FPS: keine verlorenen Komplettbilder, keine Queue-Drops, kein Watchdog, keine wiederholten/gerissenen Bilder. Der sichere Betrieb unter zusätzlicher Serial-Telemetrie gehört zum Test.
5. Sender stoppen, Kabel abziehen, wieder verbinden und rebooten: Schwarz-/ARM-Verhalten jeweils überprüfen. Alle Ergebnisse als Rohdaten speichern; erst danach 30 FPS als erreicht markieren.

## Primärquellen

- [FastLED 3.10.4](https://github.com/FastLED/FastLED/releases/tag/3.10.4)
- [FastLED ObjectFLED-Beispiel](https://github.com/FastLED/FastLED/blob/3.10.4/examples/SpecialDrivers/Teensy/ObjectFLED/TeensyMassiveParallel/TeensyMassiveParallel.ino)
- [QNEthernet: UDP-Queue und Zähler](https://github.com/ssilverman/QNEthernet/blob/v0.37.0/src/qnethernet/QNEthernetUDP.h)
- [QNEthernet Dokumentation](https://github.com/ssilverman/QNEthernet#udp-receive-buffering)
- [Teensy PlatformIO-Board](https://docs.platformio.org/en/latest/boards/teensy/teensy41.html)
- [PJRC Teensy 4.1 und Ethernet-Hardware](https://www.pjrc.com/store/teensy41.html)

Lizenzhinweis für Weitergabe: QNEthernet verwendet AGPL-3.0-or-later. Die eingebundenen Bibliothekslizenzen und die korrespondierenden Firmwarequellen müssen bei einer späteren Distribution berücksichtigt werden.

## Moderner FastLED-Channel-Pfad und historischer Korrekturpatch

Die aktuelle Firmware verwendet `ChannelConfig` und `Channel::create` mit `FastLED.setExclusiveDriver<fl::Bus::OBJECT_FLED>()`. Damit nutzt sie den modernen Engine im unveränderten FastLED-Release. Gamma ist 1, Dithering deaktiviert, Farbreihenfolge GRB. Die öffentliche Channel-API vermeidet drei im älteren `addLeds`-Proxy unabhängig geprüfte Fehler: nuller `drawBuffer`, zu früh abgeschlossenes Mehrkanallayout und fehlender Frame-End-Flush.

Der erste experimentelle Legacy-Build und sein späterer Zeigerpatch bleiben als Diagnosebelege erhalten. Der Hook `tools/patch_fastled.py` stellt heute ausschließlich diesen exakt bekannten früheren Einfügetext auf den SHA-geprüften Originaltext zurück; er wendet keinen neuen Vendorpatch an. Fremde Änderungen brechen den Build ab. Historischer lesbarer Diff und Patchtests liegen unter `patches/` beziehungsweise `test/`. Die Entscheidung mit geprüften Primärquellen steht in `docs/performance-program-20260912/teensy/driver-decision.md`.

Dieser Patch gilt für unser ausschließlich RGB verwendendes, nicht serpentin gedrehtes Profil. Eine allgemeine RGBW-Freigabe folgt daraus nicht. Boot wird zusätzlich von der LED-Initialisierung getrennt und die erste DMA-Prüfung berücksichtigt noch nicht initialisierte DMA-Completion-Flags. Der tatsächliche Hardwarestatus der Folgeversion steht in `docs/performance-program-20260912/teensy/STATUS.md`.

## Getesteter Ethernet-RX32-Stand (12.09.2026)

Der Hook `tools/patch_qnethernet.py` erweitert ausschließlich im `teensy41_unwired_bench`-Build den SHA-geprüften QNEthernet-Ethernet-Ring von fünf auf 32 Deskriptoren. Die nachgelagerte 96er-UDP-Queue allein verhinderte die beobachteten Burstverluste während der FastLED-Vorbereitung nicht. Unbekannte Vendoränderungen brechen den Patch ab; zwei Regressionstests und die ELF-Pufferprüfung bestanden.

120-s-Hardwaretest: alle 4.081 Bilder / 114.268 Pakete empfangen, 3.598 DMA-Abschlüsse (29,983 FPS), keine Teilbilder oder UDP-Queue-Drops. 34-FPS-Zuspielung bietet Reserve für den auf maximal 30 FPS begrenzten Ausgang; ältere wartende Komplettbilder werden dabei bewusst ersetzt. Im echten TD-Integrationstest kamen 33,770 Bilder/s an und 29,771 DMA-Abschlüsse/s zustande. Beide Tests endeten bestätigt STOP/schwarz. Keine LEDs angeschlossen: noch keine optische Freigabe.

Aktuelle Images: `artifacts/teensy41_unwired_bench_rx32.hex` und `.elf`. Identitäts-/Hashprüfung und Flashprotokoll: `tools/flash_rx32_verified.py`; Ergebnisse: `docs/performance-program-20260912/teensy/RX32_ERGEBNISSE_DE.md` im Projektroot.
