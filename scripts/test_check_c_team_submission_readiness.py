#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import textwrap
import unittest
import os
from pathlib import Path
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "check_c_team_submission_readiness.sh"


def write_status(markdown: str) -> str:
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
        fp.write(markdown)
        return fp.name


class CheckCTeamSubmissionReadinessTest(unittest.TestCase):
    def run_script(
        self,
        markdown: str,
        extra_env: dict[str, str] | None = None,
        status_path: str | None = None,
    ) -> subprocess.CompletedProcess[str]:
        if status_path is None:
            status_path = write_status(markdown)
        env = os.environ.copy()
        env.pop("C_COLLECT_PREFLIGHT_LOG", None)
        env.pop("C_COLLECT_EXPECT_TEAM_STATUS", None)
        env["C_REQUIRE_REVIEW_COMMANDS"] = "0"
        env["C_REQUIRE_COLLECT_PREFLIGHT_ENABLED"] = "1"
        env["C_COLLECT_LATEST_REQUIRE_FOUND"] = "0"
        env.setdefault("C_TEAM_SKIP_STAGING_BUNDLE", "1")
        if extra_env:
            env.update(extra_env)
        return subprocess.run(
            ["bash", str(SCRIPT_PATH), status_path, "30"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
            env=env,
        )

    def test_pass_with_strict_safe_entry(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-21
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド: `scripts/c_stage_dryrun.sh --log /tmp/c.log`
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        proc = self.run_script(markdown)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("PASS: C-team submission readiness", proc.stdout)
        self.assertIn("collect_preflight_check_reason=latest_not_found_default_skip", proc.stdout)
        self.assertIn("submission_readiness_collect_preflight_log_source=default_latest", proc.stdout)
        self.assertIn("submission_readiness_collect_preflight_log_effective=latest", proc.stdout)
        self.assertIn("submission_readiness_collect_preflight_check=skipped", proc.stdout)
        self.assertIn(
            "submission_readiness_collect_preflight_reason=latest_not_found_default_skip",
            proc.stdout,
        )
        self.assertIn("submission_readiness_require_review_commands=0", proc.stdout)

    def test_fail_trace_env_alias_is_honored_when_require_prefix_is_empty(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-58
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド: `scripts/c_stage_dryrun.sh --log /tmp/c58.log`
              - scripts/c_stage_dryrun.sh --log /tmp/c58.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY": "",
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY": "",
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV": "",
                "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY": "0",
                "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY": "1",
                "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV": "1",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        combined = proc.stdout + proc.stderr
        self.assertIn("submission_readiness_require_fail_trace_retry_consistency=0", combined)
        self.assertIn("submission_readiness_require_fail_trace_retry_consistency_key=1", combined)
        self.assertIn("submission_readiness_require_fail_trace_retry_consistency_strict_env=1", combined)
        self.assertIn("fail_trace_retry_consistency_check=skipped", combined)

    def test_pass_with_multiple_elapsed_values_prefers_latest(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-57
              - SESSION_TIMER_START
              - タイマーガード出力（途中確認）
              - elapsed_min=10
              - タイマーガード出力（報告前）
              - elapsed_min=30
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド: `scripts/c_stage_dryrun.sh --log /tmp/c57.log`
              - scripts/c_stage_dryrun.sh --log /tmp/c57.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        proc = self.run_script(markdown, extra_env={"C_TEAM_SKIP_STAGING_BUNDLE": "1"})
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("elapsed_min: 30", proc.stdout)
        self.assertIn("PASS: C-team submission readiness", proc.stdout)

    def test_reports_explicit_preflight_log_source(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-44
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド:
                - `scripts/c_stage_dryrun.sh --log /tmp/c44.log`
                - missing_log_review_command=rg -n 'collect_preflight' /tmp/c44_entry.md
              - scripts/c_stage_dryrun.sh --log /tmp/c44.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "",
                "C_REQUIRE_REVIEW_COMMANDS": "1",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        combined = proc.stdout + proc.stderr
        self.assertIn("submission_readiness_collect_preflight_log_source=explicit_env", combined)
        self.assertIn("submission_readiness_collect_preflight_log_effective=<empty>", combined)
        self.assertIn("submission_readiness_require_review_commands=1", combined)
        self.assertIn("collect_preflight_check=skipped", combined)

    def test_pass_with_parent_env_contamination_after_sanitization(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-44
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド: `scripts/c_stage_dryrun.sh --log /tmp/c44.log`
              - scripts/c_stage_dryrun.sh --log /tmp/c44.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        with patch.dict(
            os.environ,
            {
                "C_REQUIRE_REVIEW_COMMANDS": "1",
                "C_COLLECT_PREFLIGHT_LOG": "/tmp/c44_parent_env_missing.log",
                "C_COLLECT_EXPECT_TEAM_STATUS": "/tmp/c44_parent_env_wrong.md",
                "C_COLLECT_LATEST_REQUIRE_FOUND": "1",
                "C_REQUIRE_COLLECT_PREFLIGHT_ENABLED": "0",
            },
            clear=False,
        ):
            proc = self.run_script(markdown, extra_env={"C_TEAM_SKIP_STAGING_BUNDLE": "1"})
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        combined = proc.stdout + proc.stderr
        self.assertIn("collect_preflight_check_reason=latest_not_found_default_skip", combined)
        self.assertIn("review_command_check=skipped", combined)
        self.assertIn("PASS: C-team submission readiness", combined)

    def test_fail_without_safe_stage_command(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-21
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド: `scripts/c_stage_dryrun.sh --log /tmp/c.log`
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        proc = self.run_script(markdown)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("missing safe_stage_command", proc.stdout)

    def test_fail_with_pending_timer_placeholder(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-24
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=<pending>
              - elapsed_min=<pending>
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: FAIL
            """
        )
        proc = self.run_script(markdown)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("pending_placeholder_detected", proc.stdout)

    def test_review_command_fail_reason_codes_include_multiple_missing_reasons(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-58
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c58.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - python scripts/check_c_team_collect_preflight_report.py /tmp/c58.log --require-enabled -> PASS
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_REQUIRE_REVIEW_COMMANDS": "1",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn(
            "review_command_fail_reason=missing missing_log_review_command; missing collect_report_review_command",
            combined,
        )
        self.assertIn(
            "review_command_fail_reason_codes=review_command_missing_missing_log_review_command,review_command_missing_collect_report_review_command",
            combined,
        )
        self.assertIn("review_command_fail_reason_codes_source=checker", combined)
        self.assertIn("submission_readiness_fail_step=review_command", combined)

    def test_fail_with_token_missing_marker(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-24
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - ERROR: token file not found: /tmp/c_team_missing.token
              - pass/fail: FAIL
            """
        )
        proc = self.run_script(markdown)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("token_missing_marker_detected", proc.stdout)

    def test_fail_with_template_placeholder(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-27
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド / pass-fail:
                - <記入>
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        proc = self.run_script(markdown, extra_env={"C_TEAM_SKIP_STAGING_BUNDLE": "1"})
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("template_placeholder_detected", proc.stdout)

    def test_fail_with_pass_fail_placeholder(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-27
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド / pass-fail:
                - scripts/c_stage_dryrun.sh --log /tmp/c.log -> PASS
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail:
                - <PASS|FAIL>
            """
        )
        proc = self.run_script(markdown, extra_env={"C_TEAM_SKIP_STAGING_BUNDLE": "1"})
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("template_placeholder_detected", proc.stdout)

    def test_pass_when_staging_bundle_is_skipped_via_env(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-26
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド: `scripts/c_stage_dryrun.sh --log /tmp/c.log`
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        proc = self.run_script(markdown, extra_env={"C_TEAM_SKIP_STAGING_BUNDLE": "1"})
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("skip_reason=C_TEAM_SKIP_STAGING_BUNDLE=1", proc.stdout)
        self.assertIn("review_command_check=skipped", proc.stdout)
        self.assertIn("PASS: C-team submission readiness", proc.stdout)

    def test_fail_when_review_command_required_but_missing(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-41
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド: `scripts/c_stage_dryrun.sh --log /tmp/c.log`
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_REQUIRE_REVIEW_COMMANDS": "1",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn("missing missing_log_review_command", combined)
        self.assertIn("review_command_check=fail", proc.stderr)
        self.assertIn(
            "review_command_fail_reason=missing missing_log_review_command",
            proc.stderr,
        )
        self.assertIn(
            "review_command_fail_reason_codes=review_command_missing_missing_log_review_command",
            proc.stderr,
        )
        self.assertIn("review_command_fail_reason_codes_source=checker", proc.stderr)
        self.assertIn(
            "review_command_retry_command=python scripts/check_c_team_review_commands.py --team-status ",
            proc.stderr,
        )
        self.assertIn("submission_readiness_retry_command=", proc.stderr)
        self.assertIn("submission_readiness_fail_step=review_command", proc.stderr)

    def test_pass_when_review_command_required_and_present(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-41
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド: `scripts/c_stage_dryrun.sh --log /tmp/c.log`
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - missing_log_review_command=rg -n 'collect_preflight' /tmp/entry.md
              - pass/fail: PASS
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_REQUIRE_REVIEW_COMMANDS": "1",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("verdict=PASS", proc.stdout)
        self.assertIn("review_command_check=pass", proc.stdout)
        self.assertIn("review_command_fail_reason=-", proc.stdout)
        self.assertIn("review_command_fail_reason_codes=-", proc.stdout)
        self.assertIn("review_command_fail_reason_codes_source=-", proc.stdout)
        self.assertIn(
            "review_command_retry_command=python scripts/check_c_team_review_commands.py --team-status ",
            proc.stdout,
        )

    def test_fail_when_fail_trace_retry_consistency_required_and_finalize_retry_missing(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-50
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - missing_log_review_command=rg -n 'collect_preflight' /tmp/entry.md
              - fail_trace_audit_result=FAIL
              - fail_trace_audit_retry_reason=audit_result_FAIL
              - fail_trace_audit_retry_command=scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 30 | tee /tmp/c50_retry.log
              - pass/fail: PASS
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_REQUIRE_REVIEW_COMMANDS": "1",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn("missing fail_trace_finalize_retry_command", combined)
        self.assertIn("fail_trace_retry_consistency_check=fail", proc.stderr)

    def test_fail_when_fail_trace_retry_consistency_key_required_and_missing(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-51
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド / pass-fail:
                - scripts/c_stage_dryrun.sh --log /tmp/c.log -> PASS
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - fail_trace_audit_result=PASS
              - fail_trace_audit_retry_command=scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 45 | tee /tmp/c51_retry.log
              - pass/fail: PASS
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY": "1",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("missing fail_trace_retry_consistency_check", proc.stderr)

    def test_pass_when_fail_trace_retry_consistency_key_required_and_present(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-51
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド / pass-fail:
                - scripts/c_stage_dryrun.sh --log /tmp/c.log -> PASS
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - fail_trace_audit_result=PASS
              - fail_trace_retry_consistency_check=pass
              - fail_trace_audit_retry_command=scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 45 | tee /tmp/c51_retry.log
              - pass/fail: PASS
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY": "1",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("fail_trace_retry_consistency_check=pass", proc.stdout)

    def test_pass_when_strict_env_match_is_required_and_present(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-53
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `scripts/check_c_team_fail_trace_retry_consistency.py`
              - 実行コマンド: `python scripts/test_check_c_team_fail_trace_retry_consistency.py`
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - fail_trace_audit_result=FAIL
              - fail_trace_audit_retry_reason=audit_result_FAIL
              - fail_trace_retry_consistency_required=1
              - fail_trace_retry_consistency_require_key=1
              - fail_trace_audit_retry_command=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 45 | tee /tmp/c53_retry.log
              - fail_trace_finalize_retry_command=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 bash scripts/recover_c_team_token_missing_session.sh --team-status docs/team_status.md --finalize-session-token /tmp/c_team_session.token --task-title "C-53" --guard-minutes 30 --check-submission-readiness-minutes 45 --fail-trace-audit-log /tmp/c53_retry.log
              - fail_trace_retry_consistency_check=pass
              - pass/fail: PASS
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY": "1",
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV": "1",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("submission_readiness_require_fail_trace_retry_consistency_strict_env=1", proc.stdout)
        self.assertIn("fail_trace_retry_consistency_reasons=-", proc.stdout)
        self.assertIn("fail_trace_retry_consistency_reason_codes=-", proc.stdout)
        self.assertIn("fail_trace_retry_consistency_check=pass", proc.stdout)

    def test_fail_when_strict_env_match_is_required_but_missing(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-53
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `scripts/check_c_team_fail_trace_retry_consistency.py`
              - 実行コマンド: `python scripts/test_check_c_team_fail_trace_retry_consistency.py`
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - fail_trace_audit_result=FAIL
              - fail_trace_audit_retry_reason=audit_result_FAIL
              - fail_trace_retry_consistency_required=1
              - fail_trace_retry_consistency_require_key=1
              - fail_trace_audit_retry_command=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 45 | tee /tmp/c53_retry.log
              - fail_trace_finalize_retry_command=bash scripts/recover_c_team_token_missing_session.sh --team-status docs/team_status.md --finalize-session-token /tmp/c_team_session.token --task-title "C-53" --guard-minutes 30 --check-submission-readiness-minutes 45 --fail-trace-audit-log /tmp/c53_retry.log
              - fail_trace_retry_consistency_check=pass
              - pass/fail: PASS
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY": "1",
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV": "1",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn(
            "fail_trace_retry_consistency_reason_codes=missing_c_fail_trace_require_retry_consistency_in_fail_trace_finalize_retry_command",
            proc.stderr,
        )
        self.assertIn(
            "fail_trace_retry_consistency_retry_command=python scripts/check_c_team_fail_trace_retry_consistency.py --team-status ",
            proc.stderr,
        )
        self.assertIn(
            "missing C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY in fail_trace_finalize_retry_command",
            proc.stderr,
        )

    def test_pass_when_fail_trace_retry_consistency_check_is_disabled(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-50
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド / pass-fail:
                - scripts/c_stage_dryrun.sh --log /tmp/c.log -> PASS
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - fail_trace_audit_result=FAIL
              - fail_trace_audit_retry_reason=audit_result_FAIL
              - fail_trace_audit_retry_command=scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 30 | tee /tmp/c50_retry.log
              - pass/fail: PASS
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY": "0",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("fail_trace_retry_consistency_check=skipped", proc.stdout)

    def test_pass_with_collect_preflight_log(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-29
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド: `scripts/c_stage_dryrun.sh --log /tmp/c.log`
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        status_path = write_status(markdown)
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            fp.write(
                "collect_result=PASS\n"
                "preflight_mode=enabled\n"
                f"preflight_team_status={status_path}\n"
                "preflight_result=pass\n"
            )
            collect_log = fp.name
        proc = self.run_script(
            markdown,
            status_path=status_path,
            extra_env={"C_COLLECT_PREFLIGHT_LOG": collect_log},
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("C_TEAM_COLLECT_PREFLIGHT_REPORT", proc.stdout)
        self.assertIn("PASS: C-team submission readiness", proc.stdout)

    def test_fail_with_collect_preflight_log_not_enabled(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-29
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド: `scripts/c_stage_dryrun.sh --log /tmp/c.log`
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            fp.write(
                "collect_result=PASS\n"
                "preflight_mode=disabled\n"
                "preflight_result=skipped\n"
            )
            collect_log = fp.name
        proc = self.run_script(markdown, extra_env={"C_COLLECT_PREFLIGHT_LOG": collect_log})
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn("preflight_mode=disabled", combined)

    def test_fail_with_collect_preflight_log_missing_file(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-37
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド: `scripts/c_stage_dryrun.sh --log /tmp/c.log`
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        missing_log = "/tmp/c37_explicit_missing_readiness.log"
        Path(missing_log).unlink(missing_ok=True)
        proc = self.run_script(
            markdown,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": missing_log,
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn(f"collect_preflight_log_missing={missing_log}", combined)
        self.assertIn("collect_preflight_check_reason=explicit_log_missing", combined)
        self.assertIn("collect_preflight_check=fail", combined)

    def test_fail_with_collect_preflight_log_team_status_mismatch(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-29
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド: `scripts/c_stage_dryrun.sh --log /tmp/c.log`
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        status_path = write_status(markdown)
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            mismatched_path = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            fp.write(
                "collect_result=PASS\n"
                "preflight_mode=enabled\n"
                f"preflight_team_status={mismatched_path}\n"
                "preflight_result=pass\n"
            )
            collect_log = fp.name
        proc = self.run_script(
            markdown,
            status_path=status_path,
            extra_env={"C_COLLECT_PREFLIGHT_LOG": collect_log},
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn("expected_preflight_team_status=", combined)

    def test_pass_with_collect_preflight_log_and_explicit_expected_override(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-29
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド: `scripts/c_stage_dryrun.sh --log /tmp/c.log`
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        status_path = write_status(markdown)
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            explicit_expected = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            fp.write(
                "collect_result=PASS\n"
                "preflight_mode=enabled\n"
                f"preflight_team_status={explicit_expected}\n"
                "preflight_result=pass\n"
            )
            collect_log = fp.name
        proc = self.run_script(
            markdown,
            status_path=status_path,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": collect_log,
                "C_COLLECT_EXPECT_TEAM_STATUS": explicit_expected,
            },
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("PASS: C-team submission readiness", proc.stdout)

    def test_pass_with_collect_preflight_log_latest_resolution(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-30
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c30_latest_readiness.log --require-enabled
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        status_path = write_status(markdown)
        Path("/tmp/c30_latest_readiness.log").write_text(
            "collect_result=PASS\n"
            "preflight_mode=enabled\n"
            f"preflight_team_status={status_path}\n"
            "preflight_result=pass\n",
            encoding="utf-8",
        )
        proc = self.run_script(
            markdown,
            status_path=status_path,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("collect_preflight_log_resolved=/tmp/c30_latest_readiness.log", proc.stdout)
        self.assertIn("PASS: C-team submission readiness", proc.stdout)

    def test_pass_with_collect_preflight_log_latest_auto_resolution_when_env_unset(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-30
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c30_latest_readiness_auto.log --require-enabled
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        status_path = write_status(markdown)
        Path("/tmp/c30_latest_readiness_auto.log").write_text(
            "collect_result=PASS\n"
            "preflight_mode=enabled\n"
            f"preflight_team_status={status_path}\n"
            "preflight_result=pass\n",
            encoding="utf-8",
        )
        proc = self.run_script(
            markdown,
            status_path=status_path,
            extra_env={"C_TEAM_SKIP_STAGING_BUNDLE": "1"},
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("collect_preflight_log_resolved=/tmp/c30_latest_readiness_auto.log", proc.stdout)
        self.assertIn("PASS: C-team submission readiness", proc.stdout)

    def test_collect_preflight_can_be_disabled_with_explicit_empty(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-30
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c30_latest_readiness_disable.log --require-enabled
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        status_path = write_status(markdown)
        Path("/tmp/c30_latest_readiness_disable.log").write_text(
            "collect_result=PASS\n"
            "preflight_mode=enabled\n"
            f"preflight_team_status={status_path}\n"
            "preflight_result=pass\n",
            encoding="utf-8",
        )
        proc = self.run_script(
            markdown,
            status_path=status_path,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("collect_preflight_check=skipped", proc.stdout)
        self.assertNotIn("collect_preflight_log_resolved=", proc.stdout)

    def test_fail_when_collect_preflight_log_latest_is_required_and_missing(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-30
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド: `scripts/c_stage_dryrun.sh --log /tmp/c.log`
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_COLLECT_LATEST_REQUIRE_FOUND": "1",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn("collect_log_not_found", combined)
        self.assertIn("collect_preflight_check_reason=latest_not_found_strict", combined)
        self.assertIn("collect_preflight_check=fail", combined)

    def test_fail_when_collect_preflight_latest_missing_with_review_required(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-43
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド: `scripts/c_stage_dryrun.sh --log /tmp/c.log`
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_COLLECT_LATEST_REQUIRE_FOUND": "1",
                "C_REQUIRE_REVIEW_COMMANDS": "1",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn("collect_log_not_found", combined)
        self.assertIn("collect_preflight_check_reason=latest_not_found_strict", combined)
        self.assertIn("collect_preflight_check=fail", combined)
        self.assertNotIn("review_command_check=fail", combined)

    def test_pass_when_collect_preflight_latest_resolved_log_file_is_missing_by_default(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-37
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c37_readiness_missing_default.log --require-enabled
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        combined = proc.stdout + proc.stderr
        self.assertIn("collect_preflight_log_resolved=/tmp/c37_readiness_missing_default.log", combined)
        self.assertIn("collect_preflight_log_missing=/tmp/c37_readiness_missing_default.log", combined)
        self.assertIn("collect_preflight_check_reason=latest_resolved_log_missing_default_skip", combined)
        self.assertIn("collect_preflight_check=skipped", combined)

    def test_pass_when_collect_preflight_latest_resolved_missing_by_default_with_review_required(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-45
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c45_readiness_missing_default.log --require-enabled
                - missing_log_review_command=rg -n 'collect_preflight_log_resolved|collect_preflight_log_missing|collect_preflight_check_reason|submission_readiness_retry_command' /tmp/c45_entry.md
                - collect_report_review_command=python scripts/check_c_team_collect_preflight_report.py /tmp/c45_readiness_missing_default.log --require-enabled --expect-team-status docs/team_status.md
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_REQUIRE_REVIEW_COMMANDS": "1",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        combined = proc.stdout + proc.stderr
        self.assertIn("collect_preflight_log_resolved=/tmp/c45_readiness_missing_default.log", combined)
        self.assertIn("collect_preflight_log_missing=/tmp/c45_readiness_missing_default.log", combined)
        self.assertIn("collect_preflight_check_reason=latest_resolved_log_missing_default_skip", combined)
        self.assertIn("collect_preflight_check=skipped", combined)
        self.assertIn("submission_readiness_collect_preflight_check=skipped", combined)
        self.assertIn(
            "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_default_skip",
            combined,
        )
        self.assertNotIn("submission_readiness_retry_command=", combined)
        self.assertNotIn("submission_readiness_fail_step=collect_preflight", combined)
        self.assertIn("review_command_check=pass", combined)

    def test_fail_when_collect_preflight_latest_resolved_log_file_is_missing_and_required(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-37
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c37_readiness_missing_strict.log --require-enabled
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_COLLECT_LATEST_REQUIRE_FOUND": "1",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn("collect_preflight_log_resolved=/tmp/c37_readiness_missing_strict.log", combined)
        self.assertIn("collect_preflight_log_missing=/tmp/c37_readiness_missing_strict.log", combined)
        self.assertIn("collect_preflight_check_reason=latest_resolved_log_missing_strict", combined)
        self.assertIn("collect_preflight_check=fail", combined)
        self.assertIn("submission_readiness_retry_command=", combined)
        self.assertIn("C_COLLECT_LATEST_REQUIRE_FOUND=1", combined)
        strict_trace_stderr = proc.stderr
        idx_reason = strict_trace_stderr.find(
            "collect_preflight_check_reason=latest_resolved_log_missing_strict"
        )
        idx_summary = strict_trace_stderr.find(
            "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_strict"
        )
        idx_retry = strict_trace_stderr.find("submission_readiness_retry_command=")
        idx_fail_step = strict_trace_stderr.find("submission_readiness_fail_step=collect_preflight")
        self.assertGreaterEqual(idx_reason, 0, msg=strict_trace_stderr)
        self.assertGreater(idx_summary, idx_reason, msg=strict_trace_stderr)
        self.assertGreater(idx_retry, idx_summary, msg=strict_trace_stderr)
        self.assertGreater(idx_fail_step, idx_retry, msg=strict_trace_stderr)

    def test_fail_when_collect_preflight_latest_resolved_missing_strict_with_review_required(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-45
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c45_readiness_missing_strict.log --require-enabled
                - missing_log_review_command=rg -n 'collect_preflight_log_resolved|collect_preflight_log_missing|collect_preflight_check_reason|submission_readiness_retry_command' /tmp/c45_entry.md
                - collect_report_review_command=python scripts/check_c_team_collect_preflight_report.py /tmp/c45_readiness_missing_strict.log --require-enabled --expect-team-status docs/team_status.md
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_COLLECT_LATEST_REQUIRE_FOUND": "1",
                "C_REQUIRE_REVIEW_COMMANDS": "1",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn("collect_preflight_log_resolved=/tmp/c45_readiness_missing_strict.log", combined)
        self.assertIn("collect_preflight_log_missing=/tmp/c45_readiness_missing_strict.log", combined)
        self.assertIn("collect_preflight_check_reason=latest_resolved_log_missing_strict", combined)
        self.assertIn("collect_preflight_check=fail", combined)
        self.assertIn("submission_readiness_collect_preflight_check=fail", combined)
        self.assertIn(
            "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_strict",
            combined,
        )
        self.assertIn("submission_readiness_retry_command=", combined)
        self.assertIn("C_COLLECT_LATEST_REQUIRE_FOUND=1", combined)
        self.assertIn("C_REQUIRE_REVIEW_COMMANDS=1", combined)
        self.assertIn("submission_readiness_fail_step=collect_preflight", combined)
        strict_trace_stderr = proc.stderr
        idx_reason = strict_trace_stderr.find(
            "collect_preflight_check_reason=latest_resolved_log_missing_strict"
        )
        idx_summary = strict_trace_stderr.find(
            "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_strict"
        )
        idx_retry = strict_trace_stderr.find("submission_readiness_retry_command=")
        idx_fail_step = strict_trace_stderr.find("submission_readiness_fail_step=collect_preflight")
        self.assertGreaterEqual(idx_reason, 0, msg=strict_trace_stderr)
        self.assertGreater(idx_summary, idx_reason, msg=strict_trace_stderr)
        self.assertGreater(idx_retry, idx_summary, msg=strict_trace_stderr)
        self.assertGreater(idx_fail_step, idx_retry, msg=strict_trace_stderr)
        self.assertNotIn("review_command_check=fail", combined)

    def test_emits_fail_trace_reason_codes_when_collect_preflight_strict_fails_before_retry_check(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-55
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c55_readiness_missing_strict.log --require-enabled
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_COLLECT_LATEST_REQUIRE_FOUND": "1",
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY": "1",
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV": "1",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn(
            "fail_trace_retry_consistency_reasons=collect_preflight_check_failed_before_retry_consistency (latest_resolved_log_missing_strict)",
            combined,
        )
        self.assertIn(
            "fail_trace_retry_consistency_reason_codes=collect_preflight_latest_resolved_log_missing_strict_before_retry_consistency",
            combined,
        )
        self.assertIn(
            "fail_trace_retry_consistency_retry_command=python scripts/check_c_team_fail_trace_retry_consistency.py --team-status ",
            combined,
        )
        self.assertIn(
            "--require-retry-consistency-check-key --require-strict-env-prefix-match",
            combined,
        )
        self.assertIn("fail_trace_retry_consistency_check=unknown", combined)

    def test_pass_when_collect_preflight_log_latest_is_invalid_by_default(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-31
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c31_latest_invalid_readiness_default.log --require-enabled
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        status_path = write_status(markdown)
        Path("/tmp/c31_latest_invalid_readiness_default.log").write_text(
            "preflight_mode=enabled\n",
            encoding="utf-8",
        )
        proc = self.run_script(
            markdown,
            status_path=status_path,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        combined = proc.stdout + proc.stderr
        self.assertIn("missing_keys=", combined)
        self.assertIn("collect_preflight_check_reason=latest_invalid_report_default_skip", combined)
        self.assertIn("collect_preflight_check=skipped", combined)

    def test_fail_when_collect_preflight_log_latest_is_required_and_invalid(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-31
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c31_latest_invalid_readiness.log --require-enabled
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        status_path = write_status(markdown)
        Path("/tmp/c31_latest_invalid_readiness.log").write_text(
            "preflight_mode=enabled\n",
            encoding="utf-8",
        )
        proc = self.run_script(
            markdown,
            status_path=status_path,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_COLLECT_LATEST_REQUIRE_FOUND": "1",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn("missing_keys=", combined)
        self.assertIn("collect_preflight_check_reason=latest_invalid_report_strict", combined)
        self.assertIn("collect_preflight_check=fail", combined)

    def test_emits_fail_trace_reason_codes_for_latest_invalid_report_strict(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-55
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c55_readiness_invalid_strict.log --require-enabled
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        status_path = write_status(markdown)
        Path("/tmp/c55_readiness_invalid_strict.log").write_text(
            "preflight_mode=enabled\n",
            encoding="utf-8",
        )
        proc = self.run_script(
            markdown,
            status_path=status_path,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_COLLECT_LATEST_REQUIRE_FOUND": "1",
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV": "1",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn("collect_preflight_check_reason=latest_invalid_report_strict", combined)
        self.assertIn(
            "fail_trace_retry_consistency_reason_codes=collect_preflight_latest_invalid_report_strict_before_retry_consistency",
            combined,
        )
        self.assertIn("fail_trace_retry_consistency_check=unknown", combined)

    def test_pass_when_collect_preflight_latest_prefers_checker_command_over_collect_log_out(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-33
              - SESSION_TIMER_START
              - SESSION_TIMER_END
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - 変更ファイル: `docs/team_status.md`
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c33_checker_readiness.log --require-enabled
              - collect_log_out=/tmp/c33_collect_only_readiness.log
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail: PASS
            """
        )
        status_path = write_status(markdown)
        Path("/tmp/c33_checker_readiness.log").write_text(
            "collect_result=PASS\n"
            "preflight_mode=enabled\n"
            f"preflight_team_status={status_path}\n"
            "preflight_result=pass\n",
            encoding="utf-8",
        )
        Path("/tmp/c33_collect_only_readiness.log").write_text(
            "preflight_mode=enabled\n",
            encoding="utf-8",
        )
        proc = self.run_script(
            markdown,
            status_path=status_path,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_TEAM_SKIP_STAGING_BUNDLE": "1",
            },
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("collect_preflight_log_resolved=/tmp/c33_checker_readiness.log", proc.stdout)
        self.assertIn("PASS: C-team submission readiness", proc.stdout)


if __name__ == "__main__":
    unittest.main()
