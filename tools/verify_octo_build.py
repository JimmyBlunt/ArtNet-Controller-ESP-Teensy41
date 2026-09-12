"""Inspect an Octo ELF without flashing hardware; emit reproducible build evidence."""
import argparse
import hashlib
import json
import re
from pathlib import Path
import subprocess


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect(build_dir, nm):
    environment = build_dir.name
    if environment not in ('teensy41_octo_identify_rx32', 'teensy41_octo_web_rx32'):
        raise ValueError('Unrecognized Octo build directory')
    elf, hexfile = build_dir / 'firmware.elf', build_dir / 'firmware.hex'
    symbols = subprocess.check_output([str(nm), '-S', str(elf)], text=True)
    rx_buffers = [int(s.split()[1], 16) for s in symbols.splitlines() if 's_rxBufs' in s]
    rx_ring = [int(s.split()[1], 16) for s in symbols.splitlines() if 's_rxRing' in s]
    if rx_buffers != [32 * 1536] or rx_ring != [32 * 32]:
        raise ValueError(f'RX32 missing: buffers={rx_buffers}, ring={rx_ring}')
    if 'ChannelEngineObjectFLED' not in symbols or 'OctoWS2811' in symbols:
        raise ValueError('Expected modern ObjectFLED Channel engine without OctoWS2811')
    if b'PJRC_OCTO_ADAPTER_T41' not in elf.read_bytes():
        raise ValueError('Named Octo profile missing from ELF')
    root = Path(__file__).resolve().parents[1]
    fw = root / 'firmware/teensy41_artnet'
    profile_text = (fw / 'include/esp_output_profile.h').read_text(encoding='utf-8')
    output_profile = re.search(r'kId\[\]\s*=\s*"([^"]+)"', profile_text).group(1)
    if output_profile.encode('ascii') not in elf.read_bytes():
        raise ValueError('ELF does not contain the current output profile revision')
    sources = sorted(p for folder in ('src', 'include', 'tools') for p in (fw / folder).rglob('*')
                     if p.suffix in ('.cpp', '.h', '.py'))
    sources += [fw / 'platformio.ini', root / 'firmware/include/board_profiles/PjrcOctoAdapterT41.h']
    if environment == 'teensy41_octo_web_rx32':
        sources += sorted((fw / 'web').glob('*'))
    return {
        'environment': environment,
        'board_profile': 'PJRC_OCTO_ADAPTER_T41',
        'output_profile': output_profile,
        'rx_buffer_bytes': rx_buffers[0], 'rx_descriptor_bytes': rx_ring[0],
        'modern_objectfled_channel_engine': True, 'octows2811_linked': False,
        'sources_sha256': {str(p.relative_to(root)): digest(p) for p in sources},
        'artifacts': {p.suffix[1:]: {'path': str(p.resolve()), 'sha256': digest(p)} for p in (elf, hexfile)},
        'hardware_flashed': False, 'physical_outputs_verified': False,
        'note': 'Post-build inspection; source hashes are a record, not proof of a reproducible binary.',
    }


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--nm', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = inspect(args.build_dir, args.nm)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({key: value for key, value in result.items() if key not in ('sources_sha256', 'artifacts')}, indent=2))
