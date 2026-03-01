#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "check_c_coupled_freeze_file.py"


class CheckCoupledFreezeFileTest(unittest.TestCase):
    def run_script(self, content: str) -> subprocess.CompletedProcess[str]:
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            fp.write(content)
            path = fp.name
        return subprocess.run(
            ["python", str(SCRIPT_PATH), path],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

    def test_pass_with_valid_patterns(self):
        proc = self.run_script(
            textwrap.dedent(
                """\
                # comment
                FEM4C/src/analysis/runner.c
                FEM4C/src/fem4c.c
                """
            )
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("verdict: PASS", proc.stdout)

    def test_fail_for_duplicate_pattern(self):
        proc = self.run_script(
            textwrap.dedent(
                """\
                FEM4C/src/analysis/runner.c
                FEM4C/src/analysis/runner.c
                """
            )
        )
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("duplicate_pattern", proc.stdout)

    def test_fail_for_invalid_prefix(self):
        proc = self.run_script("tmp/runner.c\n")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("invalid_prefix", proc.stdout)


if __name__ == "__main__":
    unittest.main()
