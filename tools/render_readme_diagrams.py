"""Generate self-contained README SVGs. Standard library only; no network/assets."""
from html import escape
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'docs/assets/readme'
CYAN, VIOLET, WHITE, MUTED = '#69dded', '#aa91ff', '#edf2ff', '#a4b3cb'


def text(x, y, value, size=20, color=WHITE, weight=400, extra=''):
    return f'<text x="{x}" y="{y}" font-size="{size}" fill="{color}" font-weight="{weight}" {extra}>{escape(value)}</text>'


def line(d, color=CYAN, arrow=False, dashed=False):
    return f'<path d="{d}" fill="none" stroke="{color}" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"' + (' marker-end="url(#arrow)"' if arrow else '') + (' stroke-dasharray="5 7"' if dashed else '') + '/>'


def panel(x, y, w, h, accent=CYAN):
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="16" fill="#111c32" stroke="#2a3855"/><path d="M{x+19} {y+1} H{x+w-19}" stroke="{accent}" stroke-opacity=".6" stroke-width="2"/>'


def canvas(name, height, title, description, content):
    head = f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1280 {height}" width="1280" height="{height}" role="img" aria-labelledby="title desc">
<title id="title">{escape(title)}</title><desc id="desc">{escape(description)}</desc>
<defs>
  <linearGradient id="bg" x2="1" y2="1"><stop stop-color="#0b1225"/><stop offset="1" stop-color="#12152d"/></linearGradient>
  <radialGradient id="light"><stop stop-color="#6053ad" stop-opacity=".18"/><stop offset="1" stop-color="#6053ad" stop-opacity="0"/></radialGradient>
  <marker id="arrow" markerWidth="8" markerHeight="8" refX="7" refY="4" orient="auto" markerUnits="userSpaceOnUse"><path d="M1 1 L7 4 L1 7" fill="none" stroke="#a4b3cb" stroke-width="1.4"/></marker>
</defs>
<rect width="1280" height="{height}" rx="24" fill="url(#bg)"/>
<ellipse cx="1120" cy="100" rx="420" ry="320" fill="url(#light)"/>
<g fill="none" stroke="#738bcb" opacity=".09"><path d="M0 28 H186 L218 60 H320"/><path d="M0 38 H178 L210 70 H346"/><path d="M1050 {height} V{height-38} L1082 {height-70} H1280"/><circle cx="-150" cy="{height-50}" r="250"/><circle cx="-150" cy="{height-50}" r="268"/></g>
<g font-family="Segoe UI,Inter,Arial,sans-serif">'''
    svg = head + content + '</g></svg>\n'
    (OUT / name).write_text(svg, encoding='utf-8', newline='\n')


def header(number, title, subtitle):
    return (text(48, 43, f'ART·NET   /   {number}', 14, CYAN, 600, 'letter-spacing="3"')
            + text(48, 100, title, 42, WHITE, 650)
            + text(48, 139, subtitle, 20, MUTED))


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    s = header('01  SYSTEM', 'Ein Netzwerk. Zwei Node-Plattformen.',
               'Von der Art-Net-Quelle bis zum LED-Ausgang — mit eigener Webkonfiguration.')
    s += panel(48, 262, 290, 130)
    s += text(72, 302, 'ART-NET-QUELLE', 14, CYAN, 600, 'letter-spacing="2"')
    s += text(72, 342, 'Lichtdaten / RGB', 27, WHITE, 600)
    s += text(72, 374, 'ArtDmx · UDP 6454', 20, MUTED)
    s += line('M338 327 H390 V251 H442', arrow=True)
    s += line('M390 327 V417 H442', VIOLET, arrow=True)
    s += '<circle cx="390" cy="327" r="4" fill="#69dded"/>'
    s += panel(454, 190, 330, 122)
    s += text(478, 220, '01 / WLAN', 14, CYAN, 600)
    s += text(478, 258, 'ESP32', 30, WHITE, 650)
    s += text(478, 291, 'FastLED · flexible Portprofile', 19, MUTED)
    s += panel(454, 356, 330, 122, VIOLET)
    s += text(478, 386, '02 / NATIVES ETHERNET', 14, VIOLET, 600)
    s += text(478, 424, 'Teensy 4.1', 30, WHITE, 650)
    s += text(478, 457, 'FastLED Channels · ObjectFLED', 19, MUTED)
    s += line('M784 251 H894', arrow=True) + line('M784 417 H894', VIOLET, arrow=True)
    s += panel(906, 190, 326, 122)
    s += text(930, 228, 'WS2812B + APA102', 25, WHITE, 600)
    s += text(930, 260, 'Pins und Längen je Profil', 19, MUTED)
    s += text(930, 290, 'Konfiguration in NVS', 19, MUTED)
    s += panel(906, 356, 326, 122, VIOLET)
    s += text(930, 394, 'OCTO-ADAPTER', 25, WHITE, 600)
    s += text(930, 427, 'Pegelwandlung · 2 × RJ45', 19, MUTED)
    s += text(930, 457, '8 feste WS2812B-Ausgänge', 19, MUTED)
    s += line('M48 519 H1232', '#293451')
    s += text(48, 552, 'Browser → Konfiguration · Porttests · Diagnose', 19, MUTED)
    s += text(1232, 552, 'Quellcode, Web-Assets und Buildprofile im Repository', 17, MUTED, extra='text-anchor="end"')
    canvas('01-system.svg', 584, 'Art-Net: ESP32 und Teensy 4.1',
           'Eine Art-Net-Quelle sendet über UDP 6454 an ESP32 per WLAN oder Teensy 4.1 per nativem Ethernet. ESP unterstützt WS2812B und APA102; Teensy nutzt ObjectFLED und den Octo-Adapter.', s)

    s = header('02  FRAME PIPELINE', 'Vom Paket zum vollständigen LED-Frame.',
               'Teensy-Webbuild · gemeinsame Frame-Sammlung für alle aktiven Ausgänge.')
    cards = [(48, '01 / EMPFANG', 'ArtDmx prüfen', ['Header · Universe · Länge', 'Nur konfigurierte Routen']),
             (350, '02 / SAMMLUNG', 'Universes sammeln', ['Alle aktiven Routen nötig', 'Eine gemeinsame Maske']),
             (652, '03 / BEREIT', 'Frame übernehmen', ['Nur vollständige Bilder', 'Neuestes ersetzt wartendes']),
             (954, '04 / AUSGABE', 'FastLED.show()', ['Art-Net AN · DMA frei', 'Ausgabezeitpunkt erreicht'])]
    for x, label, title, rows in cards:
        s += panel(x, 201, 278, 160, VIOLET if x == 954 else CYAN)
        s += text(x+20, 233, label, 14, VIOLET if x == 954 else CYAN, 600)
        s += text(x+20, 273, title, 23, WHITE, 600)
        s += text(x+20, 311, rows[0], 18, MUTED) + text(x+20, 339, rows[1], 18, MUTED)
        if x != 954:
            s += line(f'M{x+279} 281 H{x+298}', arrow=True)
    s += line('M489 362 V404', CYAN, arrow=True, dashed=True)
    s += panel(350, 416, 580, 88)
    s += text(374, 451, 'Teilframe > 100 ms → verwerfen', 23, WHITE, 600)
    s += text(374, 481, 'Gezählt ab dem ersten akzeptierten Paket des Kandidaten.', 18, MUTED)
    s += line('M1093 362 V414', VIOLET, arrow=True)
    s += text(1093, 449, 'ObjectFLED / DMA', 21, VIOLET, 600, 'text-anchor="middle"')
    s += text(1093, 480, 'Transfer + Latch', 19, MUTED, extra='text-anchor="middle"')
    s += line('M48 543 H1232', '#293451')
    s += text(48, 578, 'Sequence ≠ 0: gemeinsam für alle Universes. Sequence 0: Sammlung ohne Sequenzsortierung.', 20, MUTED)
    s += text(48, 613, 'ArtSync wird nicht ausgewertet. ESP32 verarbeitet die Sequence in seinem eigenen Receiver pro Universe.', 18, MUTED)
    canvas('02-frame-pipeline.svg', 646, 'Teensy Art-Net-Empfang und DMA-Ausgabe',
           'Pakete prüfen, alle erwarteten Universes sammeln, vollständigen Frame puffern und erst bei aktiver Ausgabe, freiem DMA und fälligem Ausgabezeitpunkt ausgeben. Teilframes verfallen nach mehr als 100 Millisekunden.', s)

    s = header('03  RUN / TEST', 'Automatisch starten. Gezielt testen.',
               'Teensy-Webbuild · gültige gespeicherte Konfiguration als Voraussetzung.')
    cards = [(48, '01', 'Konfiguration', 'Beim Boot laden'),
             (350, '02', 'Art-Net AN', 'Initiales Schwarzbild'),
             (652, '03', 'Auf Daten warten', 'Vollständigen Frame bilden'),
             (954, '04', 'LEDs ausgeben', 'FPS- und DMA-Freigabe')]
    for x, number, title, sub in cards:
        s += panel(x, 199, 278, 116)
        s += text(x+19, 226, number, 13, CYAN, 600)
        s += text(x+19, 262, title, 23, WHITE, 600)
        s += text(x+19, 293, sub, 18, MUTED)
        if x != 954:s += line(f'M{x+279} 258 H{x+298}', arrow=True)
    s += line('M1093 316 V348 H791 V317', VIOLET, arrow=True)
    s += text(791, 379, 'Signalverlust → Schwarz → automatisch auf neue Daten warten', 18, MUTED, extra='text-anchor="middle"')
    s += text(48, 433, 'PORTTESTS', 14, VIOLET, 600, 'letter-spacing="2"')
    s += panel(48, 453, 366, 113, VIOLET)
    s += text(70, 490, 'Ein Port oder alle aktiven', 23, WHITE, 600)
    s += text(70, 523, 'Einmal: 20 Sekunden', 19, MUTED)
    s += text(70, 549, 'Loop: bis zum Beenden', 19, MUTED)
    s += line('M415 510 H451', VIOLET, arrow=True)
    s += panel(463, 453, 354, 113, VIOLET)
    s += text(486, 490, 'RGB-Lauflicht', 25, WHITE, 600)
    for x, c in [(486, '#fc6988'), (517, '#69dda7'), (548, '#729bff')]:
        s += f'<rect x="{x}" y="512" width="20" height="20" rx="5" fill="{c}"/>'
    s += text(583, 529, 'mit Webvorschau', 19, MUTED)
    s += line('M818 510 H854', VIOLET, arrow=True)
    s += panel(866, 453, 366, 113, VIOLET)
    s += text(889, 490, 'Test beenden', 25, WHITE, 600)
    s += text(889, 523, 'Schwarzbild abschließen', 19, MUTED)
    s += text(889, 549, 'Vorherigen Modus herstellen', 19, MUTED)
    s += line('M48 601 H1232', '#293451')
    s += text(48, 635, 'Manueller STOP bleibt wirksam bis Start oder Neustart. Die Vorschau ist keine optische Rückmeldung.', 19, MUTED)
    canvas('03-run-and-test.svg', 666, 'Teensy Autostart und Porttest-Ablauf',
           'Beim Boot gültige Konfiguration laden, Art-Net aktivieren, schwarz senden und auf vollständige Daten warten. Porttests laufen einmal oder im Loop. Testende stellt nach Schwarz den vorherigen Modus wieder her; globaler Stop bleibt wirksam.', s)
    print(f'Generated 3 SVG diagrams in {OUT}')


if __name__ == '__main__':
    main()
