"""Archive and flash the RX32 image only onto the USB-identified unwired Teensy."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time
import serial
from serial.tools import list_ports

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT.parents[1]/'docs/performance-program-20260912/teensy'
BUILD=Path('C:/codex-tools/teensy41-build/teensy41_unwired_bench')
TOOLS=Path('C:/codex-tools/teensy41-pio/packages')


def main(retry=False):
    label='rx32'
    manifest_path=OUT/'rx32-build-flash.json'
    if retry:
        manifest=json.loads(manifest_path.read_text())
        if manifest.get('flash_returncode')==0:raise RuntimeError('Original flash already succeeded')
        for item in manifest['artifacts'].values():
            assert hashlib.sha256(Path(item['path']).read_bytes()).hexdigest()==item['sha256']
        command="@(Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -like 'USB\\VID_16C0&PID_0478\\*' } | Select-Object -ExpandProperty InstanceId) | ConvertTo-Json -Compress"
        probe=subprocess.run(['powershell','-NoProfile','-Command',command],capture_output=True,text=True,timeout=15)
        ids=json.loads(probe.stdout)
        if isinstance(ids,str):ids=[ids]
        assert len(ids)==1 and ids[0].upper().endswith('\\000BFDD8'),ids
        log=OUT/'flash-rx32-retry.txt'
        if log.exists():raise RuntimeError('Retry evidence already exists')
        result=subprocess.run([str(TOOLS/'tool-teensy/teensy_loader_cli.exe'),'--mcu=TEENSY41','-w','-v',
                               str(Path(manifest['artifacts']['hex']['path']).relative_to(ROOT))],
                              cwd=ROOT,capture_output=True,text=True,timeout=45)
        log.write_text(result.stdout+result.stderr,encoding='utf-8')
        (OUT/'flash-rx32-retry.json').write_text(json.dumps({'bootloader_ids':ids,'returncode':result.returncode,
            'hex_sha256':manifest['artifacts']['hex']['sha256']},indent=2),encoding='utf-8')
        print(result.stdout+result.stderr)
        if result.returncode:raise RuntimeError('Retry flash failed')
        return
    if manifest_path.exists():raise RuntimeError('Evidence already exists')
    nm=TOOLS/'toolchain-gccarmnoneeabi-teensy/bin/arm-none-eabi-nm.exe'
    symbols=subprocess.check_output([str(nm),'-S',str(BUILD/'firmware.elf')],text=True)
    rx=[int(line.split()[1],16) for line in symbols.splitlines() if 's_rxBufs' in line]
    rings=[int(line.split()[1],16) for line in symbols.splitlines() if 's_rxRing' in line]
    assert rx==[32*1536] and rings==[32*32],(rx,rings)
    assert 'ChannelEngineObjectFLED' in symbols and 'OctoWS2811' not in symbols
    sources=[ROOT/'src/main.cpp',ROOT/'platformio.ini',ROOT/'tools/patch_qnethernet.py']
    assert all(p.stat().st_mtime < (BUILD/'firmware.hex').stat().st_mtime for p in sources)
    manifest={'rx_buffer_bytes':rx[0],'rx_descriptor_bytes':rings[0],
              'sources':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},
              'artifacts':{},'unwired_only':True}
    for suffix in ('hex','elf'):
        target=ROOT/'artifacts'/('teensy41_unwired_bench_'+label+'.'+suffix)
        if target.exists():raise RuntimeError('Artifact already exists')
        shutil.copy2(BUILD/('firmware.'+suffix),target)
        manifest['artifacts'][suffix]={'path':str(target),'sha256':hashlib.sha256(target.read_bytes()).hexdigest()}
    ports=[p for p in list_ports.comports() if p.vid==0x16c0 and p.pid==0x0483]
    if len(ports)!=1 or ports[0].device!='COM4' or ports[0].serial_number!='7858800':
        raise RuntimeError('Expected single identified COM4 Teensy absent; no reboot/flash')
    # Only the serial-number-matched board is requested to enter HalfKay.
    try:
        with serial.Serial('COM4',134,timeout=.2,write_timeout=1):pass
    except serial.SerialException:
        pass  # USB re-enumeration may invalidate the handle immediately.
    command="@(Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -like 'USB\\VID_16C0&PID_0478\\*' } | Select-Object -ExpandProperty InstanceId) | ConvertTo-Json -Compress"
    deadline=time.monotonic()+25
    while time.monotonic()<deadline:
        response=subprocess.run(['powershell','-NoProfile','-Command',command],capture_output=True,text=True,timeout=12)
        ids=json.loads(response.stdout) if response.stdout.strip() else []
        if isinstance(ids,str):ids=[ids]
        if ids:
            if len(ids)!=1 or not ids[0].upper().endswith('\\000BFDD8'):
                raise RuntimeError('HalfKay serial does not match the identified test board')
            break
        time.sleep(.25)
    else:raise RuntimeError('Identified HalfKay did not enumerate')
    manifest['bootloader_ids']=ids
    result=subprocess.run([str(TOOLS/'tool-teensy/teensy_loader_cli.exe'),'--mcu=TEENSY41','-v',
                           manifest['artifacts']['hex']['path']],capture_output=True,text=True,timeout=45)
    (OUT/'flash-rx32.txt').write_text(result.stdout+result.stderr,encoding='utf-8')
    manifest['flash_returncode']=result.returncode
    manifest_path.write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print(json.dumps(manifest,indent=2))
    if result.returncode:raise RuntimeError('Flash failed; inspect preserved evidence')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description='Historical UNWIRED GPIO2-8 image only; never an Octo flasher')
    parser.add_argument('--retry',action='store_true')
    parser.add_argument('--confirm-unwired-pins-2-8', action='store_true',
                        help='Confirm the historical fixture has no LED wiring or Octo adapter connected')
    args=parser.parse_args()
    if not args.confirm_unwired_pins_2_8:
        parser.error('Refusing historical GPIO2-8 image: incompatible with the connected Octo adapter. '
                     'Build teensy41_octo_identify_rx32 instead; see docs/OCTO_ADAPTER_ABNAHME.md.')
    main(args.retry)
