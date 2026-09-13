# ArtNet-Hintergrundvarianten

Fünf mit dem integrierten `image_gen`-Werkzeug erzeugte Bearbeitungen des
Nutzerbilds. Finale PNGs: 1254 × 1254 Pixel, unverändert aus der Bildgenerierung
kopiert. Originalanordnung der LED-Punktflächen als gestalterische Grundlage;
zusätzliche abstrakte Leiterbahnen in den Freiräumen.

| Datei | Charakter |
|---|---|
| `01-violet-circuit.png` | Feine violette Leiterbahnen und kleine Lichtpunkte |
| `02-cyan-glass.png` | Kühle Cyan-Akzente mit dezentem Glasglanz |
| `03-orbital-traces.png` | Geschwungene Lichtbahnen, etwas bewegter |
| `04-iridescent-etch.png` | Parallele Linien mit blau-lila Farbschimmer |
| `05-ghost-circuit.png` | Vier sehr zarte Fragmente, besonders ruhig |

`vergleich.html` zeigt eine lokale Beispieloberfläche über dem vorhandenen
Blau-Lila-Farbschema. Variante und Deckkraft sind wählbar; die Oberfläche lässt
sich ausblenden. Keine Verbindung zum Controller, keine Firmwareänderung.

Die fünf finalen Bilder haben einen dunklen Hintergrund und **keinen Alphakanal**.
Für die transparente Wirkung verwendet die Vorschau `mix-blend-mode: screen`
und eine variable `opacity`. Der dunkle Bildanteil tritt dadurch zurück.
Empfohlener Einstieg: Variante 2 bei 25–40 Prozent, Variante 5 für mehr Ruhe.
Die Glanzakzente sind statisch; es handelt sich nicht um Animationen.

Ein Transparenzversuch erzeugte zu breite Farbflächen, eine weitere Bearbeitung
ein eingebranntes Schachbrett. Beide wurden aus der finalen Auswahl verworfen.
`prompts.json` enthält die vollständigen finalen Prompts und Herkunftspfade.

Die PNGs sind Entwurfs-/Masterdateien. Vor einem späteren Einbetten in die
Teensy-Firmware sollte nur die ausgewählte Variante passend zur Bildschirmgröße
komprimiert werden; die fünf Master werden nicht gemeinsam in den Flash geladen.
