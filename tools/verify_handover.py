"""Read-only SHA-256 verification of the original RX32 handover snapshot."""
import hashlib
import json
from pathlib import Path


def verify(root):
    root = root.resolve()
    manifest = json.loads((root / 'MANIFEST.json').read_text(encoding='utf-8-sig'))
    for entry in manifest['files']:
        path = (root / entry['path']).resolve()
        if not path.is_relative_to(root):
            raise ValueError(f"Manifest path outside snapshot: {entry['path']}")
        data = path.read_bytes()
        if len(data) != entry['bytes'] or hashlib.sha256(data).hexdigest() != entry['sha256']:
            raise ValueError(f"Manifest mismatch: {entry['path']}")
    return {'verified_files': len(manifest['files']),
            'firmware_commit_claimed_by_manifest': manifest['firmware_commit'],
            'git_object_history_included': False}


if __name__ == '__main__':
    print(json.dumps(verify(Path(__file__).resolve().parents[1] /
                           'reference/teensy-rx32-385d5ed'), indent=2))
