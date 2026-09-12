"""Pinned FastLED 3.10.4 source guard and historical patch evidence.

Current Channel-API firmware needs no vendor patch. The build hook verifies the
original legacy source, restoring only our exact earlier experiment if present.
patch_bytes and the checked-in diff preserve the failed legacy experiment for
reproduction; they are no longer applied by the build hook.
"""
from hashlib import sha256
from pathlib import Path

ORIGINAL_SHA256 = '3ff259b7a15b3172fd16dc342e928e1ccfa71f313a54b6bb2b9fbf782a63f2e8'
ANCHOR = b'    // Configure timing at runtime\n'
INSERT = (b'    // Local fix: show() reads drawBuffer; FastLED already filled frameBufferLocal.\n'
          b'    objectfled->drawBuffer = objectfled->frameBufferLocal;\n\n')


def patch_bytes(source):
    if INSERT in source:
        original = source.replace(INSERT, b'', 1)
        if sha256(original).hexdigest() != ORIGINAL_SHA256:
            raise RuntimeError('Previously patched FastLED source has unexpected modifications')
        return source
    if sha256(source).hexdigest() != ORIGINAL_SHA256 or source.count(ANCHOR) != 1:
        raise RuntimeError('FastLED wrapper differs from audited 3.10.4 source; review required')
    return source.replace(ANCHOR, INSERT + ANCHOR, 1)


def apply_build_patch(env):
    if env.subst('$PIOENV') == 'teensy41_safe':
        return
    root = Path(env.subst('$PROJECT_LIBDEPS_DIR')) / env.subst('$PIOENV') / 'FastLED'
    source = root / 'src/platforms/arm/teensy/teensy4_common/clockless_objectfled.cpp.hpp'
    before = source.read_bytes()
    # Current firmware uses the official modern Channel engine. Restore only our
    # exact prior experiment; retain patch_bytes/tests as historical evidence.
    after = before.replace(INSERT, b'', 1) if INSERT in before else before
    if sha256(after).hexdigest() != ORIGINAL_SHA256:
        raise RuntimeError('FastLED legacy source differs from pinned original')
    if before != after:
        source.write_bytes(after)
    print('FastLED modern Channel engine; unmodified legacy source SHA256:', sha256(after).hexdigest())


if 'Import' in globals():  # PlatformIO/SCons entry point; importable by offline tests.
    Import('env')
    apply_build_patch(env)
