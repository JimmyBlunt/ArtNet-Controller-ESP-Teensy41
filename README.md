# ArtNet-Controller: Teensy 4.1 / Octo-Adapter

Hardwarekorrektur vom 13.09.2026: Der vorhandene Octo-Adapter bleibt für
Pegelwandlung und die beiden LED-RJ45-Buchsen erhalten. Natives Ethernet läuft
über das separate Flachbandmodul. Der vorgesehene Ausgabepfad bleibt die moderne
FastLED-Channel-API mit ObjectFLED; keine OctoWS2811-Bibliothek und kein Legacy-Wrapper.

Dieses zuvor leere Repository enthält zunächst das feste Profil
`PJRC_OCTO_ADAPTER_T41`, Revision 1, und die vorbereitete Hardwareabnahme.
Es enthält **noch keine integrierte oder flashbare Teensy-Firmware**.

- [Festes C++-Boardprofil](firmware/include/board_profiles/PjrcOctoAdapterT41.h)
- [Belegung, Beschriftung und Einzel-/Paralleltest](docs/OCTO_ADAPTER_ABNAHME.md)
- [Auszufüllendes Ausgangsprotokoll](reports/octo-output-identification.csv)
- [Quellstand und offene Integration](docs/SOURCE_STATUS.md)

Pins OUT1–OUT8: **2, 14, 7, 8, 6, 20, 21, 5**.
Das bestätigt die PJRC-Standardbelegung, nicht die tatsächlich verbaute Platine.
Boardmodell/Revision, Buchsenorientierung und reale LED-Ausgabe sind offen.
Das historische Profil mit Pins 2–8 darf nicht als RJ45-Profil geladen werden.
