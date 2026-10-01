"""Deterministic, offline firmware assets. No external assets or filesystem needed."""
from pathlib import Path
import gzip


def embed(root):
    lines = ['#pragma once', '#include <Arduino.h>', '#include <stddef.h>', '#include <stdint.h>',
             'namespace web_assets {',
             'struct Asset { const char* path; const char* mime; const uint8_t* data; size_t size; };']
    entries = []
    for index, (name, mime) in enumerate((('index.html', 'text/html; charset=utf-8'),
                                         ('styles.css', 'text/css; charset=utf-8'),
                                         ('app.js', 'application/javascript; charset=utf-8'),
                                         ('orbital-prism.webp', 'image/webp'))):
        data = gzip.compress((root / 'web' / name).read_bytes(), compresslevel=9, mtime=0)
        # A dedicated .progmem.* section avoids GCC section-type collisions with
        # ArduinoJson's inline template tables; Teensy's linker keeps both in flash.
        lines.append(f'const uint8_t asset{index}[] __attribute__((section(".progmem.web_assets"))) = {{')
        lines += [','.join(str(x) for x in data[i:i+32]) + ',' for i in range(0, len(data), 32)]
        lines.append('};')
        entries.append(f'{{"/{name}", "{mime}", asset{index}, sizeof(asset{index})}}')
    lines += ['const Asset assets[] = {', ',\n'.join(entries), '};', '}']
    output = root / 'include/web_assets.h'
    generated = '\n'.join(lines) + '\n'
    if not output.exists() or output.read_text(encoding='utf-8') != generated:
        output.write_text(generated, encoding='utf-8', newline='\n')
    print(f'Embedded {len(entries)} deterministic gzip web assets in flash')


if 'Import' in globals():
    Import('env')
    embed(Path(env.subst('$PROJECT_DIR')))
elif __name__ == '__main__':
    embed(Path(__file__).resolve().parents[1])
