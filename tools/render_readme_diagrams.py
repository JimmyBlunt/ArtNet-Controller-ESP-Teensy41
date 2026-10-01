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


def save(name, title, desc, content, height=860, scene=False):
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
{f'<ellipse cx="720" cy="{height-250}" rx="760" ry="210" fill="url(#halo)"/>' if scene else ''}
{_floor(height) if scene else ''}
<g opacity=".2">{deco}</g><g font-family="Segoe UI,Arial,sans-serif">{content}</g></svg>\n'''
    (OUT/name).write_text(s, encoding='utf-8', newline='\n')


def _floor(height):
    """Perspective stage grid kept behind scene artwork and labels."""
    horizon=height-295
    s=f'<path d="M0 {horizon} L1440 {horizon} L1440 {height} L0 {height}Z" fill="#091326" fill-opacity=".48"/>'
    for i in range(17):
        x=i*90
        s+=f'<path d="M720 {horizon} L{x} {height}" fill="none" stroke="#56cce7" stroke-opacity=".075"/>'
    for i in range(1,8):
        y=horizon+(height-horizon)*(i/8)**1.7
        s+=f'<path d="M0 {y} H1440" fill="none" stroke="#8376ea" stroke-opacity=".075"/>'
    s+=f'<path d="M0 {horizon} H1440" stroke="{C}" stroke-opacity=".2" filter="url(#glow)"/>'
    return s


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
    s=header('02 / PACKETS → FRAMES','One packet enters. One complete frame emerges.','An ArtDmx universe is a slice of channel data; the LED frame is assembled across active routes.')
    # Abstract packet ribbons feed an exploded, three-layer rendering stack.
    s+=path('M60 412 C145 335 230 478 315 405 S465 340 520 405',C,5,True)
    for x,y in [(100,379),(190,425),(280,396),(380,374),(468,420)]: s+=cube(x,y,28,C)
    s+=label(62,330,'UDP / ARTDMX',C)+txt(62,356,'one universe per packet',17,MUTED)
    for yy,col,name,detail,kind in [(440,C,'01  VALIDATE + ROUTE','Address · length · RGB destination','route'),(625,V,'02  COLLECT THE GLOBAL MASK','Every active universe · shared sequence','mask'),(810,G,'03  PUBLISH COMPLETE RGB','The newest complete image is ready','led')]:
        s+=platform(235,yy,750,126,col,32)+holo_icon(350,yy-4,kind,col)
        s+=label(420,yy-17,name,col)+txt(420,yy+13,detail,18,MUTED)
        if yy<810:
            s+=path(f'M600 {yy+46} V{yy+142}',col,4,True,True,True)
            for ydot in (yy+67,yy+91,yy+115): s+=dot(600,ydot,col,3)
    # A glowing 29-bit universe mask sits in perspective on the middle layer.
    for i in range(29):
        xx=650+(i%15)*17; yy=606+(i//15)*17; col=MUTED if i==18 else V
        s+=f'<polygon points="{xx},{yy} {xx+7},{yy-4} {xx+14},{yy} {xx+7},{yy+4}" fill="{col}" fill-opacity="{.14 if i==18 else .85}" stroke="{col}" stroke-opacity=".6"/>'
    s+=txt(650,661,'29 active route bits',14,V)
    # The protocol packet is drawn as a vertical holographic schematic.
    s+=panel(1028,202,356,608,C)+label(1058,242,'ARTDMX DATAGRAM',C)+txt(1058,278,'18-byte header',27,WHITE,600)
    for i,(name,detail,col) in enumerate([('ID','Art-Net + NUL',C),('OpCode','0x5000 · little-endian',C),('Version','Protocol ≥ 14',C),('Sequence','0 disables ordering',V),('Port address','15-bit universe',V),('Length','even · 2 to 512',O)]):
        y=319+i*56
        s+=f'<path d="M1058 {y+7} H1090" stroke="{col}" stroke-width="5" opacity=".75"/>'
        s+=label(1106,y+7,name,col)+txt(1106,y+30,detail,16,MUTED)
    s+=f'<path d="M1058 676 H1352" stroke="{G}" stroke-width="24" opacity=".18" filter="url(#glow)"/><path d="M1058 676 H1352" stroke="{G}" stroke-width="3"/>'
    s+=label(1058,710,'PAYLOAD · 18 + declared length',G)+txt(1058,744,'510 RGB bytes per full mapped universe',16,MUTED)
    s+=panel(56,890,678,88,V)+label(80,921,'DEFAULT TEENSY ROUTES',V)+txt(80,954,'120–137 + 139–149  ·  29 active  ·  U138 is a gap',17,WHITE)
    s+=panel(762,890,622,88,O)+label(786,921,'SHORT FINAL UNIVERSE · OUT1',O)+txt(786,954,'203 LEDs → 510 + 99 RGB bytes → send 100 (even)',17,WHITE)
    s+=txt(56,1023,'Shared non-zero sequence is Teensy-specific. ESP tracks sequence per universe. Neither path implements ArtSync.',17,MUTED)
    save('02-frame-pipeline.svg','Isometric ArtDmx packet and frame assembly','An abstract packet ribbon feeds large isometric validation, collection and RGB output layers. The central universe mask contains 29 expected route bits. A holographic packet schematic and short final universe example explain packet boundaries.',s,1055,True)


def tests():
    s=header('03 / OUTPUT & TESTING','Light from a named board profile.','Boot armed. Send a test pattern through the Octo adapter. Identify and verify each physical lane.')
    # Status rail floats above the hardware scene.
    for x,k,title,detail,col,kind in [(72,'01 / BOOT','ART-NET ON','Valid configuration',C,'chip'),(542,'02 / STREAM','WAIT → RENDER','Complete + DMA ready',V,'wave'),(1012,'03 / RECOVERY','BLACK → RESUME','After >1 s without frame',O,'clock')]:
        s+=panel(x,196,356,134,col)+holo_icon(x+51,255,kind,col)
        s+=label(x+98,231,k,col)+txt(x+98,265,title,23,WHITE,650)+txt(x+98,299,detail,17,MUTED)
    s+=path('M428 263 H526',C,4,True,False,True)+path('M898 263 H996',V,4,True,False,True)
    s+=path('M1186 331 C1310 387 1290 431 1150 443',O,3,True,True,True)+label(1150,416,'AUTO-RECOVER',O)
    # A large perspective stage anchors the controller and output illustration.
    s+=f'<ellipse cx="715" cy="719" rx="435" ry="104" fill="{V}" fill-opacity=".08" filter="url(#glow)"/>'
    s+=platform(250,650,932,197,V,46)
    s+=f'<polygon points="467,496 687,418 908,496 687,579" fill="#17314b" stroke="{C}" stroke-width="2"/>'
    s+=f'<polygon points="467,496 687,579 687,632 467,549" fill="#0b182b" stroke="{C}" stroke-opacity=".8"/>'
    s+=f'<polygon points="687,579 908,496 908,549 687,632" fill="#10243b" stroke="{C}" stroke-opacity=".8"/>'
    for i in range(8):
        x=504+i*48
        s+=path(f'M{x} 482 l-13 -19 M{x+13} 531 l-13 19',O,3,True)
    s+=holo_icon(687,494,'chip',C)+label(915,472,'TEENSY 4.1',C)
    s+=txt(915,500,'PJRC OCTO PROFILE',14,MUTED)
    # Eight fiber-like lanes radiate to parallel pixel strips.
    for i in range(8):
        yy=673+i*20; col=[C,V,G,O][i%4]; xend=353+i*103
        s+=path(f'M{535+i*43} 588 C{500+i*28} 632 {xend+70} {yy-23} {xend} {yy}',col,3,True,False,True)
        for pix in range(8): s+=dot(xend+pix*13,yy,col,2.6 if pix!=3 else 4.5)
    s+=label(272,638,'8 PARALLEL LED LANES',V)
    # Twin connector housings echo the two RJ45 jacks without implying their pin orientation.
    for x in [302,1075]:
        s+=f'<polygon points="{x},787 {x+63},765 {x+126},787 {x+63},810" fill="#19334c" stroke="{C}" stroke-width="2"/>'
        s+=f'<polygon points="{x+20},785 {x+63},769 {x+106},785 {x+63},801" fill="#091525" stroke="{C}" stroke-width="2"/>'
        for pin in range(8): s+=path(f'M{x+35+pin*8} 787 v10',O,2)
    s+=txt(518,783,'LEVEL SHIFT  /  TWO LED RJ45 CONNECTORS',16,C,600)
    # Floating controls show the complete test interaction, apart from ArtDmx input.
    s+=panel(54,393,294,132,V)+holo_icon(98,444,'mask',V)
    s+=label(150,425,'SELECT PORTS',V)+txt(150,459,'One / all active lanes',16,WHITE)+txt(150,490,'Once / continuous loop',16,MUTED)
    s+=panel(1092,469,292,132,G)+holo_icon(1135,519,'led',G)
    s+=label(1186,500,'RGB RUNNER',G)+txt(1186,533,'R → G → B',17,WHITE)+txt(1186,562,'Single moving pixel',16,MUTED)
    s+=path('M348 460 C410 465 433 505 484 522',V,3,True,False,True)
    s+=path('M1250 601 C1324 654 1278 729 1196 781',G,3,True,True,True)
    s+=panel(54,872,1330,92,O)+label(80,902,'PHYSICAL ACCEPTANCE',O)
    s+=txt(80,934,'Identify + label each lane → test ports individually → test all active ports in parallel.',19,WHITE)
    s+=txt(80,955,'Web preview shows software intent, not LED feedback. Confirm the adapter revision and RJ45 orientation on the hardware.',16,MUTED)
    save('03-run-and-test.svg','Three-dimensional Teensy Octo board and output test flow','A large isometric Teensy package and Octo adapter scene connects eight parallel pixel lanes to schematic dual RJ45 connectors. Above, boot, stream and recovery states orbit the hardware; side controls show individual/all-port RGB tests. Physical connector orientation remains an acceptance item.',s,985,True)


def performance():
    with (ROOT/'reports/ethernet-matrix-20260913/ergebnisse.csv').open(encoding='utf-8-sig',newline='') as f:
        row = next(r for r in csv.DictReader(f) if r['profile']=='aktuell' and r['soll_fps']=='40')
    complete, dma, replaced = (int(row[k]) for k in ['frames_vollstaendig','dma','ersetzt'])
    s=header('04 / MEASURED PERFORMANCE','1,195 frames enter the scheduler.','Archived hardware run · 4,031 LEDs · 29 universes · sender 40 FPS · output target 30 FPS')
    # The left side is a proportional, extruded three-column telemetry sculpture.
    s+=platform(62,635,884,112,V,42)
    base=591; maxh=244
    columns=[(196,complete,'COMPLETE',C),(469,dma,'DMA OUTPUT',G),(742,replaced,'REPLACED',P)]
    for x,val,name,col in columns:
        h=maxh*val/complete; y=base-h; w=112; dx=38; dy=19
        s+=f'<polygon points="{x},{y} {x+dx},{y-dy} {x+w+dx},{y-dy} {x+w},{y}" fill="{col}" fill-opacity=".32" stroke="{col}" stroke-width="2"/>'
        s+=f'<polygon points="{x},{y} {x+w},{y} {x+w},{base} {x},{base}" fill="{col}" fill-opacity=".78" stroke="{col}" stroke-width="2"/>'
        s+=f'<polygon points="{x+w},{y} {x+w+dx},{y-dy} {x+w+dx},{base-dy} {x+w},{base}" fill="{col}" fill-opacity=".36" stroke="{col}" stroke-width="2"/>'
        s+=f'<path d="M{x+12} {y+12} V{base-12}" stroke="white" stroke-opacity=".3" stroke-width="3"/>'
        s+=txt(x+56,y-18,f'{val:,}',28,WHITE,700,'middle')+label(x+56,628,name,col)
    # The stacks sit over a receding telemetry floor, with the frame equation in the field.
    s+=path('M85 347 H916 M85 395 H916 M85 443 H916 M85 491 H916',C,1,False,True)
    for y in [347,395,443,491]: s+=path(f'M85 {y} L172 {y+40} M916 {y} L830 {y+40}',V,1,False,True)
    s+=panel(78,760,860,102,C)+label(104,795,'SETTLED COUNTER BALANCE',C)
    s+=txt(104,837,f'{complete:,} complete  =  {dma:,} output  +  {replaced:,} replaced',25,WHITE,650)
    # Separate constraint panel; bright wire paths show where time is spent.
    s+=panel(1000,199,384,662,V)+holo_icon(1055,251,'clock',V)
    s+=label(1105,234,'CODE-DERIVED OUTPUT BUDGET',V)+txt(1030,307,'Longest lane sets parallel wire time.',19,WHITE,600)
    s+=path('M1030 332 H1350',V,2,True)
    s+=txt(1030,376,'30 µs × lane pixels + 300 µs',20,MUTED)
    s+=holo_icon(1055,447,'network',O)+label(1105,438,'RECEIVE BURST PATH',O)
    s+=txt(1030,493,'32 RX descriptors',19,WHITE)+path('M1048 510 V554',O,3,True,False,True)
    s+=txt(1030,578,'96-packet UDP queue',19,WHITE)+path('M1048 594 V635',O,3,True,False,True)
    s+=txt(1072,654,'CPU preparation + DMA',19,WHITE)
    s+=txt(1030,707,'880 LEDs → 26.70 ms wire guard',17,MUTED)
    s+=txt(1030,738,'DMA completion is polled; show() call time',16,MUTED)+txt(1030,762,'is not the whole transfer time.',16,MUTED)
    s+=panel(62,893,1322,84,O)+holo_icon(111,934,'check',O)
    s+=txt(165,927,'34,655 / 34,655 packets received · 0 incomplete · 0 UDP queue drops',19,WHITE,600)
    s+=txt(165,954,'~30 s black-payload run. Software counters are not optical FPS, pixel integrity or a rated ceiling.',16,MUTED)
    s+=txt(62,1015,'Source: reports/ethernet-matrix-20260913/ergebnisse.csv · profile “current” · requested 40 FPS',16,C)
    save('04-performance.svg','Three-dimensional Art-Net frame and DMA performance comparison','Proportional isometric columns visualize 1195 complete frames, 900 DMA outputs and 295 replaced waiting frames. A separate timing and receive path panel identifies the longest-lane wire guard, RX descriptors, UDP queue, CPU preparation and DMA polling. Evidence is a short black-payload run, not optical validation.',s,1045,True)


def diagnostics():
    s=header('05 / DIAGNOSE & TUNE','Trace the signal through the stack.','Follow the first layer that stops advancing; tune only after the evidence points to it.')
    layers=[(315,C,'01  NETWORK ARRIVAL','Packets · rejected · ignored','packet'),(475,V,'02  FRAME INTEGRITY','Coverage · length · sequence','mask'),(635,G,'03  OUTPUT SCHEDULER','Armed · ready · period · replaced','clock'),(795,O,'04  DMA + LED LOAD','Submit · completion · physical output','led')]
    # Exploded stack of transparent diagnostic planes, joined by a luminous spine.
    for y,col,title,detail,kind in layers:
        s+=platform(65,y,820,123,col,32)
        s+=path(f'M465 {y-71} V{y-5}',col,4,True,True,True)
        for k in range(3): s+=dot(465,y-57+k*19,col,3)
        s+=holo_icon(210,y-3,kind,col)+label(292,y-25,title,col)+txt(292,y+7,detail,19,WHITE)
    # Diagnostic specimen cards orbit to the right of the stack.
    s+=panel(965,202,419,640,V)
    s+=label(1000,241,'READ COUNTER DELTAS TOGETHER',V)
    s+=holo_icon(1045,309,'network',C)+txt(1100,302,'890 packets / s',24,WHITE,650)
    s+=path('M1045 346 V396',C,3,True,False,True)
    s+=holo_icon(1045,437,'mask',V)+txt(1100,428,'Complete frames?',22,WHITE,650)
    s+=txt(1100,458,'Check expected universe bits',16,MUTED)
    s+=path('M1045 472 V522',V,3,True,False,True)
    s+=holo_icon(1045,558,'wave',G)+txt(1100,552,'Submits / DMA?',22,WHITE,650)
    s+=txt(1100,582,'Separate scheduler from transfer',16,MUTED)
    s+=path('M1045 596 V646',G,3,True,False,True)
    s+=holo_icon(1045,682,'check',O)+txt(1100,676,'Light verified?',22,WHITE,650)
    s+=txt(1100,706,'Test with non-black patterns',16,MUTED)
    s+=path('M1000 755 H1350',V,2,True)+txt(1000,791,'890 pkt/s alone cannot prove a complete frame.',16,MUTED)
    # Three tuning vectors descend from the exploded stack into a grounded footer.
    s+=panel(64,925,1320,111,V)+label(94,959,'CHANGE ONE VARIABLE · REPEAT THE SAME MEASUREMENT',V)
    for x,title,body1,body2,col in [(96,'PACE','Spread packet bursts','100 µs was not a universal fix',C),(512,'PROFILE','Measure RX queue','CPU gaps · frame age',V),(930,'VERIFY','Longer runs + frame IDs','CRC and optical checks',G)]:
        s+=holo_icon(x+20,1001,'route' if title=='PACE' else ('chip' if title=='PROFILE' else 'check'),col)
        s+=label(x+67,990,title,col)+txt(x+67,1013,body1+' · '+body2,15,MUTED)
    s+=txt(65,1075,'Validate routing and payload size before changing buffers. A larger ring remains an unmeasured candidate.',16,MUTED)
    save('05-diagnostics.svg','Exploded Art-Net diagnostics and performance tuning stack','Four large isometric layers show network arrival, frame integrity, output scheduling and DMA/physical output. A luminous vertical spine links the stages; a side diagnostic flow traces counters and a grounded tuning rail presents controlled experiments. High packet rate alone cannot prove complete frame assembly.',s,1100,True)


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    system()
    pipeline()
    tests()
    performance()
    diagnostics()


if __name__ == '__main__':
    main()
