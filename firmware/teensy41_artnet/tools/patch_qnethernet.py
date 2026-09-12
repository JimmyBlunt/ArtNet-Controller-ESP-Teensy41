"""Pinned QNEthernet RX ring for the 28-datagram, seven-port DMA bench.

Only the reviewed Teensy driver constant changes: 5 -> 32 descriptors.
The extra 27 receive buffers cost 41472 bytes in RAM2 plus 864 descriptor
bytes (and alignment) in the default RAM1 ring allocation.
The historical unwired and named Octo identification builds apply this patch;
other environments stay original. Each uses a separate project dependency copy.
"""
from hashlib import sha256
from pathlib import Path

ORIGINAL_SHA256='24087e449395631bc7a25ec8a71412d4ef63cafa33d41a6c08707ffd5f25baee'
OLD=b'static constexpr size_t kRxSize = 5;'
NEW=b'static constexpr size_t kRxSize = 32;  // Art-Net 28-packet burst during FastLED preparation'
RX32_ENVIRONMENTS = frozenset({'teensy41_unwired_bench', 'teensy41_octo_identify_rx32', 'teensy41_octo_web_rx32'})


def patch_bytes(source):
    original=source.replace(NEW,OLD,1) if NEW in source else source
    if sha256(original).hexdigest()!=ORIGINAL_SHA256 or original.count(OLD)!=1:
        raise RuntimeError('QNEthernet Teensy driver differs from reviewed 0.37.0 source')
    return original.replace(OLD,NEW,1)


def apply_build_patch(env):
    if env.subst('$PIOENV') not in RX32_ENVIRONMENTS:return
    source=(Path(env.subst('$PROJECT_LIBDEPS_DIR'))/env.subst('$PIOENV')/
            'QNEthernet/src/qnethernet/drivers/driver_teensy41.cpp')
    before=source.read_bytes();after=patch_bytes(before)
    if before!=after:source.write_bytes(after)
    print('QNEthernet audited Teensy RX descriptors: 32; SHA256:',sha256(after).hexdigest())


if 'Import' in globals():
    Import('env')
    apply_build_patch(env)
