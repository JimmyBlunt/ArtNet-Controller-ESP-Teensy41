"""Exercise user-authorized ON boot/recovery/STOP on the flashed Octo controller.

All generated ArtDmx pixels are BLACK. This tests real submissions and DMA,
not physical LED colors or cable mapping. Configuration is never modified.
"""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import socket
import threading
import time
from urllib.request import Request, urlopen

from check_octo_receive import packets, LENGTHS, STARTS, PINS


def main(args):
    if args.report.exists():
        raise RuntimeError('Refusing to overwrite acceptance evidence')
    evidence = {'passed': False, 'payload': 'all RGB bytes zero',
                'physical_outputs_verified': False, 'checks': {}}
    base = 'http://' + args.ip

    def request(path, body=None):
        req = Request(base + path, data=None if body is None else json.dumps(body).encode(),
                      headers={'Content-Type': 'application/json'})
        with urlopen(req, timeout=4) as response:
            return json.load(response)

    def snapshot(name):
        status = request('/api/status')
        evidence['checks'][name] = status
        return status

    def wait_for(predicate, timeout=5):
        until = time.monotonic() + timeout
        last = None
        while time.monotonic() < until:
            try:
                last = request('/api/status')
                if predicate(last):
                    return last
            except (OSError, ValueError):
                pass
            time.sleep(.06)
        raise RuntimeError(f'Status timeout: {last}')

    def on(status):
        return (status.get('boot_mode') == 'ARTNET_ON' and status.get('armed') and
                status.get('initialized') and status.get('state') == 'ARTNET_RUNNING')

    def stop():
        request('/api/stop', {})
        return wait_for(lambda s: s['state'] == 'STOPPED' and not s['armed'] and
                        s['black_latched'] and not s['dma_pending'])

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sender:
        sender.connect((args.ip, 6454))

        def send(sequence, omit=None):
            for universe, packet in packets(sequence):
                if universe != omit:
                    sender.send(packet)

        try:
            before_config = request('/api/config')
            evidence['configuration_before'] = before_config
            before = snapshot('boot_on')
            assert on(before) and before['artnet_waiting'] and before['black_latched']
            assert before['lengths'] == LENGTHS and before['pins'] == PINS
            assert before['start_universes'] == STARTS
            assert before['build_revision'] == args.revision
            # Confirm a quiet sender interval before injecting an isolated stream.
            time.sleep(1.2)
            quiet = snapshot('delayed_sender_wait')
            assert on(quiet) and quiet['artnet_waiting']
            assert quiet['artnet_packets'] == before['artnet_packets'], 'Another sender is active'
            assert quiet['frames_submitted'] == before['frames_submitted']

            web_result = {}
            def load_background():
                try:
                    started = time.monotonic()
                    with urlopen(base + '/orbital-prism.webp', timeout=4) as response:
                        raw = response.read()
                        decoded = gzip.decompress(raw) if response.headers.get('Content-Encoding') == 'gzip' else raw
                        web_result.update(bytes=len(decoded), seconds=time.monotonic()-started,
                                          sha256=hashlib.sha256(decoded).hexdigest())
                    local = Path(__file__).resolve().parents[1] / 'firmware/teensy41_artnet/web/orbital-prism.webp'
                    assert decoded == local.read_bytes()
                except Exception as error:
                    web_result['error'] = repr(error)

            web_thread = threading.Thread(target=load_background)
            web_thread.start()
            for sequence in range(1, 61):
                started = time.monotonic()
                send(sequence)
                time.sleep(max(0, 1/30 - (time.monotonic()-started)))
            web_thread.join(timeout=5)
            assert not web_thread.is_alive() and 'error' not in web_result
            evidence['background_during_output'] = web_result
            playing = snapshot('sender_active')
            assert on(playing) and not playing['artnet_waiting']
            assert playing['artnet_complete'] - quiet['artnet_complete'] == 60
            assert playing['artnet_packets'] - quiet['artnet_packets'] == 60*29
            assert playing['udp_queue_drops'] == quiet['udp_queue_drops']
            assert playing['frames_submitted'] > quiet['frames_submitted'] + 1
            assert playing['dma_completed'] > quiet['dma_completed']

            lost = wait_for(lambda s: on(s) and s['artnet_waiting'] and s['black_latched'])
            evidence['checks']['sender_lost_still_on'] = lost
            time.sleep(.3)
            assert request('/api/status')['blackouts_submitted'] == lost['blackouts_submitted']
            send(80, omit=149)
            time.sleep(.2)
            partial = snapshot('partial_frame_ignored')
            assert partial['frames_submitted'] == lost['frames_submitted']
            assert partial['artnet_complete'] == lost['artnet_complete']
            send(81)
            resumed = wait_for(lambda s: on(s) and not s['artnet_waiting'] and
                               s['frames_submitted'] > partial['frames_submitted'])
            evidence['checks']['automatic_recovery'] = resumed

            stopped = stop()
            evidence['checks']['manual_stop'] = stopped
            for sequence in range(90, 95):
                send(sequence)
                time.sleep(.04)
            time.sleep(1.2)
            held = snapshot('manual_stop_holds_with_data')
            assert held['state'] == 'STOPPED' and not held['armed']
            assert held['frames_submitted'] == stopped['frames_submitted']
            request('/api/start', {})
            restarted = wait_for(lambda s: on(s) and s['artnet_waiting'] and s['black_latched'])
            assert restarted['frames_submitted'] == held['frames_submitted'] + 1
            send(100)
            evidence['checks']['explicit_restart'] = wait_for(lambda s: on(s) and not s['artnet_waiting'])

            stop()
            request('/api/reboot', {})
            time.sleep(1.5)
            evidence['checks']['reboot_restores_on'] = wait_for(
                lambda s: on(s) and s['artnet_waiting'] and s['black_latched'] and
                s['frames_submitted'] == 1 and s['artnet_packets'] == 0, timeout=20)
            evidence['configuration_after'] = request('/api/config')
            assert evidence['configuration_after'] == before_config
            evidence['passed'] = True
        except Exception as error:
            evidence['error'] = repr(error)
            raise
        finally:
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(json.dumps(evidence, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({'passed': evidence['passed'], 'checks': list(evidence['checks']),
                      'background': evidence['background_during_output']}, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ip', required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--revision', default='orbital-prism-autostart-20260913')
    main(parser.parse_args())
