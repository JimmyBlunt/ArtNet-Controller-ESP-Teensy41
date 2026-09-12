"""Live HTTP/config/storage checks. Never starts LED output; restores initial config."""
import argparse
import copy
import gzip
import json
from pathlib import Path
import socket
import time
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


def main(args):
    if args.report.exists():
        raise RuntimeError('Evidence already exists')
    evidence = {'ip': args.ip, 'led_output_exercised': False, 'checks': []}
    baseline = None
    changed = False

    def api(path, value=None, expected=200):
        body = None if value is None else json.dumps(value, separators=(',', ':')).encode()
        request = Request(f'http://{args.ip}{path}', body,
                          headers={} if body is None else {'Content-Type': 'application/json'})
        try:
            result = urlopen(request, timeout=4)
        except HTTPError as error:
            result = error
        with result:
            data = json.loads(result.read())
            if result.status != expected:
                raise AssertionError((path, result.status, expected, data))
            return data

    def stopped():
        status = api('/api/status')
        assert status['state'] == 'DISARMED' and not status['initialized'] and not status['armed'], status
        assert status['frames_submitted'] == 0 and status['dma_completed'] == 0, status
        return status

    def reboot():
        api('/api/reboot', {})
        time.sleep(1.3)
        deadline = time.monotonic() + 25
        while time.monotonic() < deadline:
            try:
                status = stopped()
                if not status.get('reboot_pending') and status['ip'] == args.ip:
                    return status
            except (OSError, URLError, AssertionError):
                pass
            time.sleep(.25)
        raise RuntimeError('Reboot did not return on the same IP')

    def raw(data):
        with socket.create_connection((args.ip, 80), timeout=3) as client:
            client.sendall(data)
            result = bytearray()
            while True:
                try:
                    part = client.recv(8192)
                except ConnectionResetError:
                    if not result:
                        raise
                    break
                if not part:
                    break
                result.extend(part)
                if b'\r\n\r\n' in result:
                    headers, body = bytes(result).split(b'\r\n\r\n', 1)
                    lengths = [line.split(b':', 1)[1].strip() for line in headers.split(b'\r\n')
                               if line.lower().startswith(b'content-length:')]
                    if lengths and len(body) == int(lengths[0]):
                        break
            return bytes(result)

    try:
        evidence['before'] = stopped()
        baseline = api('/api/config')
        evidence['baseline'] = baseline
        assert baseline['hardwareProfile'] == 'PJRC_OCTO_ADAPTER_T41'
        assert [x['dataPin'] for x in baseline['outputs']] == [2, 14, 7, 8, 6, 20, 21, 5]
        assert [x['pixelCount'] for x in baseline['outputs']] == [203, 738, 880, 810, 352, 536, 512, 0]
        assert baseline['pixelCount'] == 4031 and baseline['universeCount'] == 29
        for path, marker in [('/', b'LED control desk'), ('/app.js', b'PJRC_OCTO_ADAPTER_T41'), ('/styles.css', b'.side-rail')]:
            with urlopen(f'http://{args.ip}{path}', timeout=4) as response:
                data = response.read()
                if response.headers.get('Content-Encoding') == 'gzip':
                    data = gzip.decompress(data)
                assert marker in data, path
                evidence['checks'].append(f'asset {path}: {len(data)} uncompressed bytes')
        mutations = [
            ('wrong hardware profile', lambda c: c.update(hardwareProfile='FLEX8')),
            ('wrong physical GPIO', lambda c: c['outputs'][1].update(dataPin=3)),
            ('unsupported output type', lambda c: c['outputs'][0].update(type='APA102')),
            ('overlapping universes', lambda c: c['outputs'][1].update(startUniverse=120)),
            ('oversized output', lambda c: c['outputs'][5].update(pixelCount=1201)),
            ('fractional FPS', lambda c: c.update(targetFps=29.5)),
            ('numeric string brightness', lambda c: c.update(brightness='8')),
            ('missing output', lambda c: c['outputs'].pop()),
        ]
        for name, mutate in mutations:
            candidate = copy.deepcopy(baseline)
            mutate(candidate)
            response = api('/api/config', candidate, 400)
            assert response['ok'] is False and api('/api/config') == baseline
            evidence['checks'].append(name + ': rejected, configuration unchanged')
        api('/api/test', {'output': 8, 'seconds': 1}, 409)  # Disabled; emits nothing.
        api('/api/test', {'output': 9, 'seconds': 1}, 400)
        api('/api/stop', {})
        stopped()
        header = b'POST /api/stop HTTP/1.1\r\nHost: teensy\r\nContent-Type: application/json\r\n'
        for label, request, status in [
            ('duplicate length', header + b'Content-Length: 2\r\nContent-Length: 2\r\n\r\n{}', 400),
            ('oversized length', header + b'Content-Length: 6145\r\n\r\n' + b' ' * 6145, 413),
            ('chunked framing', header + b'Transfer-Encoding: chunked\r\n\r\n0\r\n\r\n', 400),
            ('trailing JSON', header + b'Content-Length: 4\r\n\r\n{}{}', 400),
        ]:
            answer = raw(request)
            assert answer.startswith(f'HTTP/1.1 {status} '.encode()), (label, answer)
            evidence['checks'].append(label + ': rejected')
        # A slow incomplete request must neither block the second client nor live indefinitely.
        with socket.create_connection((args.ip, 80), timeout=3) as slow:
            slow.sendall(b'GET /api/status HTTP/1.1\r\nHost: teen')
            start = time.monotonic()
            stopped()
            latency = time.monotonic() - start
            assert latency < 1, latency
            time.sleep(1.8)
            try:
                assert slow.recv(100) == b''
            except ConnectionResetError:
                pass
            evidence['checks'].append(f'slow client isolated and timed out; second request {latency:.3f}s')
        # RAM-only changes do not touch LEDs. Save/reboot exercises real EEPROM.
        candidate = copy.deepcopy(baseline)
        candidate['brightness'] = 7 if baseline['brightness'] != 7 else 8
        changed = True
        api('/api/config', candidate)
        assert api('/api/config') == candidate
        assert stopped()['unsaved']
        api('/api/reboot', {}, 409)  # Pending unsaved changes cannot be lost silently.
        api('/api/save', {})
        assert not stopped()['unsaved']
        evidence['persisted_status'] = reboot()
        assert api('/api/config') == candidate
        evidence['checks'].append('brightness saved in real EEPROM and recovered after reboot; no LED initialization')
        api('/api/config', baseline)
        api('/api/save', {})
        evidence['after'] = reboot()
        assert api('/api/config') == baseline
        changed = False
        evidence['checks'].append('original configuration restored, saved and verified after second reboot')
        evidence['passed'] = True
    except Exception as error:
        evidence['error'] = repr(error)
        raise
    finally:
        if changed and baseline:
            try:
                stopped()
                api('/api/config', baseline)
                api('/api/save', {})
                evidence['restored_on_failure'] = api('/api/config') == baseline
            except Exception as error:
                evidence['restoration_error'] = repr(error)
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(evidence, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: v for k, v in evidence.items() if k not in ('before', 'after', 'baseline', 'persisted_status')}, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ip', required=True)
    parser.add_argument('--report', required=True, type=Path)
    main(parser.parse_args())
