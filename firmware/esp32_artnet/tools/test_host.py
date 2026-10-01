"""Run existing ESP C++ and non-browser JS tests without contacting devices."""
import argparse
import json
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='g++')
    parser.add_argument('--node', default='node')
    parser.add_argument('--include-historical', action='store_true',
                        help='Also run the known-failing esp251 profile test; failures stay failures')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    os.chdir(root)
    output = root / 'build'
    output.mkdir(exist_ok=True)
    base = ['firmware/src/' + name + '.cpp' for name in
            ('ConfigManager', 'OutputRouter', 'Performance', 'LogicalPixelBuffer')]
    tests = [
        ('test_firmware_core', [], [str(p.relative_to(root)) for p in sorted((root / 'firmware/src').glob('*.cpp'))]),
        ('test_network_boot_guard', ['-Itests/stubs/network', '-DARDUINO'], ['firmware/src/NetworkManager.cpp']),
        ('test_flexible_output_profile', ['-DLED_PROFILE_FLEX8'], base),
        ('test_extension_board_profile', ['-DLED_PROFILE_FLEX8', '-DLED_PROFILE_EXTENSION_BOARD'], base),
    ]
    if args.include_historical:
        tests.append(('test_esp251_profile', ['-DLED_PROFILE_FLEX8', '-DLED_PROFILE_EXTENSION_BOARD', '-DLED_PROFILE_ESP251'],
                      base + ['firmware/src/UniverseAssembler.cpp']))
    results = []

    def run(name, command):
        result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        print(f'{name}: exit={result.returncode}\n{result.stdout}', flush=True)
        results.append({'name': name, 'command': command, 'returncode': result.returncode, 'output': result.stdout})
        return result.returncode == 0

    run('embedded_ui', [args.node, 'tools/build-web-ui.js', '--check'])
    for name, flags, sources in tests:
        exe = output / (name + ('.exe' if os.name == 'nt' else ''))
        # Stub includes must precede private firmware includes, even on a developer checkout.
        command = [args.cxx, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-O2', *flags,
                   '-Ifirmware/include', *sources, f'tests/{name}.cpp', '-o', str(exe)]
        if run(name + '_compile', command):
            run(name, [str(exe)])
    for name in ('test_mapping_schema', 'test_web_calculations', 'test_processing_protocol',
                 'test_processing_visualizer', 'test_hardware_benchmark_tool'):
        run(name, [args.node, f'tests/{name}.js'])
    (output / 'host-test-results.json').write_text(json.dumps(results, indent=2) + '\n', encoding='utf-8')
    return 0 if all(r['returncode'] == 0 for r in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
