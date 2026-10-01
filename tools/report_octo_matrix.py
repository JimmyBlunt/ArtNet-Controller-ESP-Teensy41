"""Build a detailed German report, CSV tables and static scientific plots from evidence."""
import csv
import base64
import html
import json
from pathlib import Path
import statistics
import sys
import re
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT/'reports/ethernet-matrix-20260913'
sys.path.insert(0, str(OUT/'.report-deps'))
def main():
    raw = json.loads((OUT/'raw.json').read_text(encoding='utf-8'))
    audit=raw.get('restoration_audit',{})
    cases = raw['cases']
    rows = []
    for c in cases:
        d = c['delta']; intervals = np.diff(c['send_times_s'])*1000
        row = dict(profile=c['profile'], soll_fps=c['requested_fps'], dauer_s=c['elapsed_s'],
            universen=c['config']['universeCount'], leds=c['config']['pixelCount'],
            gesendet=c['frames_sent'], pakete_gesendet=c['packets_sent'],
            pakete_empfangen=d['artnet_packets'], frames_vollstaendig=d['artnet_complete'],
            fehlende_frames=c['frames_sent']-d['artnet_complete'], teilbilder=d['artnet_incomplete'],
            queue_drops=d['udp_queue_drops'], ersetzt=d['artnet_overwritten'],
            dma=d['dma_completed'], submits=d['frames_submitted'],
            send_fps=c['send_fps'], empfang_fps=c['receive_fps'], dma_fps=c['output_fps'],
            empfang_prozent=100*d['artnet_complete']/c['frames_sent'],
            ausgabe_prozent=100*d['dma_completed']/c['frames_sent'],
            intervall_p50_ms=float(np.median(intervals)), intervall_p95_ms=float(np.percentile(intervals,95)),
            intervall_max_ms=float(max(intervals)), burst_p95_ms=float(np.percentile(c['burst_ms'],95)),
            http_max_ms=max([s['http_ms'] for s in c['samples']],default=0),
            show_sample_max_ms=max([s['status']['show_call_us']/1000 for s in c['samples']],default=0),
            dma_sample_max_ms=max([s['status']['dma_observed_us']/1000 for s in c['samples']],default=0),
            http_fehler=len(c['http_errors']), bestanden=c['receive_complete'])
        rows.append(row)
    with (OUT/'ergebnisse.csv').open('w', newline='', encoding='utf-8-sig') as f:
        w=csv.DictWriter(f,fieldnames=rows[0]); w.writeheader(); w.writerows(rows)
    plt.rcParams.update({'font.size':10,'axes.spines.top':False,'axes.spines.right':False,'figure.dpi':140})
    profiles=list(dict.fromkeys(r['profile'] for r in rows))
    fig, axes=plt.subplots(2,3,figsize=(14,8),constrained_layout=True)
    for ax,p in zip(axes.flat,profiles):
        rr=[r for r in rows if r['profile']==p]; x=[r['soll_fps'] for r in rr]
        ax.plot(x,x,':',color='#8b95a5',label='Soll Sender')
        for k,label,col in [('send_fps','Sender','#386cb0'),('empfang_fps','Vollständig RX','#23a983'),('dma_fps','DMA-Abschlüsse','#dc7d25')]:
            ax.plot(x,[r[k] for r in rr],marker='o',label=label,color=col)
        ax.set(title=p,xlabel='Angeforderte Sender-FPS',ylabel='Frames / s',ylim=(15,43)); ax.grid(alpha=.2)
    axes.flat[-1].axis('off'); handles,labels=axes.flat[0].get_legend_handles_labels()
    axes.flat[-1].legend(handles,labels,loc='center',frameon=False)
    fig.suptitle('Ethernet-Empfang und Ausgabe · unveränderter Build · Ausgangsziel 30 FPS')
    fig.savefig(OUT/'fps-vergleich.png'); plt.close(fig)
    fig,axes=plt.subplots(1,2,figsize=(13,4.5),constrained_layout=True)
    for ax,key,title in [(axes[0],'empfang_prozent','Vollständiger Empfang / gesendet (%)'),(axes[1],'ausgabe_prozent','DMA-Abschlüsse / gesendet (%)')]:
        matrix=np.array([[next((r[key] for r in rows if r['profile']==p and r['soll_fps']==f),np.nan) for f in [20,25,30,35,40]] for p in profiles])
        ax.imshow(matrix,vmin=60,vmax=100,cmap='YlGn'); ax.set_xticks(range(5),[20,25,30,35,40]); ax.set_yticks(range(len(profiles)),profiles)
        ax.set_title(title); ax.set_xlabel('Sender-Soll-FPS')
        for i in range(len(profiles)):
            for j in range(5): ax.text(j,i,f'{matrix[i,j]:.1f}',ha='center',va='center')
    fig.savefig(OUT/'vollstaendigkeit.png'); plt.close(fig)
    fig,axes=plt.subplots(2,1,figsize=(14,7),constrained_layout=True)
    x=np.arange(len(rows)); labels=[f"{r['profile']}\n{r['soll_fps']}" for r in rows]
    axes[0].bar(x,[r['ersetzt'] for r in rows],color='#dc7d25',label='vollständige Warteframes ersetzt')
    axes[0].bar(x,[r['fehlende_frames'] for r in rows],color='#be3144',label='nicht vollständig empfangen')
    axes[0].set_ylabel('Frames'); axes[0].legend(); axes[0].set_xticks(x,labels,fontsize=7)
    axes[1].plot(x,[r['intervall_p95_ms'] for r in rows],label='Sender-Intervall P95')
    axes[1].plot(x,[r['intervall_max_ms'] for r in rows],label='Sender-Intervall Maximum')
    axes[1].set_ylabel('Millisekunden'); axes[1].set_xticks(x,labels,fontsize=7); axes[1].legend(); axes[1].grid(alpha=.2)
    fig.savefig(OUT/'verluste-und-sendetakt.png'); plt.close(fig)
    # Per-case time series: cumulative-counter-derived rates avoid firmware sampling phase errors.
    for p in profiles:
        fig,axes=plt.subplots(5,1,figsize=(12,13),constrained_layout=True)
        for ax,c in zip(axes,[c for c in cases if c['profile']==p]):
            ss=c['samples']; ts=np.array([s['time'] for s in ss]); dt=np.diff(ts)
            for key,label,col in [('artnet_complete','RX vollständig','#23a983'),('dma_completed','DMA','#dc7d25')]:
                vals=np.array([s['status'][key] for s in ss]); ax.plot(ts[1:]-ts[0],np.diff(vals)/dt,label=label,color=col)
            ax.set(title=f"{p} · Sender {c['requested_fps']} FPS",ylabel='Frames/s',ylim=(0,45)); ax.grid(alpha=.2); ax.legend(loc='lower right')
        axes[-1].set_xlabel('Sekunden ab erster HTTP-Stichprobe')
        fig.savefig(OUT/f'zeitverlauf-{p}.png'); plt.close(fig)
    sent=sum(r['gesendet'] for r in rows); received=sum(r['frames_vollstaendig'] for r in rows)
    md=['# Teensy Ethernet-Testbericht – 13.09.2026','',
        f"Gemessener Build: `{raw['initial']['build_revision']}`. {len(rows)} Lastfälle à ungefähr 30 Sekunden; insgesamt {sent:,} gesendete und {received:,} vollständig empfangene Frames. Die folgenden Aussagen betreffen genau diese Messungen, keine Hochrechnung auf einen Dauerbetrieb.",
        '', f"**Hauptreihe: {sum(r['bestanden'] for r in rows)}/{len(rows)} Empfangsfälle vollständig bestanden. {sent-received} Frames wurden nicht vollständig empfangen; {sum(r['pakete_gesendet']-r['pakete_empfangen'] for r in rows)} gesendete Pakete fehlen in den Receiver-Zählern.** Die bewusst ersetzten Komplettbilder werden separat gezählt. Danach folgen die Zusatzreihe mit angehobenem Ausgangsziel und der gezielte Paketverteilungs-Vergleich, jeweils mit eigenen Rohdaten und Einzelbewertungen.",
        '', '## Testaufbau und Aussagekraft','',
        'Der Windows-Rechner sendete ArtDmx-Unicast auf UDP 6454 an 10.0.0.253. Der lokale Socket und die Ethernet-Route sind in den Rohdaten dokumentiert. Der Teensy bestätigte den Empfang über seine eigenen HTTP-Zähler. Es handelt sich um reale Netzwerkübertragung und reale FastLED-/ObjectFLED-DMA-Aufrufe. TouchDesigner war nicht die Quelle.',
        '', 'Die Firmware wurde weder geändert noch neu geflasht. Die Buildkennung wurde vor dem Test geprüft. Lokale HEX-/Quellhashes liegen bei; eine aus dem Gerät zurückgelesene Binäridentität wurde nicht ermittelt. Für jede geänderte Portlänge wurde die Konfiguration gespeichert und der Controller neu gestartet, weil die initialisierten LED-Kanäle strukturelle Änderungen erst danach übernehmen.',
        '', 'Alle Profile behielten 30 Ziel-FPS, Helligkeit 8/255, WS2812B und die vorhandene Farbreihenfolge. 35/40 FPS sind in dieser Hauptreihe Eingangsbelastungen; sie sind keine Anforderung an einen auf 30 FPS begrenzten Ausgang. Die acht Pins bleiben 2,14,7,8,6,20,21,5.',
        '', 'Je Universum wurden 510 RGB-Nutzbytes gesendet, auch wenn die letzte Teilstrecke weniger benötigt. Die Nutzdaten waren schwarz. Das beansprucht dieselbe Paketgröße und den DMA-Ausgabepfad, prüft aber keine Farbabhängigkeiten, Verkabelung, Stromversorgung unter Weißlast oder sichtbare Pixelkorrektheit. Die Sequenznummer 1–255 wechselte pro Frame und war innerhalb eines Frames für alle Universen gleich.',
        '', 'Der Sender wartet auf den nächsten Termin und holt Verzögerungen nicht mit Frame-Bursts nach. Windows-Scheduling führt deshalb zu leicht niedrigeren Ist-FPS. Als Verlustbasis gilt ausschließlich die tatsächlich erfolgreich an den UDP-Socket übergebene Paket-/Framezahl. UDP-send allein beweist keinen Empfang; diesen belegt der Vergleich mit den unabhängigen Geräte-Zählern. Die Schleife läuft mindestens 30 Sekunden, das letzte Warten kann die Dauer geringfügig verlängern.',
        '', 'HTTP-Status wurde etwa einmal pro Sekunde parallel erfasst. Vorher-/Nachher-Snapshots begrenzen jeden Fall. Nach Sendeende gab es 150 ms Auslauf für das letzte DMA, bevor der Abschlussstatus gelesen wurde. Die daraus berechnete DMA-Rate ist der Durchsatz der zugespielten Testmenge einschließlich abgearbeiteter Restframes, keine exakt zeitgestempelte physische Bildrate.',
        '', '## Zähler richtig lesen','',
        '| Größe | Bedeutung |\n|---|---|\n| Vollständige Frames | Alle konfigurierten Universen einer Sequenz im Receiver vorhanden. |\n| Teilbilder | Begonnener Universensatz durch Timeout/Sequenzwechsel aufgegeben. |\n| Queue-Drops | Vom UDP-Socket gemeldete Überläufe; keine vollständige PHY-Verluststatistik. |\n| Ersetzt | Vollständiges wartendes Bild durch ein neueres ersetzt, bevor es zur Ausgabe genommen wurde. |\n| Submit | FastLED.show für ein Bild gestartet. |\n| DMA-Abschluss | Firmware hat das Ende des Transfers einschließlich eigener Wartebedingungen beobachtet. |\n| Show-Zeit | Dauer von FastLED.show, nicht identisch mit der gesamten LED-Datenzeit. |',
        '', 'Die Frame-Vollständigkeit ist eine Protokollprüfung anhand der Universenmaske. Der Build bietet keine Rückgabe des Framebuffers oder CRC des empfangenen Pixelbildes. Eine bytegenaue Ende-zu-Ende-Integritätsaussage ist daher nicht möglich. `physical_fps_verified` bleibt false. Selbst 100 % empfangene Universensätze sind kein optischer Beweis.',
        '', '## Konfigurationen und Belastung','',
        '| Profil | LEDs | Universen/Frame | Längste Kette | LED-Zeit + 0,3 ms Reserve | ArtDmx bei 40 FPS |\n|---|---:|---:|---:|---:|---:|']
    for p in profiles:
        c=next(c for c in cases if c['profile']==p); n=c['config']['universeCount']; longest=max(o['pixelCount'] for o in c['config']['outputs'])
        md.append(f"| {p} | {c['config']['pixelCount']} | {n} | {longest} | {longest*.03+.3:.2f} ms | {n*40} Pakete/s · {n*40*528*8/1e6:.2f} Mbit/s |")
    md += ['', 'Die Startuniversen der zusätzlichen Profile wurden ab U120 fortlaufend und ohne Überlappung vergeben. Alle acht Ports waren jeweils aktiviert. Universen sind 0-basiert angegeben.', '', '| Profil | OUT1 | OUT2 | OUT3 | OUT4 | OUT5 | OUT6 | OUT7 | OUT8 |\n|---|---|---|---|---|---|---|---|---|']
    for p in profiles:
        config=next(c['config'] for c in cases if c['profile']==p)
        values=[]
        for o in config['outputs']:
            values.append(f"{o['pixelCount']} LEDs / U{o['startUniverse']}–{o['startUniverse']+(o['pixelCount']+169)//170-1}" if o['enabled'] else 'deaktiviert')
        md.append('| '+p+' | '+' | '.join(values)+' |')
    md += ['', 'Bandbreite oben zählt den 18-Byte-ArtDmx-Header plus 510 Nutzbytes, ohne UDP/IP/Ethernet. Mit 66 Byte zusätzlichem IPv4/Ethernet-Aufwand einschließlich Präambel und Paketabstand liegt das Maximum von 48 Universen bei 40 FPS rechnerisch bei etwa 9,12 Mbit/s. Teensy 4.1 besitzt 10/100-Mbit-Ethernet ([PJRC](https://www.pjrc.com/store/teensy41.html)); die tatsächlich ausgehandelte Teensy-Linkrate wird von dieser API nicht gemeldet. Die Host-Linkrate allein beweist keine 100-Mbit-Verbindung am Teensy.',
        '', 'Die Zeitformel 30 µs je Pixel + 300 µs Reserve stammt aus dem getesteten Firmwarecode. Acht Ausgänge werden parallel betrieben: Entscheidend für die Drahtzeit ist die längste Kette. Speicherarbeit und Paketanzahl wachsen trotzdem mit der Gesamtpixelzahl. ObjectFLED beschreibt diesen parallelen DMA-Ansatz in seiner [Primärdokumentation](https://github.com/KurtMF/ObjectFLED).',
        '', '## Gesamtvergleich','', '![FPS-Vergleich](fps-vergleich.png)', '', '![Empfang und Ausgabe](vollstaendigkeit.png)','',
        '| Profil | Soll | Ist Sender | RX FPS | DMA FPS | Frames gesendet/RX | Fehlend | Ersetzt | RX vollständig |\n|---|---:|---:|---:|---:|---:|---:|---:|---:|']
    for r in rows:
        md.append(f"| {r['profile']} | {r['soll_fps']} | {r['send_fps']:.3f} | {r['empfang_fps']:.3f} | {r['dma_fps']:.3f} | {r['gesendet']}/{r['frames_vollstaendig']} | {r['fehlende_frames']} | {r['ersetzt']} | {r['empfang_prozent']:.2f} % |")
    md += ['', '![Ersetzte Frames und Sendetakt](verluste-und-sendetakt.png)', '', '## Jeder Testfall im Detail','']
    for p in profiles:
        md += [f'### Profil {p}', '', f'![Zeitverläufe {p}](zeitverlauf-{p}.png)', '', 'Die Kurven sind Differenzen kumulativer Geräte-Zähler zwischen HTTP-Abfragen. Auflösung ungefähr eine Sekunde; einzelne Spitzen können durch Messfenster/Antwortzeit entstehen und sind keine pro-Frame-Latenzmessung.', '']
        for c,r in zip(cases,rows):
            if r['profile']!=p: continue
            d=c['delta']; balance=d['artnet_complete']-d['frames_submitted']-d['artnet_overwritten']
            md += [f"#### {r['soll_fps']} FPS Zuspielung", '',
                f"Messdauer **{r['dauer_s']:.4f} s**. Gesendet wurden **{r['gesendet']} Frames / {r['pakete_gesendet']} Pakete** bei **{r['send_fps']:.3f} FPS**. Der Teensy zählte **{r['pakete_empfangen']} Pakete / {r['frames_vollstaendig']} vollständige Frames**: **{r['empfang_prozent']:.3f} %** der tatsächlich gesendeten Bilder. Gegenüber dem Soll liegt der Sender um {100*(1-r['send_fps']/r['soll_fps']):.2f} % niedriger.", '',
                f"Ausgabe: **{r['submits']} Submits**, **{r['dma']} DMA-Abschlüsse**, entsprechend **{r['dma_fps']:.3f} FPS** und **{r['ausgabe_prozent']:.2f} %** der eingespeisten Bilder. **{r['ersetzt']}** vollständige Warteframes wurden ersetzt. Die Bilanz vollständig − Submit − ersetzt ist **{balance}**; null bedeutet, dass kein empfangenes Komplettbild in dieser Bilanz unerklärt bleibt. Blackout-Submits im Messfenster: {d['blackouts_submitted']}.", '',
                f"Fehlerzähler: fehlende Pakete **{r['pakete_gesendet']-r['pakete_empfangen']}**, fehlende Komplettbilder **{r['fehlende_frames']}**, Teilbilder **{r['teilbilder']}**, UDP-Queue-Drops **{r['queue_drops']}**, abgewiesen **{d['artnet_rejected']}**, ignoriert **{d['artnet_ignored']}**, veraltet **{d['artnet_stale']}**, Duplikate **{d['artnet_duplicates']}**. HTTP-Abfragefehler: **{r['http_fehler']}**.", '',
                f"Sendetakt: Intervall-Median **{r['intervall_p50_ms']:.3f} ms**, P95 **{r['intervall_p95_ms']:.3f} ms**, Maximum **{r['intervall_max_ms']:.3f} ms**; nominal {1000/r['soll_fps']:.3f} ms. P95 der Paket-Burstdauer pro Frame: **{r['burst_p95_ms']:.3f} ms**. Längste beobachtete HTTP-Antwort: **{r['http_max_ms']:.3f} ms**.", '',
                f"In den Einsekunden-Stichproben: höchste aktuelle Show-Zeit **{r['show_sample_max_ms']:.3f} ms**, höchste aktuelle beobachtete DMA-Dauer **{r['dma_sample_max_ms']:.3f} ms**. Dies sind Stichprobenmaxima; kurze Spitzen dazwischen können fehlen. Firmware-Maximalzähler im Rohprotokoll laufen seit Neustart und dürfen nicht als individuelle Fallmaxima gelesen werden.", '']
            if r['fehlende_frames'] or r['queue_drops'] or r['teilbilder']:
                md += ['**Bewertung:** Auffälliger Empfang. Die Zähler oben zeigen, welcher Anteil nicht vollständig wurde; die Ursache ist anhand dieser Summenzähler nicht eindeutig bis auf PHY, Treiber oder Sender lokalisierbar.', '']
            elif r['ersetzt']:
                md += ['**Bewertung:** Alle gesendeten Bilder kamen vollständig an. Ein Teil wurde vor der Ausgabe bewusst durch neuere vollständige Bilder ersetzt. Damit ist der Engpass dieser Messung im Ausgabetakt bzw. dessen Verarbeitung zu suchen; die Differenz ist kein Ethernet-Paketverlust. Das Verfahren priorisiert Aktualität und verhindert eine anwachsende Warteschlange.', '']
            else:
                md += ['**Bewertung:** In diesem Fall wurden alle gesendeten Frames vollständig empfangen; es wurden keine wartenden Komplettbilder ersetzt. Stimmen Submit- und DMA-Zahl mit der Empfangszahl überein, wurde jedes Bild auch durch den beobachteten Ausgabepfad abgearbeitet.', '']
    capacity_path=ROOT/'reports/ethernet-capacity-20260913/raw.json'
    if capacity_path.exists():
        cap=json.loads(capacity_path.read_text(encoding='utf-8'))
        cc=cap['cases']
        fig,axes=plt.subplots(1,2,figsize=(13,5),constrained_layout=True)
        for ax,fps in zip(axes,[35,40]):
            cr=[c for c in cc if c['requested_fps']==fps]; xx=np.arange(len(cr))
            ax.bar(xx-.18,[c['receive_fps'] for c in cr],width=.36,label='Empfang',color='#23a983')
            ax.bar(xx+.18,[c['output_fps'] for c in cr],width=.36,label='DMA',color='#dc7d25')
            ax.set_xticks(xx,[c['profile'] for c in cr]); ax.axhline(fps,color='#8b95a5',linestyle=':')
            ax.set(title=f'{fps} FPS Zuspielung · Ausgangsziel 60 FPS',ylabel='Frames/s',ylim=(0,45)); ax.legend()
        fig.savefig(OUT/'kapazitaetsvergleich.png'); plt.close(fig)
        md += ['## Zusatzmessung: Ausgabe ohne 30-FPS-Begrenzung','',
            'Diese getrennte Reihe setzt das Ausgangsziel auf 60 FPS und sendet weiterhin nur 35 bzw. 40 FPS. Die Kettenlängenbegrenzung und DMA-Busy-/Latch-Prüfungen der unveränderten Firmware bleiben aktiv. Alle fünf Portprofile werden jeweils erneut gespeichert, gestartet und für ungefähr 30 Sekunden pro Rate getestet. Die ursprüngliche Konfiguration wird auch nach dieser Reihe wiederhergestellt.',
            '', '![Kapazitätsvergleich](kapazitaetsvergleich.png)', '',
            '| Profil | Soll Eingang | Ist Eingang | RX FPS | DMA FPS | gesendet/RX | ersetzt | Frameperiode laut Firmware |\n|---|---:|---:|---:|---:|---:|---:|---:|']
        for c in cc:
            d=c['delta']
            md.append(f"| {c['profile']} | {c['requested_fps']} | {c['send_fps']:.3f} | {c['receive_fps']:.3f} | {c['output_fps']:.3f} | {c['frames_sent']}/{d['artnet_complete']} | {d['artnet_overwritten']} | {c['before']['frame_period_us']/1000:.3f} ms |")
        for c in cc:
            d=c['delta']; intervals=np.diff(c['send_times_s'])*1000; samples=c['samples']
            md += ['', f"### Zusatzfall {c['profile']} · {c['requested_fps']} FPS", '',
                f"Dauer **{c['elapsed_s']:.4f} s**, **{c['frames_sent']} Frames / {c['packets_sent']} Pakete** gesendet; empfangen **{d['artnet_packets']} Pakete / {d['artnet_complete']} vollständige Frames**. Sender **{c['send_fps']:.3f} FPS**, vollständiger Empfang **{c['receive_fps']:.3f} FPS**, Ausgabe **{c['output_fps']:.3f} FPS**. Die Firmware meldet eine Mindestperiode von **{c['before']['frame_period_us']/1000:.3f} ms**.", '',
                f"**{d['frames_submitted']} Submits**, **{d['dma_completed']} DMA-Abschlüsse**, **{d['artnet_overwritten']} ersetzte Warteframes**. Bilanz RX − Submit − ersetzt: **{d['artnet_complete']-d['frames_submitted']-d['artnet_overwritten']}**. Teilbilder **{d['artnet_incomplete']}**, Queue-Drops **{d['udp_queue_drops']}**, abgewiesen **{d['artnet_rejected']}**, ignoriert **{d['artnet_ignored']}**, veraltet **{d['artnet_stale']}**, Duplikate **{d['artnet_duplicates']}**, Blackouts **{d['blackouts_submitted']}**.", '',
                f"Sender-Intervall P50/P95/Maximum: **{np.median(intervals):.3f}/{np.percentile(intervals,95):.3f}/{max(intervals):.3f} ms**. Paket-Burst P95 **{np.percentile(c['burst_ms'],95):.3f} ms**. HTTP-Fehler **{len(c['http_errors'])}**, HTTP-Maximum **{max((s['http_ms'] for s in samples),default=0):.3f} ms**. Aktuelle DMA-Zeit in den Statusstichproben maximal **{max((s['status']['dma_observed_us']/1000 for s in samples),default=0):.3f} ms**.", '',
                ('**Bewertung:** Jedes gesendete Bild wurde vollständig empfangen und im DMA-Pfad abgearbeitet.' if d['dma_completed']==c['frames_sent'] else '**Bewertung:** Trotz angehobenem Ziel wurden vollständige Wartebilder durch neuere ersetzt; die gemessene Ausgabe liegt unter der Zuspielrate. Die nominale Drahtperiode ist eine Untergrenze, zusätzliche Show-/Verarbeitungszeit kann die tatsächlich erreichbare Rate reduzieren.' if d['artnet_overwritten'] else '**Bewertung:** Alle vollständig empfangenen Bilder wurden ausgegeben. Die Differenz zur gesendeten Framezahl entsteht in diesem Fall bereits beim Empfang; es wurden keine vollständigen Wartebilder ersetzt. Die DMA-Zeit ist zusätzlich für die erreichbare Ausgabegrenze zu berücksichtigen.'), '']
            if not c['receive_complete']:
                md += [f"Zusätzlich ist der Empfang auffällig: **{c['packets_sent']-d['artnet_packets']} Pakete** und **{c['frames_sent']-d['artnet_complete']} vollständige Frames** fehlen. Dieser Anteil darf nicht mit ersetzten Komplettbildern zusammengefasst werden.", '']
        ca=cap.get('restoration_audit',{})
        md += [f"Zusatzreihe: **{len(cc)} Fälle**, Wiederherstellung **{cap.get('restored',ca.get('config_matches_original',False))}**, Endzustand `{cap.get('final_status',ca.get('status',{})).get('state','unbekannt')}`. Vollständige Rohdaten unter `../ethernet-capacity-20260913/raw.json`. Auch in dieser Reihe gelten die Grenzen der Schwarzbild-/Softwarezählerprüfung. Die beim Abschluss nochmals protokollierte HTTP-409-Meldung betrifft denselben unnötigen Start nach erfolgreichem Autostart wie in der Hauptreihe; die nachfolgende unabhängige Rücklesung bestätigt die Wiederherstellung.", '']
        with (OUT/'zusatzmessung.csv').open('w',newline='',encoding='utf-8-sig') as f:
            fields=['profile','requested_fps','frames_sent','packets_sent','elapsed_s','send_fps','receive_fps','output_fps','receive_complete']+list(cc[0]['delta'])
            w=csv.DictWriter(f,fieldnames=fields); w.writeheader()
            for c in cc: w.writerow({**{k:c[k] for k in fields if k in c},**c['delta']})
    paced_path=ROOT/'reports/ethernet-paced-20260913/raw.json'
    if paced_path.exists():
        paced=json.loads(paced_path.read_text(encoding='utf-8')); pc=paced['cases']
        fig,ax=plt.subplots(figsize=(12,4.5),constrained_layout=True)
        old_cases=[next(z for z in cases if z['profile']==c['profile'] and z['requested_fps']==c['requested_fps']) for c in pc]
        xx=np.arange(len(pc))
        ax.bar(xx-.18,[100*(1-c['delta']['artnet_complete']/c['frames_sent']) for c in old_cases],width=.36,label='Burst',color='#be3144')
        ax.bar(xx+.18,[100*(1-c['delta']['artnet_complete']/c['frames_sent']) for c in pc],width=.36,label='100 µs Paketabstand',color='#23a983')
        ax.set_xticks(xx,[f"{c['profile']} · {c['requested_fps']} FPS" for c in pc]);ax.set_ylabel('Nicht vollständig empfangene Frames (%)');ax.legend();ax.grid(axis='y',alpha=.2)
        fig.savefig(OUT/'paketverteilung-vergleich.png');plt.close(fig)
        md += ['## Gezielter Verbesserungsversuch: Pakete verteilen','',
            'Aufgrund der beobachteten Burstverluste wurden 8 × 700, 8 × 900 und 8 × 1000 LEDs bei 35/40 FPS nochmals jeweils 30 Sekunden getestet; bei 8 × 1000 zusätzlich 30 FPS, weil bereits dieser Fall in der Hauptreihe auffällig war. Ausgangsziel erneut 30 FPS, gleicher unveränderter Build, gleiche 510-Byte-Schwarzpakete. Einziger geplanter Unterschied zur Hauptreihe: mindestens 100 µs zwischen den Startzeitpunkten einzelner UDP-send-Aufrufe. Die Wartezeit wird im Sender aktiv abgewartet; das erhöht dessen CPU-Bedarf. Das sind keine garantierten Paketabstände auf dem Draht, da Windows/NIC puffern können.',
            '', '![Paketverteilung](paketverteilung-vergleich.png)', '', '| Profil | Soll | fehlende Pakete Burst → verteilt | unvollständige Frames Burst → verteilt | RX-Quote verteilt | DMA FPS verteilt |\n|---|---:|---:|---:|---:|---:|']
        for c in pc:
            old=next(z for z in cases if z['profile']==c['profile'] and z['requested_fps']==c['requested_fps']); d=c['delta']
            md.append(f"| {c['profile']} | {c['requested_fps']} | {old['packets_sent']-old['delta']['artnet_packets']} → {c['packets_sent']-d['artnet_packets']} | {old['delta']['artnet_incomplete']} → {d['artnet_incomplete']} | {100*d['artnet_complete']/c['frames_sent']:.3f} % | {c['output_fps']:.3f} |")
        for c in pc:
            d=c['delta']; intervals=np.diff(c['send_times_s'])*1000
            md += ['', f"### Verteilt: {c['profile']} · {c['requested_fps']} FPS", '',
                f"**{c['elapsed_s']:.4f} s**, **{c['frames_sent']} Frames / {c['packets_sent']} Pakete** gesendet. Empfang **{d['artnet_packets']} Pakete / {d['artnet_complete']} Komplettbilder**. Ist Sender **{c['send_fps']:.3f} FPS**, RX **{c['receive_fps']:.3f} FPS**, DMA **{c['output_fps']:.3f} FPS**. **{d['frames_submitted']} Submits / {d['dma_completed']} DMA-Abschlüsse**, **{d['artnet_overwritten']}** Warteframes ersetzt.", '',
                f"Teilbilder **{d['artnet_incomplete']}**, UDP-Queue-Drops **{d['udp_queue_drops']}**, abgewiesen **{d['artnet_rejected']}**, ignoriert **{d['artnet_ignored']}**, veraltet **{d['artnet_stale']}**, Duplikate **{d['artnet_duplicates']}**, Blackouts **{d['blackouts_submitted']}**. Bilanz RX − Submit − ersetzt **{d['artnet_complete']-d['frames_submitted']-d['artnet_overwritten']}**. HTTP-Fehler **{len(c['http_errors'])}**.", '',
                f"Sender-Intervall P50/P95/Maximum **{np.median(intervals):.3f}/{np.percentile(intervals,95):.3f}/{max(intervals):.3f} ms**. P95 der nun absichtlich verteilten Paketgruppe **{np.percentile(c['burst_ms'],95):.3f} ms**. Eine vollständige Gruppe beansprucht damit länger den Framezeitraum und belastet den Empfänger weniger schlagartig.", '',
                ('**Bewertung:** In diesem Wiederholungslauf wurden alle Testframes vollständig empfangen. Gegenüber einem auffälligen Burstlauf stützt das die Hypothese einer burstabhängigen Überlastung; es beweist keine konkrete Verluststelle im Treiber und keine garantierte Langzeitstabilität.' if c['receive_complete'] else '**Bewertung:** Auch mit verteilter Übertragung bleibt dieser Fall auffällig. Paketverteilung allein ist unter diesen Bedingungen keine ausreichende Abhilfe.'), '']
        md += [f"Wiederherstellung nach dem Verbesserungsversuch: **{paced.get('restored',False)}**, Endzustand `{paced.get('final_status',{}).get('state','unbekannt')}`. Rohdaten: `../ethernet-paced-20260913/raw.json`.", '',
            'Der Vergleich ist eine gezielte einmalige Gegenprobe, keine randomisierte Wiederholungsserie. Unterschiedliche Windows-Last und Phasenlage zu DMA können die Ergebnisse mitbeeinflussen. Bei erfolgreichem Pacing ist eine konfigurierbare Paketverteilung im produktiven Sender ein unmittelbar prüfbarer Ansatz; sie ersetzt keine Profilierung des Empfangsrings.', '',
            'Eine plausible Größenordnung: 32 Ringplätze entsprechen bei idealen 100 µs Paketabstand etwa 3,2 ms Überbrückung. Die Show-Stichproben liegen bei 8 × 700 ungefähr bei 2,8 ms, bei 8 × 900 etwa bei 3,55 ms und bei 8 × 1000 etwa bei 3,94 ms. Das erklärt als Hypothese, weshalb 100 µs für 700 günstiger sein können als für 900/1000. Größere Abstände, beispielsweise 150–200 µs, wären ein nächster kontrollierter Test; sie wurden hier nicht gemessen. Bei 48 Paketen beanspruchen 200 µs rund 9,6 ms pro Framegruppe und müssen in das gesamte Framebudget passen. NIC-Pufferung kann die tatsächlichen Drahtabstände verändern.', '']
        with (OUT/'paketverteilung.csv').open('w',newline='',encoding='utf-8-sig') as f:
            fields=['profile','requested_fps','frames_sent','packets_sent','elapsed_s','send_fps','receive_fps','output_fps','receive_complete']+list(pc[0]['delta'])
            w=csv.DictWriter(f,fieldnames=fields);w.writeheader()
            for c in pc:w.writerow({**{k:c[k] for k in fields if k in c},**c['delta']})
    md += ['## Absichtlich unvollständige Frames','', '| Reihe / Profil | Weggelassenes Universum | zusätzliche Teilbilder | zusätzliche Komplettbilder | zusätzlicher Submit | Recovery komplett | Ergebnis |\n|---|---:|---:|---:|---:|---:|---|']
    integrity=[('Hauptreihe',q) for q in raw['integrity']]
    if capacity_path.exists():integrity += [('Kapazität',q) for q in cap['integrity']]
    if paced_path.exists():integrity += [('Verteilt',q) for q in paced['integrity']]
    for series,q in integrity:
        b=q['before']; a=q['after_missing']; z=q['after_recovery']; c=next(c for c in cases if c['profile']==q['profile'])
        last=max(o['startUniverse']+(o['pixelCount']+169)//170-1 for o in c['config']['outputs'] if o['enabled'])
        md.append(f"| {series} / {q['profile']} | {last} | {a['artnet_incomplete']-b['artnet_incomplete']} | {a['artnet_complete']-b['artnet_complete']} | {a['frames_submitted']-b['frames_submitted']} | {z['artnet_complete']-a['artnet_complete']} | {'bestanden' if q['passed'] else 'FEHLER'} |")
    md += ['', 'Je Profil wurde das letzte Universum eines Frames ausgelassen und 200 ms gewartet. Erwartet: genau ein verworfenes Teilbild, kein neues Komplettbild und kein zusätzlicher Submit. Anschließend wurde ein kompletter Frame mit neuer Sequenz gesendet. Diese gezielte Gegenprobe testet den Vollständigkeitsfilter praktisch. Sie deckt nicht jedes mögliche fehlende Universum, jede Neuordnung oder Nutzdatenkorruption ab.',
        '', '## Einordnung und Verbesserungen','',
        '1. **Empfang und Ausgaberate getrennt bedienen.** In UI und Betrieb sollten Soll-Zuspielrate, vollständige RX-FPS, DMA-FPS und ersetzte Frames gemeinsam sichtbar sein. Ein auf 30 FPS eingestellter Ausgang kann 35/40 eintreffende Bilder pro Sekunde nicht alle anzeigen. Mehr Netzwerkpuffer beheben diese absichtliche Begrenzung nicht. Bei ausreichend kurzen Ketten das Ausgangsziel passend zur Quelle wählen und erneut prüfen. Unterschiedliche Eingangs-/Ausgabetakte können die Phasenlage zwischen Paketburst und synchroner Show-Vorbereitung verschieben; wenn ein höheres Ausgangsziel Empfangsverluste beseitigt, ist das kein Widerspruch, sondern ein Hinweis auf diese zeitliche Kopplung.',
        '2. **Ausgabegrenze aus der Zusatzreihe ableiten.** Die Hauptreihe hält die vorhandenen 30 Ziel-FPS fest; die Zusatzreihe hebt dieses Ziel an und trennt so Konfigurationslimit von tatsächlicher Ausgabegrenze. Bei 900 LEDs ergeben sich bereits nominal 27,3 ms, bei 1000 LEDs 30,3 ms Mindestperiode; 40 FPS verlangen 25 ms und sind mit dieser unveränderten Timingformel ausgeschlossen. 1000 LEDs schließen auch 35 FPS (28,57 ms) aus. Reale Verarbeitung kann die Grenze weiter senken. Ein Mittelwert nahe der Zielrate mit ersetzten Frames ist keine verlustfreie Ausgabe.',
        '3. **Ketten verkürzen, wenn mehr Ausgabe-FPS benötigt werden.** Aus 30 µs/Pixel + 300 µs folgen theoretisch höchstens 942 LEDs bei 35 FPS bzw. 823 bei 40 FPS, jeweils ohne zusätzliche Softwarekosten. Diese Zahlen sind Obergrenzen aus dem Quellcode, keine geprüften Freigaben. Für sichere Reserven darunter bleiben und die tatsächliche DMA-Dauer messen.',
        '4. **Timing-Telemetrie verbessern.** Pro Test rücksetzbare Maxima und Histogramme für Show-Zeit, DMA-Dauer, Framealter und Queue-Belegung würden Engpässe genauer lokalisieren. Zeitstempel für Empfang komplett, Submit und Transferende erlauben echte Latenz- und Jittermessungen. Die aktuellen Einsekunden-Stichproben reichen dafür nicht.',
        '5. **Bytegenaue Integrität ergänzen.** Ein eigener Diagnosemodus könnte eindeutige Frame-/Port-/Pixelmuster empfangen und nach vollständiger Assemblierung einen CRC32 plus Frame-ID melden. Zusätzlich CRC vor Submit vergleichen. Damit würden Fehlzuordnung und Pufferkorruption sichtbar, die Schwarzbilder und Universenmasken nicht erkennen. Dieser Modus gehört in einen getrennten Vergleichsbuild; der hier getestete Stand wurde bewusst nicht verändert.',
        '6. **Sender präzisieren.** Ein absolut getakteter Sender mit ausgewiesenen verpassten Terminen, hochauflösendem Timer und separatem Telemetriepfad würde die Sollraten genauer treffen. Keine Aufholbursts erzeugen. Die vorhandene Sequenzierung 1–255 beibehalten; Sequenz 0 schwächt die zeitliche Kohärenzprüfung.',
        '7. **Burstpfad gezielt profilieren.** Der lokale QNEthernet-Treiber ist auf 32 RX-Deskriptoren gepatcht; 8 × 700 verlangt 40 Pakete, 8 × 900/1000 jeweils 48. Während FastLED.show und weiterer synchroner Verarbeitung kann Ethernet.loop nicht erneut aufgerufen werden. Ein Ringüberlauf vor der 96er-UDP-Queue ist daher eine plausible Erklärung für fehlende Pakete bei gleichzeitig null UDP-Queue-Drops. Auch Host/NIC/Switch bleiben als Verluststellen möglich. Hardware-/lwIP-Dropzähler und ein Paketmitschnitt würden den Ort eingrenzen. Ein kontrollierter RX64-Vergleichsbuild ist ein konkreter Kandidat: zusätzliche 32 × 1536 = 49.152 Byte RX-Datenpuffer plus etwa 1.024 Byte Deskriptoren, vorher Speicher-/Alignment-Budget prüfen. Der Nutzen ist in diesem Bericht noch nicht gemessen; nicht als fertige Lösung behandeln.',
        '8. **Ausgabeverarbeitung optimieren.** Weitere Kandidaten sind das vollständige Löschen des maximalen Pixelpuffers pro Frame, zusätzliche Kopien und die lineare Universensuche. Aktive Bereiche gezielt kopieren und nur notwendige Paddingbereiche löschen könnte Arbeit sparen, darf jedoch keine alten Pixel oder verändertes Treiberlayout verursachen. Messung von Render-/Show-/DMA-Anteilen und unveränderte Frame-/DMA-Puffertrennung sind Voraussetzung. Ein Bibliothekswechsel ist durch die vorliegenden Daten nicht begründet.',
        '9. **Abnahme ausweiten.** 30-Sekunden-Fälle sind ein Lastvergleich, kein Langzeitstabilitätsnachweis. Danach unter realen nichtschwarzen Mustern, Stromversorgung, längster Verkabelung und tatsächlichem TouchDesigner-Sender testen; für sichtbare Vollständigkeit Frame-IDs mit Kamera oder Logic Analyzer prüfen. Wiederholungen und längere Läufe an der beobachteten Grenze sind nötig, bevor eine zuverlässige Betriebsgrenze zugesagt wird.',
        '', '## Wiederherstellung und Dateien','',
        f"Ursprüngliche Konfiguration wiederhergestellt und zurückgelesen: **{raw.get('restored', audit.get('config_matches_original',False))}**. Anfangszustand beim Start des Messskripts: `{raw['initial']['state']}`. Endzustand bzw. unmittelbar folgende Rücklesung: `{raw.get('final_status',audit.get('status',{})).get('state','unbekannt')}`. Fehler des Messlaufs: `{raw.get('error','keine')}`. Protokollierte Meldung beim Abschluss: `{raw.get('restore_error','keine')}`.",
        '', 'Beim Abschluss der Hauptreihe war die Konfiguration bereits erfolgreich gespeichert und nach Neustart aktiv. Ein anschließend unnötig wiederholter Startbefehl wurde mit HTTP 409 abgewiesen, weil Art-Net automatisch lief. Die unabhängige Rücklesung vor der Zusatzreihe bestätigt die ursprüngliche Konfiguration vollständig. Diese Abweichung im Testablauf ist im Rohprotokoll samt Audit erhalten; sie beeinflusst keine der zuvor abgeschlossenen Messungen. Das Skript wurde korrigiert, um den aktiven Zustand vor einem weiteren Start zu prüfen.',
        '', '- `raw.json`: alle Vorher-/Nachher-Zähler, Einsekunden-Statusdaten, Sendezeitpunkte und Paket-Burstdauern.',
        '- `ergebnisse.csv`: auswertbare Tabelle aller Lastfälle, UTF-8 mit BOM.',
        '- `original-config.json`: gesicherte Ausgangskonfiguration.',
        '- `hashes.json`, `host-route.json`, `host-ip.json`: lokale Identitäts-/Netzwerkbelege.',
        '- PNG-Dateien: exportierbare Diagramme; `bericht.html`: lesbarer Gesamtbericht.',
        '', 'Reproduzierbarkeit: `tools/benchmark_octo_matrix.py` erzeugt die Messdaten und verweigert das Überschreiben des bestehenden Ergebnisordners. `tools/report_octo_matrix.py` erzeugt Tabellen, Diagramme und diesen Bericht ausschließlich aus den Rohdaten.',
        '', 'Quellgrundlage der Interpretation: `firmware/teensy41_artnet/src/web_main.cpp` (timing, transferReady, renderArtNet, loop), `include/runtime_receiver.h` (Universenmaske, Sequenzierung, vollständiger Puffer, replaced-Zähler). Die Quellhashes sind beigefügt.']
    all_cases=list(cases)
    for other in [capacity_path,paced_path]:
        if other.exists(): all_cases += json.loads(other.read_text(encoding='utf-8'))['cases']
    overview=['## Ergebnis für die Anwendung','',
        f"Der vollständige Bericht enthält **{len(all_cases)} Lastläufe**, entsprechend **{len(all_cases)*30/60:.1f} Minuten nominaler Ethernet-Zuspielung**, zuzüglich Konfigurationswechseln und **{len(integrity)}** absichtlichen Teilbild-/Recovery-Prüfungen. Jede Reihe bleibt getrennt: Hauptreihe = bestehendes 30-FPS-Ziel; Kapazität = 60-FPS-Ziel; Paketverteilung = 30-FPS-Ziel mit 100 µs Sendeabstand.", '',
        '| Profil | Höchste getestete Sollrate mit allen Frames bis DMA | Dabei verwendetes Ausgangsziel | Gemessene DMA-FPS |\n|---|---:|---:|---:|']
    for p in profiles:
        valid=[c for c in all_cases if c['profile']==p and c['receive_complete'] and c['delta']['dma_completed']==c['frames_sent']]
        if valid:
            best=max(valid,key=lambda c:c['requested_fps'])
            overview.append(f"| {p} | {best['requested_fps']} | {best['config']['targetFps']} | {best['output_fps']:.3f} |")
    overview += ['', 'Diese Tabelle nennt bestandene einzelne Testfälle, keine garantierten Dauerbetriebsgrenzen. Besonders bei 8 × 700 ist die Ausgangseinstellung entscheidend: Ein bestandener 40-FPS-Lauf mit 60-FPS-Ausgangsziel hebt einen fehlerhaften Burstlauf mit 30-FPS-Ausgangsziel nicht auf. Für 8 × 1000 ist 25 FPS die höchste hier vollständig bis DMA bestandene Sollrate; ein höherer Empfangsdurchsatz allein macht daraus keine höhere vollständige Ausgaberate.', '']
    md[md.index('## Testaufbau und Aussagekraft'):md.index('## Testaufbau und Aussagekraft')]=overview
    text='\n'.join(md)+'\n'
    (OUT/'BERICHT_DE.md').write_text(text,encoding='utf-8')
    import markdown
    body=markdown.markdown(text,extensions=['tables','fenced_code'])
    body=re.sub(r'src="([^"\n]+\.png)"',lambda m:'src="data:image/png;base64,'+base64.b64encode((OUT/m[1]).read_bytes()).decode()+'"',body)
    (OUT/'bericht.html').write_text('<!doctype html><html lang="de"><meta charset="utf-8"><title>Teensy Ethernet Testbericht</title><style>body{font:16px/1.65 system-ui;max-width:1180px;margin:40px auto;padding:0 25px;color:#172b40}h1,h2,h3{line-height:1.2;color:#123654}h2{margin-top:3em;border-bottom:2px solid #23a983;padding-bottom:12px}h3{margin-top:2em}h4{font-size:1.2em}img{max-width:100%}table{border-collapse:collapse;width:100%;font-size:13px}td,th{padding:9px;border-bottom:1px solid #cdd9df;text-align:left}th{background:#e8f2f5}code{background:#edf1f4;padding:2px 5px}a{color:#176b9a}@media print{h2,h3,h4{break-after:avoid}img,tr{break-inside:avoid}body{font-size:10pt}}</style>'+body+'</html>',encoding='utf-8')
    print(json.dumps({'cases':len(rows),'frames_sent':sent,'frames_complete':received,'restored':raw.get('restored')}))

if __name__=='__main__': main()
