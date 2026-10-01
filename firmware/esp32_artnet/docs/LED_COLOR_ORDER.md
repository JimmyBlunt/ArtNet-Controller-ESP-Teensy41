# LED-Farbreihenfolge · ESP 10.0.0.251

Unter **Ausgänge → Farbreihenfolge** lässt sich für jeden Ausgang RGB, RBG,
GRB, GBR, BRG oder BGR auswählen. Danach **Anwenden** klicken; mit
**Dauerhaft speichern** bleibt die Auswahl nach einem Neustart erhalten.
Die Änderung gilt für Art-Net und die eingebauten LED-Tests, ohne Neustart.
**Pixelrichtung umkehren** ändert separat die räumliche Reihenfolge der LEDs.

Bisher setzte die Oberfläche WS2812B auf GRB und APA102 auf BGR zurück;
die Hardwarevalidierung erlaubte ebenfalls nur diese festen Werte.
Jetzt wird die gewählte Reihenfolge beim Kopieren in den Ausgabepuffer
berücksichtigt. Die vorhandenen FastLED-Controller behalten ihre internen
GRB-/BGR-Templates; die kompensierte Kanalzuordnung erzeugt die gewählte
Reihenfolge auf der Datenleitung. Dadurch können bereits registrierte
Controller auch bei mehrfachen Änderungen wiederverwendet werden.
Der logische RGB-Puffer und die Vorschau bleiben im RGB-Format.

Für das am 15.09.2026 laufende Gerät wird **esp32-wifi-flex8** gebaut
(`hardwareProfile=flex8-ws2812-apa102`). Das historische Buildprofil
`esp32-wifi-esp251` enthält derzeit zum geänderten Extensionboard-Profil
unpassende Defaultpins; dessen vorhandener Profiltest scheitert auch vor
dieser Änderung. Es ist nicht das Buildprofil des laufenden Geräts.

Prüfung: sechs Byte-Reihenfolgen für beide Treiber, Hardwarevalidierung,
Browserauswahl mit Anwenden/Speichern/Neuladen. Eine optische Prüfung der
angeschlossenen LEDs ist zusätzlich am realen Aufbau durchzuführen.
