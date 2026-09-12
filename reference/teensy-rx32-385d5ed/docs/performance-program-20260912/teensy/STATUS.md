# Teensy 4.1: getesteter RX32-FastLED-Stand

COM4, Seriennummer 7858800, DHCP 10.0.0.253, Ethernet-Link vorhanden. Installiert ist der unverdrahtete Sieben-Ausgangs-Testbuild mit moderner FastLED-Channel-API / ObjectFLED und 32 Ethernet-RX-Deskriptoren. Keine OctoWS2811-Abhängigkeit. Noch keine LEDs angeschlossen.

- 120-s-Test mit unabhängigem Sender: 4.081/4.081 vollständige Bilder, 114.268/114.268 Pakete, keine Teilbilder oder UDP-Queue-Drops. 3.598 tatsächliche DMA-Abschlüsse = **29,983 FPS**. Die Quelle sendete 34,008 FPS; 483 wartende Bilder wurden entsprechend der auf maximal 30 FPS begrenzten Ausgabe durch neuere ersetzt. STOP/schwarz bestätigt.
- 90-s-Integration mit TouchDesigner-Animation: 3.040 vollständige Empfangsbilder = **33,770 FPS** und 2.680 DMA-Abschlüsse = **29,771 FPS**. Keine Teilbilder, kein Watchdog und keine Queue-Drops. Reserviertes U138 wird bewusst ignoriert. TD und Teensy nach Abschluss aus; TD-Konfiguration, CPU-Affinität und OneDrive wiederhergestellt.
- Vor der RX-Erweiterung gingen bei aktiver DMA 22 von 25.200 Paketen verloren. Danach im 60-s-Kontrolltest alle 50.344 Pakete empfangen. Die Messungen stützen die gezielte Pufferkorrektur; daraus folgt keine Garantie gegen beliebige Netzwerklast.
- Die 15 C++-Protokolltests, 10.000-Frame-Stresstest und zwei gezielten SHA-Patchtests bestanden. Native RX32-Firmware gebaut, per ELF überprüft und auf dem identifizierten Board erfolgreich geflasht. Ein erster fehlgeschlagener Flash bleibt mit Log erhalten.

HEX-SHA256: `faefa8abf2c9819555a8442b97347b5bacf7804039cf84ff5553cd86087b0733`.
ELF-SHA256: `4e4ed88e98f6ff3a2f71c0288f96bd05b0da32ea8aff9c371570a96ed9b87a8a`.

Die gemeldeten Abschlüsse belegen den DMA-Pfad, noch keine tatsächlich sichtbaren LED-Bilder. Physische Pinbelegung, Pegelwandler und Teilung der 1.122er-Kette bleiben vor Panelbetrieb zu prüfen. Quellen, Image und Recovery liegen unter `firmware/teensy41_artnet`; Einzelbelege und Interpretation in `RX32_ERGEBNISSE_DE.md`. Frühere Zwischenstände bleiben in `STATUS_ZWISCHENSTAND_MODERN.md` erhalten.
