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
