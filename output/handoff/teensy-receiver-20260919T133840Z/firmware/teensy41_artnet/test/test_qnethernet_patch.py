import importlib.util
from pathlib import Path
import unittest
from tempfile import TemporaryDirectory
from unittest.mock import patch

spec=importlib.util.spec_from_file_location('qnet_patch',Path(__file__).parents[1]/'tools/patch_qnethernet.py')
mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)


class RingPatchTests(unittest.TestCase):
    def test_build_hook_scopes_rx32_to_named_environments(self):
        source=b'prefix\n'+mod.OLD+b'\nsuffix'
        class Environment:
            def __init__(self, name, root): self.name, self.root=name,root
            def subst(self, key):
                return self.name if key=='$PIOENV' else self.root
        with TemporaryDirectory() as root, patch.object(mod,'ORIGINAL_SHA256',mod.sha256(source).hexdigest()):
            for name in ('teensy41_unwired_bench','teensy41_octo_identify_rx32','teensy41_octo_web_rx32','teensy41_safe','teensy41_hardware'):
                path=Path(root)/name/'QNEthernet/src/qnethernet/drivers/driver_teensy41.cpp'
                path.parent.mkdir(parents=True)
                path.write_bytes(source)
                mod.apply_build_patch(Environment(name,root))
                expected=source.replace(mod.OLD,mod.NEW) if name in mod.RX32_ENVIRONMENTS else source
                self.assertEqual(path.read_bytes(),expected)

    def test_only_rx_ring_changes_and_patch_is_idempotent(self):
        source=b'prefix\n'+mod.OLD+b'\nstatic constexpr size_t kTxSize = 5;\nsuffix'
        with patch.object(mod,'ORIGINAL_SHA256',mod.sha256(source).hexdigest()):
            result=mod.patch_bytes(source)
            self.assertEqual(result.replace(mod.NEW,mod.OLD),source)
            self.assertEqual(mod.patch_bytes(result),result)

    def test_unknown_or_modified_vendor_source_is_rejected(self):
        for source in (b'',mod.OLD,mod.NEW+b' changed'):
            with self.subTest(source=source),self.assertRaises(RuntimeError):mod.patch_bytes(source)


if __name__=='__main__':unittest.main()
