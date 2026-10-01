"""Check all measured counter identities, restoration evidence and bundle the report."""
from pathlib import Path
import hashlib
import json
import zipfile

ROOT=Path(__file__).resolve().parents[1]
names=['ethernet-matrix-20260913','ethernet-capacity-20260913','ethernet-paced-20260913']
all_cases=[]; checks=[]
for name,expected in zip(names,[25,10,7]):
    r=json.loads((ROOT/'reports'/name/'raw.json').read_text(encoding='utf-8'))
    assert len(r['cases'])==expected
    assert r.get('restored',r.get('restoration_audit',{}).get('config_matches_original',False))
    assert all(q['passed'] for q in r['integrity'])
    for c in r['cases']:
        d=c['delta']
        assert all(d[k]==c['after'][k]-c['before'][k] for k in d)
        assert d['artnet_complete']==d['frames_submitted']+d['artnet_overwritten']
        assert d['frames_submitted']==d['dma_completed']
        assert d['artnet_complete']+d['artnet_incomplete']==c['frames_sent']
        assert d['artnet_packets']==d['udp_received']
        assert c['packets_sent']==c['frames_sent']*c['config']['universeCount']
        assert 30<=c['elapsed_s']<30.1
        assert not c['http_errors']
    all_cases+=r['cases']
    checks.append({'series':name,'cases':len(r['cases']),'integrity_checks':len(r['integrity']),
                   'restoration_verified':True,'all_counter_identities_valid':True})
out=ROOT/'reports'/names[0]
md=(out/'BERICHT_DE.md').read_text(encoding='utf-8')
assert md.count('#### ')==25 and md.count('### Zusatzfall ')==10 and md.count('### Verteilt: ')==7
validation={'passed':True,'cases':len(all_cases),'series':checks,
            'frames_sent':sum(c['frames_sent'] for c in all_cases),
            'complete_frames':sum(c['delta']['artnet_complete'] for c in all_cases),
            'packets_sent':sum(c['packets_sent'] for c in all_cases),
            'packets_received':sum(c['delta']['artnet_packets'] for c in all_cases),
            'nominal_stream_seconds':len(all_cases)*30,
            'min_case_seconds':min(c['elapsed_s'] for c in all_cases),
            'max_case_seconds':max(c['elapsed_s'] for c in all_cases)}
(out/'validation.json').write_text(json.dumps(validation,indent=2),encoding='utf-8')
files=[]
for name in names:
    files += [p for p in (ROOT/'reports'/name).iterdir() if p.is_file() and p.suffix!='.zip' and p.name not in ['manifest.json','pdf-preview.png']]
files += [ROOT/'tools'/name for name in ['benchmark_octo_matrix.py','report_octo_matrix.py','pdf_octo_report.py','verify_package_octo_report.py']]
manifest={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
(out/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
files.append(out/'manifest.json')
with zipfile.ZipFile(out/'Teensy_Ethernet_Testpaket.zip','w',zipfile.ZIP_DEFLATED) as z:
    for p in files:z.write(p,p.relative_to(ROOT))
print(json.dumps(validation,indent=2))
