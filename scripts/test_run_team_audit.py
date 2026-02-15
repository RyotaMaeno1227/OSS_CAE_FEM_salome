#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import textwrap
import unittest
import os
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "run_team_audit.sh"


def write_status(markdown: str) -> str:
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
        fp.write(markdown)
        return fp.name


class RunTeamAuditTest(unittest.TestCase):
    def test_invalid_impl_changes_env_fails_fast(self):
        status_path = write_status("## Aチーム\n")
        env = os.environ.copy()
        env["TEAM_AUDIT_REQUIRE_IMPL_CHANGES"] = "bad"
        proc = subprocess.run(
            ["bash", str(SCRIPT_PATH), status_path, "30", "pass"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
            env=env,
        )
        self.assertEqual(proc.returncode, 2)
        self.assertIn("invalid TEAM_AUDIT_REQUIRE_IMPL_CHANGES", proc.stderr)

    def test_invalid_policy_fails_fast(self):
        status_path = write_status("## Aチーム\n")
        proc = subprocess.run(
            ["bash", str(SCRIPT_PATH), status_path, "30", "bad"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 2)
        self.assertIn("invalid C dry-run policy", proc.stderr)

    def test_pass_policy_with_valid_sample_status(self):
        markdown = textwrap.dedent(
            """\
            ## Aチーム
            - 実行タスク: A-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=100
              - elapsed_min=40
              - 変更ファイル: `FEM4C/src/a.c`
              - 実行コマンド: `make -C FEM4C`
              - pass/fail: PASS

            ## Bチーム
            - 実行タスク: B-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=101
              - elapsed_min=40
              - 変更ファイル: `FEM4C/src/b.c`
              - 実行コマンド: `make -C FEM4C`
              - pass/fail: PASS

            ## Cチーム
            - 実行タスク: C-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=102
              - elapsed_min=40
              - 変更ファイル: `FEM4C/src/c.c`
              - 実行コマンド: `scripts/c_stage_dryrun.sh`
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        status_path = write_status(markdown)
        proc = subprocess.run(
            ["bash", str(SCRIPT_PATH), status_path, "30", "pass"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("C_STAGING_AUDIT_JSON=", proc.stdout)

    def test_require_impl_changes_mode_fails_for_docs_only_changes(self):
        markdown = textwrap.dedent(
            """\
            ## Aチーム
            - 実行タスク: A-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=100
              - elapsed_min=40
              - 変更ファイル: `FEM4C/src/a.c`
              - 実行コマンド: `make -C FEM4C`
              - pass/fail: PASS

            ## Bチーム
            - 実行タスク: B-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=101
              - elapsed_min=40
              - 変更ファイル: `FEM4C/src/b.c`
              - 実行コマンド: `make -C FEM4C`
              - pass/fail: PASS

            ## Cチーム
            - 実行タスク: C-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=102
              - elapsed_min=40
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド: `scripts/c_stage_dryrun.sh`
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        status_path = write_status(markdown)
        env = os.environ.copy()
        env["TEAM_AUDIT_REQUIRE_IMPL_CHANGES"] = "1"
        proc = subprocess.run(
            ["bash", str(SCRIPT_PATH), status_path, "30", "pass"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
            env=env,
        )
        self.assertEqual(proc.returncode, 1)
        self.assertIn("AUDIT_JSON=", proc.stdout)

    def test_both_section_policy_passes_with_c_section_entry(self):
        markdown = textwrap.dedent(
            """\
            ## Aチーム
            - 実行タスク: A-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=100
              - elapsed_min=40
              - 変更ファイル: `FEM4C/src/a.c`
              - 実行コマンド: `make -C FEM4C`
              - pass/fail: PASS

            ## Bチーム
            - 実行タスク: B-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=101
              - elapsed_min=40
              - 変更ファイル: `FEM4C/src/b.c`
              - 実行コマンド: `make -C FEM4C`
              - pass/fail: PASS

            ## Cチーム
            - 実行タスク: C-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=102
              - elapsed_min=40
              - 変更ファイル: `FEM4C/src/c.c`
              - 実行コマンド: `scripts/c_stage_dryrun.sh`
              - dryrun_result=pass
              - dryrun_result=fail
              - pass/fail: PASS
            """
        )
        status_path = write_status(markdown)
        proc = subprocess.run(
            ["bash", str(SCRIPT_PATH), status_path, "30", "both_section"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("C_STAGING_AUDIT_JSON=", proc.stdout)

    def test_both_section_freeze_policy_passes_with_safe_paths(self):
        markdown = textwrap.dedent(
            """\
            ## Aチーム
            - 実行タスク: A-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=100
              - elapsed_min=40
              - 変更ファイル: `FEM4C/src/a.c`
              - 実行コマンド: `make -C FEM4C`
              - pass/fail: PASS

            ## Bチーム
            - 実行タスク: B-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=101
              - elapsed_min=40
              - 変更ファイル: `FEM4C/src/b.c`
              - 実行コマンド: `make -C FEM4C`
              - pass/fail: PASS

            ## Cチーム
            - 実行タスク: C-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=102
              - elapsed_min=40
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド: `scripts/c_stage_dryrun.sh`
              - dryrun_result=pass
              - dryrun_result=fail
              - pass/fail: PASS
            """
        )
        status_path = write_status(markdown)
        proc = subprocess.run(
            ["bash", str(SCRIPT_PATH), status_path, "30", "both_section_freeze"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("C_STAGING_AUDIT_JSON=", proc.stdout)

    def test_parallel_invocation_avoids_tmp_json_collision(self):
        markdown = textwrap.dedent(
            """\
            ## Aチーム
            - 実行タスク: A-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=100
              - elapsed_min=40
              - 変更ファイル: `FEM4C/src/a.c`
              - 実行コマンド: `make -C FEM4C`
              - pass/fail: PASS

            ## Bチーム
            - 実行タスク: B-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=101
              - elapsed_min=40
              - 変更ファイル: `FEM4C/src/b.c`
              - 実行コマンド: `make -C FEM4C`
              - pass/fail: PASS

            ## Cチーム
            - 実行タスク: C-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=102
              - elapsed_min=40
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド: `scripts/c_stage_dryrun.sh`
              - dryrun_result=pass
              - dryrun_result=fail
              - pass/fail: PASS
            """
        )
        status_path = write_status(markdown)
        procs = [
            subprocess.Popen(
                ["bash", str(SCRIPT_PATH), status_path, "30", "both_section_freeze"],
                cwd=REPO_ROOT,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            for _ in range(2)
        ]
        outputs = [proc.communicate() for proc in procs]
        for proc, (stdout, stderr) in zip(procs, outputs):
            self.assertEqual(proc.returncode, 0, msg=(stdout or "") + (stderr or ""))
            self.assertIn("C_STAGING_AUDIT_JSON=", stdout or "")

    def test_pass_section_freeze_timer_policy_passes_with_completed_timer(self):
        markdown = textwrap.dedent(
            """\
            ## Aチーム
            - 実行タスク: A-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=100
              - elapsed_min=40
              - 変更ファイル: `FEM4C/src/a.c`
              - 実行コマンド: `make -C FEM4C`
              - pass/fail: PASS

            ## Bチーム
            - 実行タスク: B-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=101
              - elapsed_min=40
              - 変更ファイル: `FEM4C/src/b.c`
              - 実行コマンド: `make -C FEM4C`
              - pass/fail: PASS

            ## Cチーム
            - 実行タスク: C-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=102
              - end_epoch=4000
              - elapsed_min=40
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド: `scripts/c_stage_dryrun.sh`
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        status_path = write_status(markdown)
        proc = subprocess.run(
            ["bash", str(SCRIPT_PATH), status_path, "30", "pass_section_freeze_timer"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)

    def test_pass_section_freeze_timer_safe_policy_passes(self):
        markdown = textwrap.dedent(
            """\
            ## Aチーム
            - 実行タスク: A-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=100
              - elapsed_min=40
              - 変更ファイル: `FEM4C/src/a.c`
              - 実行コマンド: `make -C FEM4C`
              - pass/fail: PASS

            ## Bチーム
            - 実行タスク: B-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=101
              - elapsed_min=40
              - 変更ファイル: `FEM4C/src/b.c`
              - 実行コマンド: `make -C FEM4C`
              - pass/fail: PASS

            ## Cチーム
            - 実行タスク: C-xx
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=102
              - end_epoch=4000
              - elapsed_min=40
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド: `scripts/c_stage_dryrun.sh`
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        status_path = write_status(markdown)
        proc = subprocess.run(
            ["bash", str(SCRIPT_PATH), status_path, "30", "pass_section_freeze_timer_safe"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)


if __name__ == "__main__":
    unittest.main()
