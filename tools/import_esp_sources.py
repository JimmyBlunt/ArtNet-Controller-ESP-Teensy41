"""Import the build-related ESP working tree without credentials or caches.

Creates a new destination only. Existing imports are never overwritten.
The manifest records uncommitted files as well as the source HEAD.
"""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import re
import subprocess


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def run(source, destination):
    source, destination = source.resolve(), destination.resolve()
    if destination.exists():
        raise ValueError('Destination already exists; reconcile changes explicitly')
    if not (source / 'platformio.ini').is_file():
        raise ValueError('Expected ESP PlatformIO project')
    head = subprocess.check_output(['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip()
    tree = subprocess.check_output(['git', '-C', str(source), 'ls-tree', '-r', '-z', 'HEAD'])
    tracked = {}
    for record in tree.split(b'\0'):
        if record:
            meta, name = record.split(b'\t', 1)
            tracked[name.decode('utf-8')] = meta.split()[2].decode()
    paths = set()
    for folder in ('firmware', 'web', 'config', 'tests', 'processing', 'scripts'):
        paths.update(p for p in (source / folder).rglob('*') if p.is_file())
    paths.update(p for p in (source / 'docs').rglob('*') if p.suffix in ('.md', '.json'))
    paths.update(p for p in (source / 'tools').glob('*') if p.suffix in ('.js', '.py'))
    paths.update(source / name for name in ('platformio.ini', 'Makefile', 'README.md', 'STATUS.md', '.gitignore'))
    forbidden_names = {'WifiSecrets.h', '.env', 'secrets.h', 'credentials.json'}
    forbidden_parts = {'.git', '.pio', '.toolchain', '__pycache__', 'node_modules', 'build'}
    excluded = []
    selected = []
    for path in sorted(paths):
        relative = path.relative_to(source)
        if (path.name in forbidden_names or forbidden_parts.intersection(relative.parts)
                or path.suffix.lower() in ('.exe', '.bin', '.elf', '.pyc', '.pem', '.key')):
            excluded.append(relative.as_posix())
            continue
        if path.is_symlink():
            raise ValueError(f'Symlink needs manual review: {relative}')
        selected.append((path, relative))
    entries = []
    private_values = []
    private_header = source / 'firmware/include/WifiSecrets.h'
    if private_header.exists():
        private_values = [value.encode('utf-8') for value in re.findall(
            r'^\s*#define\s+SCHRANK_WIFI_(?:SSID|PASSWORD)\s+"([^"\n]+)"',
            private_header.read_text(encoding='utf-8'), re.M)]
    for path, relative in selected:
        data = path.read_bytes()
        blob = hashlib.sha1(f'blob {len(data)}\0'.encode() + data).hexdigest()
        imported = data
        for private in private_values:
            imported = imported.replace(private, b'[PRIVATE WIFI VALUE REMOVED]')
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(imported)
        if sha256(target.read_bytes()) != sha256(imported):
            raise ValueError(f'Copy mismatch: {relative}')
        entries.append({'path': relative.as_posix(), 'bytes': len(imported), 'sha256': sha256(imported),
                        'source_sha256': sha256(data), 'private_values_redacted': imported != data,
                        'source_head_blob': tracked.get(relative.as_posix()),
                        'matches_source_head': tracked.get(relative.as_posix()) == blob})
    manifest = {'captured_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                'source_directory': str(source), 'source_head': head,
                'kind': 'working-tree import, including modified and untracked build inputs',
                'excluded_files': excluded,
                'excluded_categories': ['private WiFi credentials', 'flash/NVS backups and ESP binaries',
                                        'toolchain/dependency caches', 'unrelated media and sender assets'],
                'files': entries}
    (destination / 'IMPORT_MANIFEST.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'copied_files': len(entries), 'different_from_source_head': sum(not e['matches_source_head'] for e in entries),
                      'excluded_files': excluded, 'source_head': head}, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--destination', type=Path, required=True)
    args = parser.parse_args()
    run(args.source, args.destination)
