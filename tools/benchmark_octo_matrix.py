"""Real Ethernet matrix for the existing web build; restores saved configuration."""
import copy
import ctypes
import json
from pathlib import Path
import socket
import struct
import sys
import time
import threading
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parents[1]
CAPACITY = '--capacity' in sys.argv
PACED = '--paced' in sys.argv
OUT = ROOT / ('reports/ethernet-paced-20260913' if PACED else 'reports/ethernet-capacity-20260913' if CAPACITY else 'reports/ethernet-matrix-20260913')
BASE = 'http://10.0.0.253'
KEYS = ['udp_received', 'udp_queue_drops', 'artnet_packets', 'artnet_complete',
        'artnet_incomplete', 'artnet_rejected', 'artnet_ignored', 'artnet_stale',
        'artnet_duplicates', 'artnet_overwritten', 'frames_submitted',
        'dma_completed', 'blackouts_submitted']

def api(path, body=None):
    req = Request(BASE + '/api/' + path, data=None if body is None else json.dumps(body).encode(),
                  headers={'Content-Type': 'application/json'})
    with urlopen(req, timeout=4) as response:
        return json.load(response)

def wait(predicate, seconds=25):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        try:
            status = api('status')
            if predicate(status):
                return status
        except (OSError, ValueError):
            pass
        time.sleep(.15)
    raise RuntimeError('Device status timeout')

def stop():
    api('stop', {})
    return wait(lambda s: s['state'] == 'STOPPED' and not s['dma_pending'] and s['black_latched'])

def configure(config):
    stop()
    if api('config') != config:
        api('config', config)
        api('save', {})
        api('reboot', {})
        time.sleep(2)
        wait(lambda s: not s['reboot_required'] and s['link'])
    actual = api('config')
    for key in ['outputs', 'brightness', 'targetFps', 'network']:
        assert actual[key] == config[key], (key, actual, config)
    return actual

def profile(original, length):
    c = copy.deepcopy(original)
    if CAPACITY:
        c['targetFps'] = 60
        for o in c['outputs']:
            o['targetFps'] = 60
    if length:
        universe = 120
        offset = 0
        for o in c['outputs']:
            o.update(enabled=True, pixelCount=length, startUniverse=universe, startPixel=offset)
            universe += (length + 169)//170
            offset += length
        c['pixelCount'] = offset
        c['universeCount'] = universe - 120
    return c

def packet_templates(config):
    result = []
    for o in config['outputs']:
        if not o['enabled']:
            continue
        for offset in range(0, o['pixelCount'], 170):
            u = o['startUniverse'] + offset//170
            header = b'Art-Net\0' + struct.pack('<HH', 0x5000, 0x0e00)
            # Full 510-byte payload deliberately stresses the last universe too.
            result.append(bytearray(header + bytes([1, 0]) + struct.pack('<H', u) + b'\x01\xfe' + bytes(510)))
    return result

def main():
    OUT.mkdir(parents=True, exist_ok=False)
    original = api('config')
    initial = api('status')
    evidence = {'initial': initial, 'original_config': original, 'cases': [], 'integrity': [],
                'payload': '510 zero RGB bytes per universe; nonzero frame sequence 1..255',
                'transport': 'UDP ArtDmx unicast over physical Ethernet; HTTP receiver counters',
                'physical_verified': False, 'pixel_readback_available': False}
    (OUT/'original-config.json').write_text(json.dumps(original, indent=2))
    def save():
        (OUT/'raw.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.connect(('10.0.0.253', 6454))
    evidence['source'] = sock.getsockname()
    evidence['packet_spacing_us'] = 100 if PACED else 0
    ctypes.windll.winmm.timeBeginPeriod(1)
    sequence = 0
    def send(templates, omit=False):
        nonlocal sequence
        sequence = sequence % 255 + 1
        for p in (templates[:-1] if omit else templates):
            tick = time.perf_counter()
            p[12] = sequence
            sock.send(p)
            if PACED:
                while time.perf_counter() - tick < .0001:
                    pass
    try:
        assert initial['build_revision'] == 'orbital-port-tests-20260913'
        before = api('status'); time.sleep(1.2); after = api('status')
        assert before['artnet_packets'] == after['artnet_packets'], 'Another sender active'
        for length in ([700, 900, 1000] if PACED else [0, 600, 700, 900, 1000]):
            label = 'aktuell' if not length else f'8x{length}'
            config = configure(profile(original, length))
            templates = packet_templates(config)
            stop(); api('start', {}); time.sleep(.2)
            # Prove that omission at each profile cannot publish a partial frame.
            b = api('status'); send(templates, omit=True); time.sleep(.2); a = api('status')
            check = {'profile': label, 'before': b, 'after_missing': a,
                     'passed': a['artnet_complete'] == b['artnet_complete'] and
                               a['frames_submitted'] == b['frames_submitted'] and
                               a['artnet_incomplete'] == b['artnet_incomplete']+1}
            send(templates); time.sleep(.15); check['after_recovery'] = api('status')
            check['passed'] &= check['after_recovery']['artnet_complete'] == a['artnet_complete']+1
            evidence['integrity'].append(check); save()
            assert check['passed'], 'Partial frame gate failed'
            rates = [30, 35, 40] if PACED and length == 1000 else [35, 40] if CAPACITY or PACED else [20, 25, 30, 35, 40]
            for fps in rates:
                stop(); api('start', {}); time.sleep(.2)
                before = api('status')
                samples, errors = [], []
                done = threading.Event()
                def poll():
                    while not done.wait(1):
                        try:
                            t = time.perf_counter(); s = api('status')
                            samples.append({'time': t, 'http_ms': (time.perf_counter()-t)*1000, 'status': s})
                        except Exception as exc:
                            errors.append(repr(exc))
                worker = threading.Thread(target=poll); worker.start()
                start = time.perf_counter(); due = start; sent = 0; times = []; bursts = []
                while time.perf_counter() < start+30:
                    now = time.perf_counter()
                    if now < due:
                        time.sleep(max(0, due-now-.0004)); continue
                    times.append(now-start); send(templates); bursts.append((time.perf_counter()-now)*1000)
                    sent += 1
                    due = max(due + 1/fps, now + 1/fps)
                elapsed = time.perf_counter()-start
                done.set(); worker.join()
                time.sleep(.15)
                after = api('status')
                delta = {k: after[k]-before[k] for k in KEYS}
                case = {'profile': label, 'requested_fps': fps, 'config': config, 'before': before,
                        'after': after, 'delta': delta, 'frames_sent': sent,
                        'packets_sent': sent*len(templates), 'elapsed_s': elapsed,
                        'send_fps': sent/elapsed, 'receive_fps': delta['artnet_complete']/elapsed,
                        'output_fps': (delta['dma_completed']-delta['blackouts_submitted'])/elapsed,
                        'send_times_s': times, 'burst_ms': bursts, 'samples': samples, 'http_errors': errors}
                case['receive_complete'] = delta['artnet_complete']==sent and delta['artnet_packets']==case['packets_sent'] and not any(delta[k] for k in ['udp_queue_drops','artnet_incomplete','artnet_rejected','artnet_ignored','artnet_stale','artnet_duplicates'])
                evidence['cases'].append(case); save()
                print(json.dumps({k: case[k] for k in ['profile','requested_fps','frames_sent','receive_fps','output_fps','receive_complete']}), flush=True)
    except BaseException as exc:
        evidence['error'] = repr(exc)
        raise
    finally:
        try:
            configure(original)
            if initial['state'] == 'STOPPED': stop()
            elif api('status')['state'] != 'ARTNET_RUNNING':
                stop(); api('start', {})
            evidence['restored_config'] = api('config')
            evidence['final_status'] = api('status')
            evidence['restored'] = evidence['restored_config'] == original
        except Exception as exc:
            evidence['restore_error'] = repr(exc)
        save(); sock.close(); ctypes.windll.winmm.timeEndPeriod(1)

if __name__ == '__main__':
    main()
