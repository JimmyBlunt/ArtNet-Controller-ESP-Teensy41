# Teensy 4.1 / Octo: Build und Quellen

Stand 01.10.2026. Aktiver Betriebsbuild ist `teensy41_octo_web_rx32`, Revision
`orbital-port-tests-20260913`. Teensy 4.1 auf Octo-Adapter mit Pegelwandlung und
LED-RJ45; natives Ethernet am separaten Flachbandmodul. Moderne FastLED-Channel-
API / ObjectFLED, keine OctoWS2811-Bibliothek und kein Legacy-Ausgabewrapper.

## Build

Im Repository-Root unter PowerShell:

```powershell
./firmware/teensy41_artnet/build.ps1 -NativeTests
./firmware/teensy41_artnet/build.ps1 -Environments teensy41_octo_identify_rx32
```

Der Standardaufruf baut **Safe und Web**. Direkter PlatformIO-Aufruf ohne `-e`
baut dagegen nur `teensy41_safe`, entsprechend `default_envs` in der INI.
Python mit venv/pip und für `-NativeTests` ein C++17-Compiler (`g++`) erforderlich.
Das Skript installiert `requirements-build.txt` in `.toolchain` und nutzt
`C:/codex-build/artnet5-rx32` für getrennte kurze Core-/Lib-/Buildpfade.
Über `-Python`, `-VenvPath`, `-CacheRoot` und `-Cxx` anpassbar. Es lädt nichts
auf ein Gerät. HEX/ELF entstehen unter `<CacheRoot>/build/<Umgebung>/`.

Gepinnt: PlatformIO 6.1.18, Teensy 6.0.0, Core/Tool 1.162.0,
GCC-Toolchain 1.150201.0, SCons 4.40801.0, FastLED 3.10.4, QNEthernet 0.37.0;
Web zusätzlich ArduinoJson 6.21.6. `tools/patch_fastled.py` schützt den geprüften
modernen Treiberstand; `tools/patch_qnethernet.py` stellt RX32 her. Beide sind
Buildbestandteile. Bei unbekannten Vendoränderungen brechen die Prüfungen ab.

`tools/embed_web.py` erzeugt `include/web_assets.h` aus HTML/CSS/JS und
`web/orbital-prism.webp` mit deterministischem gzip. Pillow ist zum normalen Build
nicht erforderlich; das bereits konvertierte WebP ist im Repository.

## Aktueller Webstand

- Feste OUT1–8-Pins **2,14,7,8,6,20,21,5**, gemeinsames Profil
  `../include/board_profiles/PjrcOctoAdapterT41.h`.
- Autostart Art-Net AN, Schwarz/Warten ohne vollständige Daten, automatische
  Wiederaufnahme nach Signalverlust; manueller Stop bleibt bis Start/Neustart.
- Webkonfiguration und EEPROM-Journal, Porttests einmal/Loop, animierte
  Farb-/Mustervorschau und Orbital Prism mit 50 % Bildstärke.
- Defaults 4031 Pixel mit OUT6/OUT7=536/512; EEPROM kann abweichen.
  Letzte dokumentierte Aufnahme am 19.09.: OUT6=610, 4105 Pixel, 29 Universes.

Aktiver Empfangspfad: `src/web_main.cpp` → `include/runtime_receiver.h` →
`include/artnet_run_policy.h` → FastLED/ObjectFLED. Ein gemeinsamer kompletter
Frame wird controllerweit erwartet. Sequence-Zustand ist ebenfalls global;
Teilframe-Timeout >100 ms. ArtSync wird nicht ausgewertet.

[Alle sechs Buildprofile](../../docs/NODE_BUILDS.md),
[Bedienung / API / bisherige Abnahme](../../docs/OCTO_WEB.md),
[Empfangsregeln und Diagnose](../../docs/ARTNET_RECEIVER.md),
[Physische Adapterabnahme](../../docs/OCTO_ADAPTER_ABNAHME.md).

## Rückfallstände und Grenzen

`teensy41_octo_identify_rx32` ist der USB-geführte Octo-Stand.
`teensy41_safe`, `teensy41_driver_compile` und `teensy41_unwired_bench` verwenden
`src/main.cpp` und den historischen Receiver. GPIO2–8 des unwired-Bench sind
kein RJ45-Profil. `teensy41_hardware` benötigt absichtlich eine zusätzliche,
geprüfte `include/hardware_pins.h` und ist hier nicht konfiguriert.

Vorhandene HEX/ELF unter `artifacts` gehören zu früheren dokumentierten Releases;
ein neuer Build überschreibt sie nicht. Der ursprüngliche RX32-Übergabestand
mit seinen Messberichten und seiner damaligen README bleibt unverändert unter
`../../reference/teensy-rx32-385d5ed`. Die alte Aussage „keine Octo-Platine“ gilt
nur dort, nicht für diesen aktiven Webbuild.

Build-/Hostprüfungen stehen unter `../../reports/build-verification-20261001`.
Software-DMA-Zähler ersetzen keine optische LED- oder Buchsenabnahme.
