"""Flash only the inspected Octo artifact onto one freshly identified Teensy 4.1.

No ARM or TEST is sent. The resulting firmware must boot disarmed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time

import serial
from serial.tools import list_ports

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def identified_port(number):
    matches = [p for p in list_ports.comports()
               if p.vid == 0x16c0 and p.pid == 0x0483 and p.serial_number == number]
    if len(matches) != 1:
        raise RuntimeError(f'Expected exactly one serial-matched Teensy, found {len(matches)}')
    return matches[0].device


def halfkay_ids():
    command = "@(Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -like 'USB\\VID_16C0&PID_0478\\*' } | Select-Object -ExpandProperty InstanceId) | ConvertTo-Json -Compress"
    result = subprocess.run(['powershell', '-NoProfile', '-Command', command],
                            capture_output=True, text=True, timeout=15, check=True)
    value = json.loads(result.stdout) if result.stdout.strip() else []
    return [value] if isinstance(value, str) else value


def require_halfkay(expected):
    ids = halfkay_ids()
    if len(ids) != 1 or not ids[0].upper().endswith('\\' + expected.upper()):
        raise RuntimeError(f'Expected only HalfKay {expected}, got {ids}')
    return ids


def status(port, seconds=4):
    with serial.Serial(port, 115200, timeout=.2, write_timeout=1) as connection:
        connection.write(b'STATUS\n')
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            try:
                value = json.loads(connection.readline())
            except (ValueError, UnicodeDecodeError):
                continue
            if isinstance(value, dict) and 'firmware' in value:
                return value
    raise RuntimeError('No firmware STATUS received')


def main(args):
    if args.report.exists():
        raise RuntimeError('Refusing to overwrite flash evidence')
    manifest = json.loads(args.manifest.read_text(encoding='utf-8'))
    if (manifest.get('environment') != 'teensy41_octo_identify_rx32' or
            manifest.get('board_profile') != 'PJRC_OCTO_ADAPTER_T41' or
            not manifest.get('modern_objectfled_channel_engine') or manifest.get('octows2811_linked') or
            manifest.get('rx_buffer_bytes') != 49152 or manifest.get('rx_descriptor_bytes') != 1024):
        raise RuntimeError('Not an inspected modern Octo RX32 build')
    for name, expected in manifest['sources_sha256'].items():
        path = (ROOT / name).resolve()
        if not path.is_relative_to(ROOT) or sha(path) != expected:
            raise RuntimeError(f'Source differs from inspected build: {name}')
    for artifact in manifest['artifacts'].values():
        if sha(Path(artifact['path'])) != artifact['sha256']:
            raise RuntimeError('Artifact SHA mismatch')
    port = identified_port(args.serial)
    before = status(port)
    if before.get('armed') or before.get('state') in ('ARTNET_RUNNING', 'TEST_RUNNING'):
        raise RuntimeError('Stop the running controller before flashing')
    evidence = {'serial': args.serial, 'port_before': port, 'before': before,
                'manifest_sha256': sha(args.manifest), 'attempts': [], 'flash_complete': False}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    try:
        if halfkay_ids():
            raise RuntimeError('Unexpected HalfKay already present before target reboot')
        try:
            with serial.Serial(port, 134, timeout=.2, write_timeout=1):
                pass
        except serial.SerialException:
            pass  # Identified target may disappear during re-enumeration.
        deadline = time.monotonic() + 25
        while time.monotonic() < deadline:
            if halfkay_ids():
                break
            time.sleep(.3)
        evidence['bootloader_ids'] = require_halfkay(args.halfkay)
        for _ in range(2):
            require_halfkay(args.halfkay)
            result = subprocess.run([str(args.loader), '--mcu=TEENSY41', '-v',
                                     manifest['artifacts']['hex']['path']],
                                    capture_output=True, text=True, timeout=45)
            evidence['attempts'].append({'returncode': result.returncode,
                                        'output': result.stdout + result.stderr})
            if result.returncode == 0:
                evidence['flash_complete'] = True
                break
        if not evidence['flash_complete']:
            raise RuntimeError('Upload failed; inspected artifact and evidence retained')
        deadline = time.monotonic() + 25
        while time.monotonic() < deadline:
            try:
                after = status(identified_port(args.serial))
                if after.get('board_profile') == 'PJRC_OCTO_ADAPTER_T41':
                    break
            except (RuntimeError, serial.SerialException):
                pass
            time.sleep(.3)
        else:
            raise RuntimeError('Octo firmware did not report after flashing')
        evidence['after'] = after
        if after.get('state') != 'DISARMED' or after.get('initialized'):
            raise RuntimeError('Expected boot disarmed with no LED initialization')
        evidence['verified_disarmed_boot'] = True
    except Exception as error:
        evidence['error'] = str(error)
        raise
    finally:
        args.report.write_text(json.dumps(evidence, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(evidence['after'], indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', required=True, type=Path)
    parser.add_argument('--serial', required=True)
    parser.add_argument('--halfkay', required=True, help='Previously verified target HalfKay ID')
    parser.add_argument('--loader', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    main(parser.parse_args())
