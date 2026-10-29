from paths import ROOT
import subprocess
import unittest

EXE = ROOT / 'out/translator-test'


class TranslatorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cmd = [
            'gcc', '-std=c11', '-Wall', '-Wextra', '-Werror',
            '-Isrc', str(ROOT / 'tests/test_translator.c'),
            str(ROOT / 'src/runtime_translator.c'),
            '-o', str(EXE)
        ]
        subprocess.run(cmd, check=True)

    def test_translator_suite(self):
        r = subprocess.run([str(EXE.resolve())], capture_output=True, text=True, timeout=10)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertIn('all translator tests passed', r.stdout)

    def test_android_cross_build(self):
        script = ROOT / 'build_android.sh'
        if script.exists():
            r = subprocess.run(['bash', str(script.resolve())], capture_output=True, text=True, timeout=60)
            self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
            out_lib = ROOT / 'out/android/arm64-v8a/libps4_translator.so'
            out_exe = ROOT / 'out/android/arm64-v8a/ps4-translator'
            self.assertTrue(out_lib.exists())
            self.assertTrue(out_exe.exists())
