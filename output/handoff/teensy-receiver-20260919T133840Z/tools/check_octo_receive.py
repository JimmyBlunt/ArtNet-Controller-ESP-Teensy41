"""Verify the configured 29-universe receiver on a disarmed Teensy; never ARM/TEST."""
import argparse
import json
from pathlib import Path
import socket
import struct
import time
import threading
from urllib.request import urlopen

import serial
from serial.tools import list_ports

LENGTHS = [203, 738, 880, 810, 352, 536, 512, 0]
STARTS = [120, 122, 127, 133, 139, 142, 146, 0]
PINS = [2, 14, 7, 8, 6, 20, 21, 5]


def read_status(connection, ip=None):
    if ip:
        # A synchronous HTTP response is fresh. Uncorrelated periodic USB JSON
        # may remain queued on the device after the host input buffer is reset.
        with urlopen('http://' + ip + '/api/status', timeout=3) as response:
            return json.load(response)
    connection.reset_input_buffer()
    connection.write(b'STATUS\n')
    deadline = time.monotonic() + 4
    while time.monotonic() < deadline:
        try:
            value = json.loads(connection.readline())
        except ValueError:
            continue
        if isinstance(value, dict) and 'firmware' in value:
            return value
    raise RuntimeError('No STATUS received')


def check_profile(value):
    if (value.get('source_profile') != 'ESP4031_SPLIT_20260913' or
            value.get('board_profile') != 'PJRC_OCTO_ADAPTER_T41' or
            value.get('lengths') != LENGTHS or value.get('pins') != PINS or
            value.get('start_universes') != STARTS or value.get('state') != 'DISARMED' or
            value.get('initialized') or value.get('armed')):
        raise RuntimeError('Expected exact profile, disarmed, no initialized outputs')


def packets(sequence):
    result = []
    for start, count in zip(STARTS, LENGTHS):
        for offset in range(0, count, 170):
            universe = start + offset // 170
            length = min(170, count - offset) * 3
            length += length % 2
            header = b'Art-Net\0' + struct.pack('<H', 0x5000) + struct.pack('>H', 14)
            header += bytes([sequence, 0]) + struct.pack('<H', universe) + struct.pack('>H', length)
            result.append((universe, header + bytes(length)))
    return result


def main(args):
    if args.report.exists():
        raise RuntimeError('Refusing to overwrite test evidence')
    ports = [p for p in list_ports.comports() if p.vid == 0x16c0 and p.pid == 0x0483
             and p.serial_number == args.serial]
    if len(ports) != 1:
        raise RuntimeError('Expected exactly one identified Teensy')
    evidence = {'hardware_receive_test': True, 'led_output_exercised': False,
                'physical_output_verified': False, 'frames_sent': 0, 'packets_sent': 0}
    try:
        with serial.Serial(ports[0].device, 115200, timeout=.2, write_timeout=1) as connection, \
                socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sender:
            before = read_status(connection)
            check_profile(before)
            if not before.get('link') or before.get('ip') in (None, '0.0.0.0'):
                raise RuntimeError('Native Ethernet not ready')
            http_ip = before['ip'] if before['firmware'] == 'teensy41-octo-web-rx32' else None
            if http_ip:
                before = read_status(connection, http_ip)
                check_profile(before)
            evidence['telemetry_source'] = 'synchronous HTTP' if http_ip else 'USB STATUS'
            sender.connect((before['ip'], 6454))
            evidence.update(before=before, sender_ip=sender.getsockname()[0])
            # Withhold U149, then expire the partial candidate. No complete image is legal.
            for universe, packet in packets(1):
                if universe != 149:
                    sender.send(packet)
                    evidence['packets_sent'] += 1
            time.sleep(.2)
            missing = read_status(connection, http_ip)
            if missing['artnet_complete'] != before['artnet_complete']:
                raise RuntimeError('Frame published without U149, or another sender is active')
            evidence['without_u149'] = missing
            started = time.monotonic()
            http_durations = []
            http_errors = []
            http_stop = threading.Event()
            def poll_web():
                index = 0
                while not http_stop.is_set():
                    http_started = time.monotonic()
                    path = '/app.js' if index % 10 == 0 else '/api/status'
                    try:
                        with urlopen('http://' + before['ip'] + path, timeout=2) as response:
                            if response.status != 200 or not response.read():
                                raise RuntimeError('Web response missing during Art-Net load')
                        http_durations.append(time.monotonic() - http_started)
                    except Exception as error:
                        http_errors.append(repr(error))
                    index += 1
                    http_stop.wait(max(0, .1 - (time.monotonic() - http_started)))
            http_thread = threading.Thread(target=poll_web, daemon=True) if args.web else None
            if http_thread:
                http_thread.start()
            next_frame = started
            for frame in range(args.frames):
                time.sleep(max(0, next_frame - time.monotonic()))
                frame_started = time.monotonic()
                for _, packet in packets((frame + 1) % 255 + 1):
                    sender.send(packet)
                    evidence['packets_sent'] += 1
                evidence['frames_sent'] += 1
                # Never compress delayed frames into catch-up bursts. HTTP is on
                # an independent thread and cannot pause then flood the sender.
                next_frame = max(next_frame + 1 / 30, frame_started + 1 / 30)
            time.sleep(max(0, next_frame - time.monotonic()))
            elapsed = time.monotonic() - started
            http_stop.set()
            if http_thread:
                http_thread.join(timeout=3)
            time.sleep(.1)
            after = read_status(connection, http_ip)
            check_profile(after)
            delta = {key: after[key] - before[key] for key in
                     ('udp_received', 'udp_queue_drops', 'artnet_complete', 'artnet_incomplete',
                      'artnet_rejected', 'artnet_ignored', 'frames_submitted', 'dma_completed')}
            evidence.update(after=after, delta=delta, send_elapsed_s=elapsed,
                            average_sender_fps=args.frames / elapsed)
            evidence['http_requests_during_stream'] = len(http_durations)
            evidence['http_max_response_s'] = max(http_durations, default=0)
            evidence['http_errors'] = http_errors
            if (delta['udp_received'] != evidence['packets_sent'] or
                    delta['artnet_complete'] != args.frames or delta['udp_queue_drops'] or
                    delta['artnet_rejected'] or delta['artnet_ignored'] or
                    delta['frames_submitted'] or delta['dma_completed'] or
                    delta['artnet_incomplete'] != 1 or http_errors):
                raise RuntimeError(f'Unexpected receiver counters or concurrent sender: {delta}')
            evidence['passed'] = True
    except Exception as error:
        evidence['error'] = str(error)
        raise
    finally:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(evidence, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: v for k, v in evidence.items() if k not in ('before', 'after', 'without_u149')}, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--serial', required=True)
    parser.add_argument('--frames', default=300, type=int)
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--web', action='store_true', help='Poll status and fetch web assets during UDP stream')
    args = parser.parse_args()
    if not 1 <= args.frames <= 1800:
        parser.error('--frames must be 1..1800')
    main(args)
