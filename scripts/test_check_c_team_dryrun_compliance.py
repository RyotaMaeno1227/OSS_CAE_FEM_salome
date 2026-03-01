#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import textwrap
import unittest
import os
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "check_c_team_dryrun_compliance.sh"


class CheckCTeamDryrunComplianceTest(unittest.TestCase):
    def run_script(
        self,
        markdown: str,
        policy: str,
        extra_env: dict[str, str] | None = None,
    ) -> subprocess.CompletedProcess[str]:
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            fp.write(markdown)
            status_path = fp.name
        env = os.environ.copy()
        if extra_env:
            env.update(extra_env)
        return subprocess.run(
            ["bash", str(SCRIPT_PATH), status_path, policy],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
            env=env,
        )

    def test_pass_policy_accepts_latest_c_entry_anywhere(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-12
              - start_epoch=100
              - 実行コマンド: `make -C FEM4C`

            ## PMチーム
            - 実行タスク: C-18
              - start_epoch=200
              - scripts/c_stage_dryrun.sh --log /tmp/c18.log
              - dryrun_result=pass
              - dryrun_result=fail
            """
        )
        proc = self.run_script(markdown, "pass")
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("RESULT: PASS", proc.stdout)

    def test_pass_section_requires_c_section_source(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-12
              - start_epoch=100
              - 実行コマンド: `make -C FEM4C`

            ## PMチーム
            - 実行タスク: C-18
              - start_epoch=200
              - scripts/c_stage_dryrun.sh --log /tmp/c18.log
              - dryrun_result=pass
              - dryrun_result=fail
            """
        )
        proc = self.run_script(markdown, "pass_section")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("latest C entry is outside ## Cチーム section", proc.stdout)

    def test_invalid_policy_returns_code_2(self):
        markdown = "## Cチーム\n- 実行タスク: C-1\n  - dryrun_result=pass\n"
        proc = self.run_script(markdown, "invalid")
        self.assertEqual(proc.returncode, 2)
        self.assertIn("invalid policy", proc.stderr)

    def test_pass_section_freeze_rejects_coupled_path(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-19
              - start_epoch=200
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - dryrun_result=pass
              - 変更ファイル: `FEM4C/src/analysis/runner.c`
            """
        )
        proc = self.run_script(markdown, "pass_section_freeze")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("forbidden_paths_detected", proc.stdout)

    def test_pass_section_freeze_with_custom_file_env(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-19
              - start_epoch=200
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - dryrun_result=pass
              - 変更ファイル: `docs/team_status.md`
            """
        )
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            fp.write("docs/team_status.md\n")
            freeze_file = fp.name
        proc = self.run_script(
            markdown,
            "pass_section_freeze",
            extra_env={"COUPLED_FREEZE_FILE": freeze_file},
        )
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("forbidden_paths_detected", proc.stdout)

    def test_pass_section_freeze_timer_requires_end_epoch(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-19
              - start_epoch=200
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - dryrun_result=pass
            """
        )
        proc = self.run_script(markdown, "pass_section_freeze_timer")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("missing end_epoch", proc.stdout)

    def test_pass_section_freeze_timer_passes_with_completed_timer(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-19
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - dryrun_result=pass
            """
        )
        proc = self.run_script(markdown, "pass_section_freeze_timer")
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("RESULT: PASS", proc.stdout)

    def test_pass_section_freeze_timer_safe_prefers_latest_elapsed_min(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-57
              - start_epoch=200
              - タイマー出力（終了）:
                - end_epoch=1999
              - タイマーガード出力（途中確認）:
                - elapsed_min=10
              - タイマーガード出力（報告前）:
                - elapsed_min=30
              - タイマー出力（終了）:
                - end_epoch=2000
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
            """
        )
        proc = self.run_script(markdown, "pass_section_freeze_timer_safe")
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("elapsed_min: 30", proc.stdout)
        self.assertIn("end_epoch: 2000", proc.stdout)

    def test_pass_section_freeze_timer_safe_ignores_incomplete_latest_end_block(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-57
              - start_epoch=200
              - タイマー出力（終了）:
            ```text
            SESSION_TIMER_END
            end_epoch=2000
            elapsed_min=30
            SESSION_TIMER_END
            end_epoch=2001
            ```
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
            """
        )
        proc = self.run_script(markdown, "pass_section_freeze_timer_safe")
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("end_epoch: 2000", proc.stdout)
        self.assertIn("elapsed_min: 30", proc.stdout)

    def test_pass_section_freeze_timer_rejects_pending_placeholder(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-24
              - start_epoch=200
              - end_epoch=<pending>
              - elapsed_min=<pending>
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - dryrun_result=pass
            """
        )
        proc = self.run_script(markdown, "pass_section_freeze_timer")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("pending_placeholder_detected", proc.stdout)

    def test_pass_section_freeze_timer_rejects_token_missing_marker(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-24
              - start_epoch=200
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - dryrun_result=pass
              - ERROR: token file not found: /tmp/c_team_missing.token
            """
        )
        proc = self.run_script(markdown, "pass_section_freeze_timer")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("token_missing_marker_detected", proc.stdout)

    def test_pass_section_freeze_timer_safe_requires_safe_stage_command(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-20
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - dryrun_result=pass
            """
        )
        proc = self.run_script(markdown, "pass_section_freeze_timer_safe")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("missing safe_stage_command", proc.stdout)

    def test_pass_section_freeze_timer_safe_passes(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-20
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
            """
        )
        proc = self.run_script(markdown, "pass_section_freeze_timer_safe")
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)

    def test_pass_section_freeze_timer_safe_rejects_non_git_add_command(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-20
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=python scripts/run_team_audit.sh docs/team_status.md
              - dryrun_result=pass
            """
        )
        proc = self.run_script(markdown, "pass_section_freeze_timer_safe")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("safe_stage_command_not_git_add", proc.stdout)

    def test_pass_section_freeze_timer_safe_rejects_template_placeholder(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-26
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - 実行コマンド / pass-fail:
                - <記入>
            """
        )
        proc = self.run_script(markdown, "pass_section_freeze_timer_safe")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("template_placeholder_detected", proc.stdout)

    def test_pass_section_freeze_timer_safe_rejects_pass_fail_placeholder(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-27
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail:
                - <PASS|FAIL>
            """
        )
        proc = self.run_script(markdown, "pass_section_freeze_timer_safe")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("template_placeholder_detected", proc.stdout)


if __name__ == "__main__":
    unittest.main()
