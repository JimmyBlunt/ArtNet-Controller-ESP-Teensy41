"""Check imported source provenance, documented environments and shared artwork."""
import configparser
import hashlib
import json
from pathlib import Path


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify(root):
    esp = root / 'firmware/esp32_artnet'
    manifest = json.loads((esp / 'IMPORT_MANIFEST.json').read_text(encoding='utf-8'))
    adaptations = json.loads((esp / 'IMPORT_ADAPTATIONS.json').read_text(encoding='utf-8'))
    changed = {entry['path']: entry for entry in adaptations['files']}
    for entry in manifest['files']:
        path = (esp / entry['path']).resolve()
        if not path.is_relative_to(esp.resolve()):
            raise ValueError('Manifest path escapes project')
        expected = changed.get(entry['path'], entry)['sha256']
        if digest(path) != expected:
            raise ValueError(f"Source changed since recorded import/adaptation: {entry['path']}")
    legacy = root / 'reference/teensy-apa102-legacy'
    for entry in json.loads((legacy / 'MANIFEST.json').read_text())['files']:
        if digest(legacy / entry['path']) != entry['sha256']:
            raise ValueError(f"Legacy snapshot changed: {entry['path']}")
    artwork = root / 'firmware/teensy41_artnet/web/orbital-prism.webp'
    if digest(artwork) != digest(esp / 'web/assets/orbital-prism.webp'):
        raise ValueError('ESP and Teensy artwork differ')
    overview = (root / 'docs/NODE_BUILDS.md').read_text(encoding='utf-8')
    environments = {}
    for project in ('esp32_artnet', 'teensy41_artnet'):
        folder = root / 'firmware' / project
        config = configparser.ConfigParser(interpolation=None)
        config.read(folder / 'platformio.ini', encoding='utf-8')
        names = [section[4:] for section in config.sections() if section.startswith('env:')]
        for name in names:
            if f'`{name}`' not in overview:
                raise ValueError(f'Undocumented environment: {name}')
        for required in ('platformio.ini', 'build.ps1', 'requirements-build.txt'):
            if not (folder / required).is_file():
                raise ValueError(f'Missing build input: {project}/{required}')
        environments[project] = names
    if (esp / 'firmware/include/WifiSecrets.h').exists():
        # A local private file is allowed, but must not enter the Git index.
        import subprocess
        tracked = subprocess.check_output(['git', 'ls-files', '--',
            'firmware/esp32_artnet/firmware/include/WifiSecrets.h'], cwd=root, text=True)
        if tracked.strip():
            raise ValueError('Private WiFi header is tracked')
    return {'imported_files_verified': len(manifest['files']),
            'documented_adaptations': len(changed), 'shared_artwork_sha256': digest(artwork),
            'environments': environments}


if __name__ == '__main__':
    print(json.dumps(verify(Path(__file__).resolve().parents[1]), indent=2))
