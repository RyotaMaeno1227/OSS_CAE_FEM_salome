#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "apply_c_stage_block_to_team_status.py"


def write_temp(text: str, suffix: str) -> str:
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=suffix, delete=False) as fp:
        fp.write(text)
        return fp.name


class ApplyCStageBlockToTeamStatusTest(unittest.TestCase):
    def sample_team_status(self) -> str:
        return textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-22
              - start_epoch=100
              - pass/fail:
                - PASS
            - 実行タスク: C-23
              - start_epoch=200
              - 追加実行コマンド / pass-fail:
                - `echo test` -> PASS
              - pass/fail:
                - PASS

            ## PMチーム
            - 実行タスク: PM-3
            """
        )

    def sample_block(self) -> str:
        return textwrap.dedent(
            """\
            - dry-run 生出力（strict-safe 記録）:
              - `dryrun_method=GIT_INDEX_FILE`
              - `dryrun_result=pass`
            """
        )

    def run_script(self, status_path: str, block_path: str, in_place: bool = False) -> subprocess.CompletedProcess[str]:
        return self.run_script_with_args(status_path, block_path, in_place=in_place)

    def run_script_with_args(
        self,
        status_path: str,
        block_path: str,
        in_place: bool = False,
        target_start_epoch: int | None = None,
    ) -> subprocess.CompletedProcess[str]:
        cmd = [
            "python",
            str(SCRIPT_PATH),
            "--team-status",
            status_path,
            "--block-file",
            block_path,
        ]
        if in_place:
            cmd.append("--in-place")
        if target_start_epoch is not None:
            cmd.extend(["--target-start-epoch", str(target_start_epoch)])
        return subprocess.run(
            cmd,
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

    def test_inserts_block_into_latest_entry(self):
        status_path = write_temp(self.sample_team_status(), ".md")
        block_path = write_temp(self.sample_block(), ".md")
        proc = self.run_script(status_path, block_path, in_place=True)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        updated = Path(status_path).read_text(encoding="utf-8")
        self.assertIn("APPLY_C_STAGE_BLOCK_TO_TEAM_STATUS", proc.stdout)
        self.assertEqual(updated.count("dry-run 生出力（strict-safe 記録）"), 1)
        self.assertIn("`dryrun_result=pass`", updated)

    def test_replaces_existing_block(self):
        status_text = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-23
              - start_epoch=200
              - dry-run 生出力（strict-safe 記録）:
                - `dryrun_result=fail`
              - pass/fail:
                - PASS
            """
        )
        status_path = write_temp(status_text, ".md")
        block_path = write_temp(self.sample_block(), ".md")
        proc = self.run_script(status_path, block_path, in_place=True)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        updated = Path(status_path).read_text(encoding="utf-8")
        self.assertIn("`dryrun_result=pass`", updated)
        self.assertNotIn("`dryrun_result=fail`", updated)

    def test_fail_when_c_section_missing(self):
        status_path = write_temp("## PMチーム\n- 実行タスク: PM-3\n", ".md")
        block_path = write_temp(self.sample_block(), ".md")
        proc = self.run_script(status_path, block_path, in_place=False)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("section not found", proc.stderr + proc.stdout)

    def test_apply_to_specific_start_epoch(self):
        status_path = write_temp(self.sample_team_status(), ".md")
        block_path = write_temp(self.sample_block(), ".md")
        proc = self.run_script_with_args(
            status_path,
            block_path,
            in_place=True,
            target_start_epoch=100,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        updated = Path(status_path).read_text(encoding="utf-8")
        self.assertIn("entry_start_epoch=100", proc.stdout)
        self.assertEqual(updated.count("dry-run 生出力（strict-safe 記録）"), 1)

    def test_fail_when_target_start_epoch_missing(self):
        status_path = write_temp(self.sample_team_status(), ".md")
        block_path = write_temp(self.sample_block(), ".md")
        proc = self.run_script_with_args(
            status_path,
            block_path,
            in_place=True,
            target_start_epoch=999,
        )
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("target start_epoch not found", proc.stderr + proc.stdout)


if __name__ == "__main__":
    unittest.main()
