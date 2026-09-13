"""Exercise real once/loop RGB output on the user-authorized Teensy build.

Uses the saved brightness (firmware caps tests at 36/255); changes no config.
This checks controller state/DMA and UI contract, not optical wiring identity.
"""
import argparse
import json
from pathlib import Path
import time
from urllib.error import HTTPError
from urllib.request import Request, urlopen


def main(args):
    if args.report.exists():
        raise RuntimeError('Report already exists')
    evidence = {'passed': False, 'rgb_output_exercised': True,
                'physical_outputs_verified': False, 'checks': {}}
    base = 'http://' + args.ip

    def request(path, body=None):
        req = Request(base + path, data=None if body is None else json.dumps(body).encode(),
                      headers={'Content-Type': 'application/json'})
        with urlopen(req, timeout=4) as response:
            return json.load(response)

    def wait(name, predicate, timeout=5):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            status = request('/api/status')
            if predicate(status):
                evidence['checks'][name] = status
                return status
            time.sleep(.1)
        raise RuntimeError(f'{name}: timeout; {status}')

    def pattern(action, output=0):
        return request('/api/test-pattern', {'action': action, 'output': output})

    def on(s):
        return s['state'] == 'ARTNET_RUNNING' and s['armed'] and not s['test_pattern']

    try:
        evidence['config_before'] = request('/api/config')
        before = wait('initial_on', on)
        assert before['build_revision'] == 'orbital-port-tests-20260913'
        for body in [{'action': 'start', 'output': 9}, {'action': 'start', 'output': 8},
                     {'action': 'loop', 'output': -1}, {'action': 'start', 'output': 1.5},
                     {'action': 'bad', 'output': 1}, {'action': 'loop'}, {'output': 1}]:
            try:
                request('/api/test-pattern', body)
                raise AssertionError(f'Accepted invalid test: {body}')
            except HTTPError as error:
                assert error.code in (400, 409)
            assert on(request('/api/status'))
        evidence['invalid_requests_rejected'] = 7

        # Per-port starts use physical numbering. Switching ports replaces the
        # test after black DMA while preserving the previous Art-Net intent.
        for port in range(1, 8):
            pattern('start', port)
            status = wait(f'port_{port}', lambda s: s['state'] == 'TEST_RUNNING' and
                          s['test_pattern'] and s['selection'] == port and not s['test_loop'])
            assert status['test_resume_artnet'] and status['test_limit_ms'] == 20000
        for name in ['all-red-chase', 'all-green-chase', 'all-blue-chase', 'final-blackout']:
            wait('once_' + name, lambda s: s['test_phase'] == name, timeout=8)
        once = wait('once_returns_to_artnet', on)
        assert once['test_duration_ms'] >= 20000

        pattern('loop', 0)
        first = wait('all_loop_starts', lambda s: s['state'] == 'TEST_RUNNING' and s['test_loop'])
        assert first['selection'] == 0 and first['test_limit_ms'] == 0
        second = wait('loop_repeats_after_20s', lambda s: s['test_loop'] and
                      s['test_duration_ms'] >= 21100 and s['test_phase'] == 'all-red-chase', timeout=24)
        assert second['frames_submitted'] > first['frames_submitted'] + 200
        assert second['dma_completed'] > first['dma_completed'] + 200
        pattern('stop')
        wait('test_end_returns_to_artnet', on)

        # Global Stop overrides a queued start and keeps outputs stopped.
        pattern('loop', 6)
        request('/api/stop', {})
        wait('global_stop_cancels_test', lambda s: s['state'] == 'STOPPED' and
             not s['armed'] and not s['test_pattern'] and s['black_latched'])
        pattern('start', 6)
        stopped_test = wait('test_from_stopped', lambda s: s['state'] == 'TEST_RUNNING')
        assert not stopped_test['test_resume_artnet']
        pattern('stop')
        wait('test_end_keeps_prior_stop', lambda s: s['state'] == 'STOPPED' and not s['armed'])
        request('/api/start', {})
        wait('final_on', on)
        evidence['config_after'] = request('/api/config')
        assert evidence['config_after'] == evidence['config_before']
        evidence['passed'] = True
    except Exception as error:
        evidence['error'] = repr(error)
        try:
            request('/api/stop', {})
        except Exception:
            pass
        raise
    finally:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(evidence, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({'passed': True, 'checks': list(evidence['checks']),
                      'invalid_requests_rejected': evidence['invalid_requests_rejected']}, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ip', required=True)
    parser.add_argument('--report', required=True, type=Path)
    main(parser.parse_args())
