# ArtNet-Controller: Teensy 4.1 / Octo-Adapter

Hardwarekorrektur vom 13.09.2026: Der vorhandene Octo-Adapter bleibt für
Pegelwandlung und die beiden LED-RJ45-Buchsen erhalten. Natives Ethernet läuft
über das separate Flachbandmodul. Der vorgesehene Ausgabepfad bleibt die moderne
FastLED-Channel-API mit ObjectFLED; keine OctoWS2811-Bibliothek und kein Legacy-Wrapper.

Das Repository enthält den SHA-geprüften RX32-Übergabestand als unveränderte
Referenz und einen eigenen Teensy-Build `teensy41_octo_identify_rx32` mit dem
festen Profil `PJRC_OCTO_ADAPTER_T41`, Revision 1. Der Build bietet lokale
Einzel-/Paralleltests und Art-Net-Ausgabe mit der vom Nutzer verlangten
ESP-Belegung. Boot bleibt gestoppt, ohne Initialisierung der LED-Ausgänge.
Webinterface und frei editierbare, dauerhaft gespeicherte Einstellungen sind
noch nicht portiert; die ESP-Belegung ist als Buildprofil hinterlegt.

- [Festes C++-Boardprofil](firmware/include/board_profiles/PjrcOctoAdapterT41.h)
- [Belegung, Beschriftung und Einzel-/Paralleltest](docs/OCTO_ADAPTER_ABNAHME.md)
- [Auszufüllendes Ausgangsprotokoll](reports/octo-output-identification.csv)
- [Quellstand und offene Integration](docs/SOURCE_STATUS.md)
- [Bedienung und ESP-Belegung des neuen Builds](docs/OCTO_RX32_FIRMWARE.md)

Pins OUT1–OUT8: **2, 14, 7, 8, 6, 20, 21, 5**.
Das bestätigt die PJRC-Standardbelegung, nicht die tatsächlich verbaute Platine.
Boardmodell/Revision, Buchsenorientierung und reale LED-Ausgabe sind offen.
Das historische Profil mit Pins 2–8 darf nicht als RJ45-Profil geladen werden.
