"""Deterministic, self-contained English SVG diagrams; Python standard library only.

Technical facts: docs/ARTNET_DATA_FLOW.md. Measurements are loaded from the
archived CSV, never synthesized. Reference images inform the visual style only.
"""
import csv
from html import escape
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'docs/assets/readme'
C, V, G, O, P = '#51ddea', '#ac8cff', '#69e4b5', '#ffb86c', '#f887bd'
WHITE, MUTED = '#eef5ff', '#a7bad2'


def txt(x, y, value, size=22, color=WHITE, weight=400, anchor='start'):
    return f'<text x="{x}" y="{y}" font-size="{size}" fill="{color}" font-weight="{weight}" text-anchor="{anchor}">{escape(str(value))}</text>'


def path(d, color=C, width=2, glow=False, dash=False, arrow=False):
    p = f'<path d="{d}" fill="none" stroke="{color}" stroke-width="{width}" stroke-linecap="round" stroke-linejoin="round"'
    return (p + ' opacity=".5" filter="url(#glow)"/>' if glow else '') + p + (' stroke-dasharray="7 8"' if dash else '') + (' marker-end="url(#arrow)"' if arrow else '') + '/>'


def dot(x, y, color=C, radius=4):
    return f'<circle cx="{x}" cy="{y}" r="{radius*2}" fill="{color}" opacity=".4" filter="url(#glow)"/><circle cx="{x}" cy="{y}" r="{radius}" fill="{color}"/>'


def panel(x, y, w, h, color=C):
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="18" fill="url(#glass)" stroke="{color}" stroke-opacity=".48"/>' + path(f'M{x+24} {y} H{x+w-24}', color, 2, True)


def label(x, y, value, color=C):
    return txt(x, y, value, 16, color, 650)


def card(x, y, w, h, kicker, title, lines, color=C):
    s = panel(x, y, w, h, color) + label(x+24, y+34, kicker, color) + txt(x+24, y+74, title, 27, WHITE, 600)
    for i, row in enumerate(lines):
        s += txt(x+24, y+111+i*29, row, 20, MUTED)
    return s


def icon(x, y, kind, color=C):
    if kind == 'chip':
        s = f'<rect x="{x-25}" y="{y-25}" width="50" height="50" rx="7" fill="#101c35" stroke="{color}" stroke-width="2"/>'
        for d in [-15, 0, 15]:
            s += path(f'M{x+d} {y-35} v10 M{x+d} {y+25} v10 M{x-35} {y+d} h10 M{x+25} {y+d} h10', color)
        return s + f'<rect x="{x-12}" y="{y-12}" width="24" height="24" rx="3" fill="none" stroke="{color}"/>'
    if kind == 'network':
        s = ''
        for dx, dy in [(-35,-22),(35,-22),(-35,22),(35,22)]:
            s += path(f'M{x} {y} L{x+dx} {y+dy}', color) + dot(x+dx, y+dy, color)
        return s + dot(x, y, color, 8)
    return ''.join(dot(x+col*14-28, y+row*14-14, color, 3) for row in range(3) for col in range(5))


def cube(x, y, size, color=C):
    """Small axonometric data cube with a luminous top face."""
    a, b = size*.52, size*.28
    return (f'<polygon points="{x},{y} {x+a},{y-b} {x+2*a},{y} {x+a},{y+b}" fill="{color}" fill-opacity=".32" stroke="{color}"/>'
            f'<polygon points="{x},{y} {x+a},{y+b} {x+a},{y+size+b} {x},{y+size}" fill="#102238" stroke="{color}" stroke-opacity=".8"/>'
            f'<polygon points="{x+a},{y+b} {x+2*a},{y} {x+2*a},{y+size} {x+a},{y+size+b}" fill="{color}" fill-opacity=".18" stroke="{color}" stroke-opacity=".8"/>')


def holo_icon(x, y, kind, color=C):
    """Glowing technical icon used as an infographic focal point."""
    s = f'<circle cx="{x}" cy="{y}" r="36" fill="{color}" fill-opacity=".06" stroke="{color}" stroke-opacity=".55"/>'
    if kind == 'packet':
        s += cube(x-22,y-20,20,color)+cube(x+2,y-8,20,color)
        s += path(f'M{x-5} {y+22} H{x+25}',color,2,True,False,True)
    elif kind == 'route':
        s += path(f'M{x-24} {y-18} H{x-7} V{y} H{x+9} V{y+18} H{x+25}',color,3,True,False,True)
        for px,py in [(x-24,y-18),(x-7,y),(x+9,y+18)]: s += dot(px,py,color,4)
    elif kind == 'mask':
        s += '<path d="M0 0"/>'
        for i in range(3):
            for j in range(3):
                s += f'<rect x="{x-20+j*14}" y="{y-20+i*14}" width="9" height="9" rx="2" fill="{color}" fill-opacity="{.9 if (i+j)%2==0 else .2}" stroke="{color}"/>'
    elif kind == 'led':
        for i in range(4): s += path(f'M{x-25} {y-20+i*13} H{x+18}',color,2,True)+dot(x+24,y-20+i*13,color,4)
    elif kind == 'clock':
        s += f'<circle cx="{x}" cy="{y}" r="22" fill="none" stroke="{color}" stroke-width="3"/><path d="M{x} {y-14} V{y} L{x+13} {y+8}" fill="none" stroke="{color}" stroke-width="3"/>'
    elif kind == 'wave':
        s += path(f'M{x-25} {y} H{x-16} L{x-8} {y-19} L{x+2} {y+19} L{x+12} {y-10} L{x+20} {y} H{x+26}',color,3,True)
    elif kind == 'check':
        s += f'<circle cx="{x}" cy="{y}" r="22" fill="none" stroke="{color}" stroke-width="3"/><path d="M{x-12} {y} l9 10 17 -22" fill="none" stroke="{color}" stroke-width="4"/>'
    else:
        s += icon(x,y,kind,color)
    return s


def platform(x, y, w, h, color=C, depth=28):
    """Layered transparent isometric slab, sized to sit behind its contents."""
    p1=f'{x},{y} {x+w*.5},{y-h*.29} {x+w},{y} {x+w*.5},{y+h*.29}'
    p2=f'{x},{y} {x+w*.5},{y+h*.29} {x+w*.5},{y+h*.29+depth} {x},{y+depth}'
    p3=f'{x+w*.5},{y+h*.29} {x+w},{y} {x+w},{y+depth} {x+w*.5},{y+h*.29+depth}'
    return (f'<polygon points="{p1}" fill="{color}" fill-opacity=".045" stroke="{color}" stroke-opacity=".58"/>'
            f'<polygon points="{p2}" fill="{color}" fill-opacity=".08" stroke="{color}" stroke-opacity=".65"/>'
            f'<polygon points="{p3}" fill="{color}" fill-opacity=".13" stroke="{color}" stroke-opacity=".75"/>'
            + path(f'M{x+8} {y+depth-2} L{x+w*.5} {y+h*.29+depth-2} L{x+w-8} {y+depth-2}',color,2,True))


def octo_board(x, y):
    """Tiny isometric adapter illustration: two RJ45 jacks, eight lanes."""
    w,h=150,48
    s=platform(x,y,w,h,V,16)
    # Paired connectors on the board's upper surface.
    for dx in [22,62]:
        s += f'<polygon points="{x+dx},{y-4} {x+dx+18},{y-14} {x+dx+37},{y-4} {x+dx+19},{y+7}" fill="#142b42" stroke="{C}" stroke-width="1.5"/>'
        s += f'<path d="M{x+dx+8} {y-4} h18 v8 h-18z" fill="{C}" fill-opacity=".3" stroke="{C}"/>'
    for i in range(8):
        xx=x+20+i*15
        s += path(f'M{xx} {y+19} l17 -9',G,1,True)
        s += dot(xx+17,y+10,G,2.5)
    return s


def header(n, title, subtitle):
    return label(56, 46, f'ART-NET CONTROLLER   /   {n}') + txt(56, 102, title, 43, WHITE, 650) + txt(56, 143, subtitle, 22, MUTED)


def save(name, title, desc, content, height=860):
    deco = ''
    for i in range(7):
        yy = 230+i*35
        deco += path(f'M0 {yy} H{24+i*8} L{64+i*8} {yy+40}', C, 1)
        deco += path(f'M1440 {yy+260} H{1416-i*8} L{1376-i*8} {yy+220}', V, 1)
    s = f'''<svg xmlns="http://www.w3.org/2000/svg" width="1440" height="{height}" viewBox="0 0 1440 {height}" role="img" aria-labelledby="title desc">
<title id="title">{escape(title)}</title><desc id="desc">{escape(desc)}</desc>
<defs>
<linearGradient id="bg" x2="1" y2="1"><stop stop-color="#050c19"/><stop offset=".55" stop-color="#0a1428"/><stop offset="1" stop-color="#100d24"/></linearGradient>
<linearGradient id="glass" x2=".6" y2="1"><stop stop-color="#172f48" stop-opacity=".78"/><stop offset="1" stop-color="#0c1428" stop-opacity=".94"/></linearGradient>
<radialGradient id="halo"><stop stop-color="#305975" stop-opacity=".3"/><stop offset="1" stop-color="#132139" stop-opacity="0"/></radialGradient>
<filter id="glow" x="-100%" y="-100%" width="300%" height="300%"><feGaussianBlur stdDeviation="5"/></filter>
<marker id="arrow" markerWidth="10" markerHeight="10" refX="9" refY="5" orient="auto" markerUnits="userSpaceOnUse"><path d="M1 1 L9 5 L1 9" fill="none" stroke="#c7e1f3" stroke-width="1.7"/></marker>
</defs><rect width="1440" height="{height}" rx="24" fill="url(#bg)"/>
<ellipse cx="400" cy="450" rx="530" ry="420" fill="url(#halo)"/>
<g opacity=".2">{deco}</g><g font-family="Segoe UI,Arial,sans-serif">{content}</g></svg>\n'''
    (OUT/name).write_text(s, encoding='utf-8', newline='\n')


def system():
    s = header('01 / SYSTEM & DEPENDENCIES', 'From network data to light.', 'Two node platforms. Shared RGB intent. Different receive and output backends.')
    # Three transparent, isometric planes joined by a vertical data spine.
    for y, color, name, sub, kind in [(270,C,'NETWORK','ArtDmx / UDP 6454','network'), (435,V,'ASSEMBLY','Routes / sequence / buffers','chip'), (600,G,'OUTPUT','FastLED / physical LEDs','pixels')]:
        pts = f'80,{y} 316,{y-72} 584,{y+12} 348,{y+90}'
        s += f'<polygon points="{pts}" fill="{color}" fill-opacity=".06" stroke="{color}" stroke-opacity=".6"/>'
        s += path(f'M80 {y} L348 {y+90} L584 {y+12}', color, 2, True)
        s += icon(224,y-2,kind,color) + label(315,y-5,name,color) + txt(315,y+23,sub,16,MUTED)
    s += path('M280 286 V598', V, 3, True, True)
    for yy in [320,485,590]: s += dot(280,yy,V)
    s += card(650,201,350,188,'WIRELESS NODE','ESP32',['Wi-Fi → universe assembler','Per-universe sequence state','FastLED → WS2812B / APA102'],C)
    s += card(650,419,350,215,'NATIVE ETHERNET NODE','Teensy 4.1',['QNEthernet → global frame mask','FastLED Channels / ObjectFLED','Octo adapter → 8 WS2812B lanes','Level shifting + 2 LED RJ45s'],V)
    s += card(1036,201,348,188,'CONTROL PLANE','Web interface',['Configuration + start / stop','Per-port tests + preview','ESP: NVS / Teensy: EEPROM'],G)
    s += card(1036,419,348,215,'OBSERVABILITY','Counters & timing',['Packets → complete frames','Submits → DMA completions','Errors, replacements, timing','Preview is not LED feedback'],O)
    s += path('M1000 281 H1036',G,2,True,True,True) + path('M1000 526 H1036',O,2,True,True,True)
    s += panel(56,727,1328,82,V)
    s += label(80,761,'TEENSY BOARD PROFILE',V) + txt(80,790,'PJRC_OCTO_ADAPTER_T41',18,MUTED)
    s += txt(437,776,'OUT 1–8 → pins 2 · 14 · 7 · 8 · 6 · 20 · 21 · 5',25,WHITE,600)
    save('01-system.svg','Art-Net controller architecture and dependencies','Layered network, frame assembly and LED output. ESP32 uses Wi-Fi and per-universe sequence state; Teensy uses native Ethernet and a controller-wide frame mask. Web control and telemetry are separate paths. Fixed Octo pin mapping shown; physical connector identification remains an acceptance task.',s)


def pipeline():
    s = header('02 / PACKETS → FRAMES', 'A packet carries one universe.', 'A complete LED frame is a receiver-defined set of active universes, not one UDP datagram.')
    fields = [('0–7','Art-Net + NUL',260,C),('8–9','0x5000 / LE',208,C),('10–11','Version / BE',208,C),('12','Sequence',168,V),('13','Physical',146,V),('14–15','Address / LE',198,V),('16–17','Length / BE',140,O)]
    x = 56
    for offset,name,w,color in fields:
        s += panel(x,190,w-8,95,color) + label(x+14,219,f'BYTE {offset}',color) + txt(x+14,256,name,18)
        x += w
    s += panel(56,303,1328,61,G) + txt(80,342,'18…  DMX payload: even length 2–512 bytes   •   This project maps 170 RGB pixels into 510 channels.',22,G)
    # A floating route carries packet cubes across the four processing stages.
    for cx,col,kind in [(208,C,'packet'),(548,C,'route'),(888,V,'mask'),(1228,G,'led')]:
        s += path(f'M{cx} 391 V414',col,2,True,True) + holo_icon(cx,390,kind,col)
    for x,k,t,rows,col in [(56,'01 / VALIDATE','Check the datagram',['Header, opcode, version','Exact size = 18 + length'],C),(396,'02 / ROUTE','Match active output',['Universe → RGB offset','Enough bytes for this route'],C),(736,'03 / COLLECT','Set the route bit',['One shared sequence*','All expected bits received'],V),(1076,'04 / READY','Publish the frame',['Copy complete RGB data','Replace older waiting frame'],G)]:
        s += card(x,412,308,175,k,t,rows,col)
        if x<1076: s += path(f'M{x+308} 499 H{x+335}',col,3,True,False,True)
    s += panel(56,632,822,169,V) + label(80,666,'DEFAULT TEENSY MAP / 29 ACTIVE UNIVERSES',V)
    # Raised universe tiles make the active-address mask read as a data layer.
    x=82
    for universe in range(120,150):
        col = MUTED if universe==138 else C
        s += f'<rect x="{x}" y="692" width="18" height="26" rx="4" fill="{col}" fill-opacity="{.12 if universe==138 else .65}"/>'
        if universe in [120,138,149]: s += txt(x+9,741,universe,14,col,anchor='middle')
        x+=25
    s += txt(80,777,'120–137 + 139–149 expected; 138 and disabled OUT8 are excluded.',20,MUTED)
    s += panel(906,632,478,169,O) + label(930,666,'SHORT LAST PACKET / OUT1',O)
    s += txt(930,703,'203 pixels = 170 + 33',27,WHITE,600) + txt(930,740,'U120: 510 bytes · U121: 99 RGB bytes',19,MUTED) + txt(930,774,'Send 100 bytes for U121 (even padding).',20,G)
    s += txt(56,843,'* Shared non-zero sequence is a Teensy implementation contract. ESP tracks sequence per universe. Neither path implements ArtSync.',18,MUTED)
    save('02-frame-pipeline.svg','ArtDmx packet anatomy and Teensy frame assembly','ArtDmx has an 18-byte header and an even 2–512-byte payload. This Teensy receiver requires all active route bits under one shared non-zero sequence. Defaults expect 29 universes, excluding gaps and disabled ports. OUT1 illustrates an even-padded short last packet. ArtSync is not implemented.',s,880)


def tests():
    s = header('03 / CONTROL & TESTING', 'Make every output observable.', 'Start automatically, isolate a port, inspect a pattern, then return to the Art-Net stream.')
    for x,k,t,rows,col in [(56,'01 / BOOT','Art-Net ON',['Valid configuration required','Black until a complete frame'],C),(509,'02 / STREAM','Render when ready',['Armed + complete + DMA idle','Output period must be due'],V),(962,'03 / SIGNAL LOSS','Black once',['No complete frame > 1 s','Remain armed; auto-recover'],O)]:
        s += card(x,204,422,183,k,t,rows,col)
        s += holo_icon(x+378,239,{'01 / BOOT':'chip','02 / STREAM':'wave','03 / SIGNAL LOSS':'clock'}[k],col)
        if x<962: s += path(f'M{x+422} 296 H{x+449}',col,3,True,False,True)
    s += path('M1173 387 V415 H720 V387',C,2,True,True,True) + label(867,443,'COMPLETE DATA RETURNS',C)
    s += panel(56,480,1328,231,V)
    s += label(80,516,'LOCAL PORT TEST / INDEPENDENT OF INCOMING ARTDMX',V)
    s += txt(80,559,'Select',28,WHITE,600) + txt(80,597,'One port or all active ports',20,MUTED) + txt(80,630,'Once (~20 s) or continuous loop',20,MUTED)
    s += path('M418 584 H485',V,3,True,False,True)
    s += txt(513,559,'Animate',28,WHITE,600)
    for i,col in enumerate(['#ff6c85','#69e4b5','#7ca5ff']):
        for j in range(5): s += dot(523+j*20,592+i*28,col,4 if j!=i+1 else 7)
    s += txt(647,601,'R → G → B',22) + txt(647,635,'Running-light pattern',19,MUTED)
    s += path('M873 584 H940',G,3,True,False,True)
    s += txt(968,559,'Restore',28,WHITE,600) + txt(968,598,'Return to the previous state',20,MUTED) + txt(968,631,'Confirm the physical output',20,MUTED)
    s += holo_icon(1282,552,'check',G)
    s += txt(80,683,'Web preview shows the commanded color and pattern; a camera or direct inspection verifies actual LEDs.',21,MUTED)
    s += panel(56,749,1328,62,G) + octo_board(74,778) + txt(256,789,'ACCEPTANCE   Label each RJ45 lane → test lanes together → verify stream recovery.',21,G)
    save('03-run-and-test.svg','Teensy automatic start and port test workflow','Valid configuration starts armed. Complete data, idle DMA and an elapsed output period permit rendering. Signal loss blacks out once and recovery is automatic. Select one or all active ports and a single or looping RGB running-light test. The web preview is not physical feedback.',s)


def performance():
    with (ROOT/'reports/ethernet-matrix-20260913/ergebnisse.csv').open(encoding='utf-8-sig',newline='') as f:
        row = next(r for r in csv.DictReader(f) if r['profile']=='aktuell' and r['soll_fps']=='40')
    complete, dma, replaced = (int(row[k]) for k in ['frames_vollstaendig','dma','ersetzt'])
    s = header('04 / MEASURED PERFORMANCE', 'Receive rate is not display rate.', 'Archived hardware run · 13 Sep 2026 · 4,031 pixels · 29 universes · 40 FPS sender target / 30 FPS output target')
    for x,val,title,sub,col in [(56,complete,'COMPLETE FRAMES',f"{float(row['empfang_fps']):.2f} complete frames/s",C),(509,dma,'DMA COMPLETIONS',f"{float(row['dma_fps']):.2f} completions/s",G),(962,replaced,'WAITING FRAMES REPLACED','Newest complete frame wins',P)]:
        s += panel(x,203,422,171,col) + label(x+24,237,title,col) + txt(x+24,303,f'{val:,}',59,WHITE,650) + txt(x+24,346,sub,22,MUTED)
    s += holo_icon(447,242,'network',C)+holo_icon(900,242,'led',G)+holo_icon(1352-16,242,'clock',P)
    barw=1278
    s += label(56,417,'SETTLED RUN: 1,195 COMPLETE = 900 OUTPUT + 295 REPLACED',C)
    s += f'<rect x="56" y="441" width="{barw*dma/complete:.2f}" height="25" rx="7" fill="{G}"/><rect x="{56+barw*dma/complete:.2f}" y="441" width="{barw*replaced/complete:.2f}" height="25" rx="7" fill="{P}"/>'
    # Extrude each proportional bar segment downward to create a compact 3D meter.
    split=56+barw*dma/complete
    s += f'<polygon points="56,466 {split:.2f},466 {split:.2f},480 56,480" fill="{G}" fill-opacity=".38" stroke="{G}" stroke-opacity=".7"/>'
    s += f'<polygon points="{split:.2f},466 1334,466 1334,480 {split:.2f},480" fill="{P}" fill-opacity=".3" stroke="{P}" stroke-opacity=".7"/>'
    s += txt(56,502,'34,655 / 34,655 packets received · 0 incomplete frames · 0 UDP queue drops',23,MUTED)
    s += card(56,548,646,197,'CODE-DERIVED OUTPUT BUDGET','The longest parallel lane sets wire time.', ['Wire guard = 30 µs × longest lane + 300 µs','880 pixels → 26.70 ms guard','Period = max(ceil(1,000,000 / target FPS), guard)'],V)
    s += card(738,548,646,197,'WHERE WORK CAN ACCUMULATE','Network burst → CPU → LED output', ['32 RX descriptors → 96-packet UDP queue','Copy + FastLED.show() preparation use CPU time','DMA runs asynchronously; completion is polled'],O)
    s += txt(56,794,'~30-second black-payload run; counters do not verify optical FPS or pixel integrity. These are observations, not rated limits.',20,MUTED)
    s += txt(56,828,'Source: reports/ethernet-matrix-20260913/ergebnisse.csv · profile “aktuell”, requested 40 FPS',18,C)
    save('04-performance.svg','Measured frame replacement and timing bottlenecks','Measured September 13 run: 1195 complete frames, 900 DMA completions and 295 replaced waiting frames. No missing packets or incomplete frames in this case. Output target was 30 FPS. Longest-lane wire guard and CPU/network buffering are separate constraints. Black-payload counters are not optical verification.',s)


def diagnostics():
    s = header('05 / DIAGNOSE & TUNE', 'Find the first stage that stops progressing.', 'Compare counter deltas over the same interval. Change one variable, repeat, and verify recovery.')
    for x,k,t,rows,col in [(56,'01 / PACKETS','Traffic arrives',['Count includes rejected data','890 pkt/s ≠ complete frames'],C),(396,'02 / COMPLETE','Frame assembled',['Coverage + length + sequence','Check incomplete / rejected'],V),(736,'03 / SUBMIT','Output scheduled',['Armed? DMA idle? Period due?','Check waiting replacements'],G),(1076,'04 / DMA','Transfer completes',['Compare submit/completion','Check timing + physical LEDs'],O)]:
        s += card(x,204,308,177,k,t,rows,col)
        s += holo_icon(x+268,239,{'01 / PACKETS':'packet','02 / COMPLETE':'mask','03 / SUBMIT':'wave','04 / DMA':'led'}[k],col)
        if x<1076: s += path(f'M{x+308} 293 H{x+335}',col,3,True,False,True)
    for x,col in [(210,C),(550,V),(890,G),(1230,O)]: s += path(f'M{x} 381 V423',col,2,True,False,True)
    rows=[(56,C,'CHECK THE INPUT',['Check target IP + UDP 6454','Inspect header and payload size','Compare sent / received counts']), (396,V,'CHECK FRAME CONTRACT',['Expect only active routes','Match Teensy shared sequence','Partial age >100 ms → abandon']), (736,G,'CHECK OUTPUT BUDGET',['Align input / output target FPS','Balance the longest LED chains','Separate replacement from loss']), (1076,O,'CHECK TRANSFER & LIGHT',['Compare show() / DMA timing','Use non-black test patterns','Check wiring, power, recovery'])]
    for x,col,title,lines in rows:
        s += panel(x,436,308,163,col) + label(x+20,471,title,col)
        s += holo_icon(x+264,464,{'CHECK THE INPUT':'packet','CHECK FRAME CONTRACT':'route','CHECK OUTPUT BUDGET':'clock','CHECK TRANSFER & LIGHT':'check'}[title],col)
        for i,r in enumerate(lines): s += txt(x+20,507+i*29,r,18,MUTED)
    s += panel(56,644,1328,175,V) + label(80,679,'CONTROLLED TUNING / CANDIDATES TO MEASURE',V)
    s += txt(80,721,'PACE',25,C,600) + txt(80,756,'Spread packet bursts;',20,MUTED) + txt(80,786,'100 µs was not a universal fix.',20,MUTED)
    s += txt(489,721,'PROFILE',25,V,600) + txt(489,756,'Measure RX ring, CPU gaps,',20,MUTED) + txt(489,786,'queue occupancy and frame age.',20,MUTED)
    s += txt(937,721,'VERIFY',25,G,600) + txt(937,756,'Repeat longer runs; consider',20,MUTED) + txt(937,786,'frame IDs / CRC + optical checks.',20,MUTED)
    s += platform(420,814,600,20,G,12)
    save('05-diagnostics.svg','Art-Net diagnostics, errors and performance tuning','Follow packet, complete-frame, submit and DMA counters in order. Validate universe coverage, length and sequence before adjusting buffering. Distinguish intentional replacement from receive loss. Measure pacing, ring pressure and timing; use non-black patterns and physical checks.',s)


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    system()
    pipeline()
    tests()
    performance()
    diagnostics()


if __name__ == '__main__':
    main()
