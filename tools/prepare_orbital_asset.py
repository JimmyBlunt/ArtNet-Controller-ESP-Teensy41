"""Encode the selected original for embedded HTTP without cropping/repainting it.

Requires Pillow (created with 12.2.0). CSS handles positioning and 100% strength.
"""
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parents[1]
source = root / 'output/imagegen/artnet-backgrounds/round-2/03-orbital-prism.png'
target = root / 'firmware/teensy41_artnet/web/orbital-prism.webp'
with Image.open(source) as image:
    assert image.size == (1672, 941)
    image.save(target, format='WEBP', quality=92, method=6)
print(f'{target}: {target.stat().st_size} bytes')
