"""Run after PlatformIO fetched the pinned release; does not modify vendor files."""
import importlib.util
import os
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('patch_fastled', ROOT/'tools/patch_fastled.py')
patch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(patch)


class VendorPatchTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(os.environ.get('PLATFORMIO_LIBDEPS_DIR', 'C:/codex-tools/teensy41-libs'))
        env = os.environ.get('FASTLED_TEST_ENV', 'teensy41_unwired_bench')
        path = root/env/'FastLED/src/platforms/arm/teensy/teensy4_common/clockless_objectfled.cpp.hpp'
        cls.original = path.read_bytes().replace(patch.INSERT, b'', 1)

    def test_exact_audited_source(self):
        after = patch.patch_bytes(self.original)
        self.assertEqual(after.count(patch.INSERT), 1)
        self.assertLess(after.index(patch.INSERT), after.index(patch.ANCHOR))

    def test_idempotent(self):
        after = patch.patch_bytes(self.original)
        self.assertEqual(patch.patch_bytes(after), after)

    def test_modified_original_rejected(self):
        with self.assertRaises(RuntimeError):
            patch.patch_bytes(self.original + b' ')

    def test_modified_patched_rejected(self):
        with self.assertRaises(RuntimeError):
            patch.patch_bytes(patch.patch_bytes(self.original) + b' ')

    def test_duplicate_insertion_rejected(self):
        after = patch.patch_bytes(self.original)
        with self.assertRaises(RuntimeError):
            patch.patch_bytes(after.replace(patch.INSERT, patch.INSERT * 2))


if __name__ == '__main__':
    unittest.main()
