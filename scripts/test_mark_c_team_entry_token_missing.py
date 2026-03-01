#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "mark_c_team_entry_token_missing.py"


def write_temp(text: str, suffix: str) -> str:
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=suffix, delete=False) as fp:
        fp.write(text)
        return fp.name


class MarkCTeamEntryTokenMissingTest(unittest.TestCase):
    def sample_team_status(self) -> str:
        return textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-24
              - タイマー出力（開始）:
            ```text
            SESSION_TIMER_START
            start_epoch=123
            ```
              - タイマー出力（終了）:
            ```text
            SESSION_TIMER_END
            session_token=/tmp/old.token
            end_utc=<pending>
            end_epoch=<pending>
            elapsed_min=<pending>
            ```
              - pass/fail:
                - PENDING

            ## PMチーム
            - 実行タスク: PM-3
            """
        )

    def run_script(self, status_path: str, start_epoch: int, in_place: bool) -> subprocess.CompletedProcess[str]:
        cmd = [
            "python",
            str(SCRIPT_PATH),
            "--team-status",
            status_path,
            "--target-start-epoch",
            str(start_epoch),
            "--token-path",
            "/tmp/missing.token",
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

    def test_marks_entry_as_token_missing(self) -> None:
        status_path = write_temp(self.sample_team_status(), ".md")
        proc = self.run_script(status_path, start_epoch=123, in_place=True)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        updated = Path(status_path).read_text(encoding="utf-8")
        self.assertIn("result=UPDATED", proc.stdout)
        self.assertIn("ERROR: token file not found: /tmp/missing.token", updated)
        self.assertNotIn("<pending>", updated)
        self.assertIn("FAIL（`token missing` により当該エントリは無効化）", updated)

    def test_fail_when_target_not_found(self) -> None:
        status_path = write_temp(self.sample_team_status(), ".md")
        proc = self.run_script(status_path, start_epoch=999, in_place=False)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("target start_epoch not found", proc.stderr + proc.stdout)


if __name__ == "__main__":
    unittest.main()
