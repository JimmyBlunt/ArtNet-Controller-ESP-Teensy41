import importlib.util
from pathlib import Path
import unittest
from unittest.mock import patch

spec=importlib.util.spec_from_file_location('qnet_patch',Path(__file__).parents[1]/'tools/patch_qnethernet.py')
mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)


class RingPatchTests(unittest.TestCase):
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
