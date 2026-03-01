#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "append_c_team_entry.py"


def write_temp(text: str, suffix: str) -> str:
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=suffix, delete=False) as fp:
        fp.write(text)
        return fp.name


class AppendCTeamEntryTest(unittest.TestCase):
    def sample_status(self) -> str:
        return textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-24
              - pass/fail:
                - PASS

            ## PMチーム
            - 実行タスク: PM-3
            """
        )

    def sample_entry(self) -> str:
        return textwrap.dedent(
            """\
            - 実行タスク: C-25
              - pass/fail:
                - PASS
            """
        )

    def run_script(self, status_path: str, entry_path: str, in_place: bool = False) -> subprocess.CompletedProcess[str]:
        cmd = [
            "python",
            str(SCRIPT_PATH),
            "--team-status",
            status_path,
            "--entry-file",
            entry_path,
        ]
        if in_place:
            cmd.append("--in-place")
        return subprocess.run(
            cmd,
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

    def test_append_into_c_section(self) -> None:
        status_path = write_temp(self.sample_status(), ".md")
        entry_path = write_temp(self.sample_entry(), ".md")
        proc = self.run_script(status_path, entry_path, in_place=True)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        updated = Path(status_path).read_text(encoding="utf-8")
        self.assertIn("result=UPDATED", proc.stdout)
        self.assertIn("- 実行タスク: C-25", updated)
        self.assertLess(updated.find("- 実行タスク: C-25"), updated.find("## PMチーム"))

    def test_fail_when_c_section_missing(self) -> None:
        status_path = write_temp("## PMチーム\n- 実行タスク: PM-3\n", ".md")
        entry_path = write_temp(self.sample_entry(), ".md")
        proc = self.run_script(status_path, entry_path, in_place=False)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("section not found", proc.stderr + proc.stdout)

    def test_fail_when_entry_invalid(self) -> None:
        status_path = write_temp(self.sample_status(), ".md")
        entry_path = write_temp("invalid\n", ".md")
        proc = self.run_script(status_path, entry_path, in_place=False)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("entry must start", proc.stderr + proc.stdout)


if __name__ == "__main__":
    unittest.main()
