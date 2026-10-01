# README-Grafiken

Eigenständige SVGs, erstellt am 01.10.2026. Dunkles Navy, Cyan und Violett,
zurückhaltende Leiterbahn-/Bogenlinien, native Vektorschrift mit Systemfonts.
Keine externen Bilder, Fonts oder Dienste sind zum Anzeigen erforderlich.

| Datei | Inhalt |
|---|---|
| `01-system.svg` | Art-Net-Quelle, ESP32/WLAN, Teensy/Ethernet und LED-Ausgabe |
| `02-frame-pipeline.svg` | Empfang, gemeinsame Frame-Maske, Timeout, Puffer und DMA-Freigabe des Teensy-Webbuilds |
| `03-run-and-test.svg` | Autostart, Warten/Wiederaufnahme und einmalige bzw. wiederholte Porttests |

Aus dem Repository-Root regenerieren:

```powershell
python tools/render_readme_diagrams.py
```

Inhaltliche Grundlage sind `web_main.cpp`, `runtime_receiver.h`,
`artnet_run_policy.h` und `esp_test_pattern.h` im Teensy-Projekt sowie
`UniverseAssembler.cpp` im ESP-Projekt. Die Grafiken stellen implementierte
Abläufe dar, keine neue physische Hardwareabnahme.

Die Browserprüfung unter `output/playwright/readme-diagrams.html` verwendet
`tools/check_readme_diagrams.js` als Playwright-CLI-Callback. Desktop-Screenshots
und ein mobiler Screenshot liegen unter `output/playwright/readme-*`.
