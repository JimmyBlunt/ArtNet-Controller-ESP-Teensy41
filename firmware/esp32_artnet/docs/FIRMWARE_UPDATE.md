# Firmware-Update über WLAN

Nach dem ersten USB-Flash: Im Controller-Webmenü unter System auf
„Firmware-Update öffnen“ klicken oder `/update` auf der Controlleradresse öffnen.
Eine passende `firmware.bin` auswählen und installieren. Nach erfolgreicher
Prüfung startet der Controller neu. Die Seite kehrt nach zwölf Sekunden zum
Bedienpult zurück. Vorher gewünschte LED-Einstellungen dauerhaft speichern.

Für das bisherige Gerät 10.0.0.251, MAC e8:68:e7:0d:38:a4:

    .venv-platformio/Scripts/platformio.exe run -e esp32-wifi-esp251

Uploaddatei: `.pio/build/esp32-wifi-esp251/firmware.bin`.
Kein Bootloader, Partitionsimage oder zusammengefasstes Flash-Backup hochladen.
Der Build enthält WS2812B/GRB GPIO32 mit 285 LEDs und APA102/BGR Data18/Clock19
mit 1024 LEDs, 4 MHz, 30 FPS als Voreinstellung. Bereits gespeicherte und für
dieses Hardwareprofil gültige Einstellungen haben beim Boot Vorrang.
Die Art-Net-Ausgangsbereiche starten bei Universe 0 und 2 (nullbasiert),
insgesamt 9 Universes. Beide beginnen jeweils bei DMX-Kanal 1.

Die bestehende Standardpartitionierung enthält zwei OTA-App-Partitionen von
je 0x140000 Bytes. Updates schreiben die inaktive App-Partition; erst ein
vollständiges, erfolgreich geprüftes Image wird zum Bootziel. NVS mit der
gespeicherten LED-Konfiguration wird dabei nicht überschrieben. Abgebrochene,
leere, zu große oder ungültige Uploads lösen keinen Neustart aus.

Die Update-Seite setzt einen zufälligen Token pro Boot als Uploadheader und
erlaubt kein CORS. Dies schützt gegen blinde browserübergreifende Uploads,
ist aber keine Benutzeranmeldung: Geräte im vertrauenswürdigen lokalen Netz
können die Update-Seite öffnen. Es werden keine Internetfreigaben eingerichtet.
Eine passende Firmware und stabile Versorgung sind auch beim OTA-Update nötig.

Implementierung mit Espressifs installierter Arduino-ESP32-Update-Bibliothek:
https://docs.espressif.com/projects/arduino-esp32/en/latest/ota_web_update.html

Die Firmwaredatei enthält wie bisher die lokal eingebauten WLAN-Zugangsdaten;
sie ist für dieses Gerät bestimmt und sollte nicht öffentlich veröffentlicht werden.
