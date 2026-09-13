# Orbital – zweite Entwurfsrunde

Fünf breite Hintergrundbilder, erstellt mit dem integrierten `image_gen`-Werkzeug.
Die vollständigen Ausgangs- und Korrekturprompts stehen in [prompts.json](prompts.json).

1. [Orbital Silk](01-orbital-silk.png): ausgewogene feine Linien und klare LED-Glanzpunkte.
2. [Orbital Cyan](02-orbital-cyan.png): kühle doppelte Leiterbahnen mit Glasglanz.
3. [Orbital Prism](03-orbital-prism.png): irisierende parallele Linien in Blau, Lila und Cyan.
4. [Orbital Depth](04-orbital-depth.png): stärkere Tiefenstaffelung und schwache Hintergrundstrukturen.
5. [Orbital Whisper](05-orbital-whisper.png): zurückhaltende Linien mit vereinzelten Lichtpunkten.

[Vergleichsansicht](vergleich.html): Auswahl aller fünf Bilder, Bildstärke und eine
ausblendbare Beispieloberfläche. Die dunklen PNGs werden mit `mix-blend-mode: screen`
über dem Blau-Lila-Verlauf dargestellt; sie besitzen keinen transparenten Alphakanal.
Die Bilder zeigen statischen Glow und Glanz. Zeitliches Aufglitzern ist noch keine
Animation und kann bei der späteren Integration ergänzt werden.

Die großen Orbital-Bögen liegen links; feine eckige Leiterbahnen verteilen sich über
die ganze Breite. Die Angabe „540-50%“ wurde im Entwurfsbrief als ungefähr 40–50 Prozent
der LED-Flächenbereiche mit sehr schwachen Untergrundstrukturen interpretiert, nicht
als exakt gezählte Pixelmaske. Die ursprüngliche Punktgeometrie bleibt visuell
erkennbar, ist durch die Bildgenerierung aber nicht pixelgenau erhalten.

Die Bilder sind Entwurfsoriginale. Vor Einbettung in die Firmware muss der gewählte
Hintergrund passend komprimiert werden. Es wurde keine Firmware geändert oder geflasht.
Der gewünschte automatische Start im Art-Net-Modus ist in `docs/NEXT_BUILD.md` vorgemerkt.
