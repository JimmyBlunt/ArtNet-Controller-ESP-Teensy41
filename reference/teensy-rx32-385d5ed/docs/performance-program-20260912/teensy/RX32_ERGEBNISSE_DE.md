# Teensy: Ethernet-Ring während paralleler FastLED-Ausgabe

## Nachgewiesener Engpass und Änderung

Der gepinnte QNEthernet-0.37.0-Treiber hat fünf RX-Deskriptoren. Ein Testbild
umfasst 28 UDP-Pakete; FastLED benötigt etwa2,2ms CPU-Vorbereitung vor dem
parallelen DMA-Transfer. [Primärquelle: QNEthernet Teensy-Treiber](https://github.com/ssilverman/QNEthernet/blob/v0.37.0/src/qnethernet/drivers/driver_teensy41.cpp).

Ohne DMA wurden600/600Frames empfangen. Mit DMA fehlten2/900Frames und22Pakete,
obwohl die nachgelagerte UDP-Queue keinen Drop meldete. Der SHA-geprüfte Buildhook
`tools/patch_qnethernet.py` erhöht ausschließlich im unwired-Testbuild den
vorgelagerten Ring auf32. Der ELF-Nachweis bestätigt49152Byte RX-Datenpuffer und
1024Byte Deskriptoren. Zusätzlich41472Byte RAM2 und864Byte RAM1 plusAlignment.
FastLED bleibt beim modernen ObjectFLED-Channel-Pfad; kein OctoWS2811.

## Hardwaremessung

| Build / Zustand | Sendebilder | vollständiger Empfang | DMA-Bilder ohne finales Schwarz | Paketverlust |
|---|---:|---:|---:|---:|
| Moderner Originalring, disarmed,20s |600|600|0|0|
| Moderner Originalring, armed,30s |900|898|898|22|
| RX32, armed,60,031852s |1798|1798|1794|0|

RX32 bestätigte50344/50344Datagramme, keinen unvollständigen Frame und keinen
UDP-Queue-Drop. Vier Frames wurden absichtlich durch neuere vollständige Frames
ersetzt. Hostrate29,950767FPS mit einer maximalen Sendelücke130,934ms; daraus
folgt keine exakt30FPS durchgehende Abnahme. Der folgende Lauf mit 34 FPS Zuspielung isoliert die Ausgabe mit ausreichender Senderreserve.
STOP ist bestätigt;1795DMA-Abschlüsse umfassen1794Datenbilder und ein Schwarzbild.
Es sind weiterhin keine LEDs angeschlossen; elektrische Laufzeit-/DMA-Telemetrie
ist kein optischer Nachweis.

## Reproduktion und Flash

Mit den im Buildskript angegebenen kurzen Cachepfaden:
`python -m platformio run -j 1 -e teensy41_unwired_bench`.
Zwei gezielte Patchtests bestanden; unbekannte Vendorquellen werden abgewiesen.
Der Build benötigte401,12s; RAM2 vor dynamischer Treiberallokation417824Byte frei.

`tools/flash_rx32_verified.py` prüft ELF-Ringgrößen, moderne Treibersymbole,
Quellalter, COM4-Seriennummer7858800 und danach HalfKay000BFDD8. Der erste
Flashlauf scheiterte beim USB-Schreiben. Der erneut identitäts-/hashgeprüfte
Aufruf mit Warteoption und relativem Artefaktpfad war erfolgreich. Eine Ursache
für den ersten USB-Schreibfehler ist damit nicht bewiesen. Beide Versuche und
identische Artefakthashes bleiben gespeichert; kein fehlgeschlagener Beleg wurde
überschrieben. Neue Firmware wurde anschließend auf der echten Hardware getestet.

## Abschlussläufe

| Quelle / Dauer | vollständige Bilder | DMA-Abschlüsse | gemessene DMA-FPS | Teilbilder / Queue-Drops |
|---|---:|---:|---:|---:|
| Unabhängiger Sender 34 FPS / 120,001 s |4081|3598|29,983070|0 / 0|
| TouchDesigner-Animation 34 FPS / 90,020 s |3040|2680|29,770998|0 / 0|

Im unabhängigen Lauf kamen alle 114.268 Pakete an. Bei 34,008 Eingangs-FPS und maximal 30 Ausgangs-FPS ersetzt die Mailbox absichtlich ältere vollständige Bilder: 483 in diesem Lauf. Das ist kein Paketverlust. Beim TD-Lauf wurden 359 wartende Bilder ersetzt; ein Bild blieb zum Messende noch in Bearbeitung bzw. wartend. Der Hersteller-Treiber wird vor jedem neuen Aufruf auf DMA-Busy geprüft; beobachtete vollständige Transferdauer etwa 28,68 ms bei 880 LEDs auf der längsten parallelen Kette.

Die Testtools lesen nach Ende der Zuspielung einen frischen Status erst nach Abschluss der letzten DMA. Abschließendes STOP-Schwarz wird getrennt gezählt. Beide Läufe bestätigten anschließend STOP und Schwarz. `rx32-armed34-120s.json` enthält die unabhängige Messung, `../td/td-teensy-fastcores-34fps-90s.json` die Integration samt Wiederherstellung der TD-Einstellungen. Keine optische Abnahme ohne angeschlossene LEDs.
