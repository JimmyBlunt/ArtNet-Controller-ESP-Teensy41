# Art-Net-Receiver: Quellen zur Weitergabe

Bitte zuerst EMPFANGSPFAD.md lesen. Enthalten sind die vollständigen lokalen Source-/Web-/Build-Konfigurationsdateien des Teensy-Unterprojekts samt gemeinsamem Boardprofil und relevanten Prüfscripten (44 Originaldateien), ein lesender Live-Snapshot sowie ein lokaler Empfangscode-Test. Keine Toolchain, Dependency-Caches oder Firmware-Binaries sind enthalten; zum Bauen werden die im Projekt beschriebenen Abhängigkeiten benötigt.

Aktive Build-Umgebung: teensy41_octo_web_rx32. Der aktive Empfangspfad ist web_main.cpp → runtime_receiver.h → artnet_run_policy.h → FastLED/ObjectFLED.

Die Originaldateien wurden gegen Git HEAD geprüft; sie stimmen bytegenau überein. Andere bestehende Arbeitskopie-Änderungen wurden nicht aufgenommen oder verändert. Es wurden weder Firmware noch Controller-Konfiguration verändert und keine Testpakete an den Controller gesendet.

Host-Test (Linux, aus diesem Verzeichnis):

    g++ -std=c++17 -Wall -Wextra diagnostics/frame_probe.cpp -o /tmp/artnet_frame_probe
    /tmp/artnet_frame_probe

SHA256SUMS.txt enthält Prüfsummen aller übrigen Dateien im Paket.
