#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import textwrap
import unittest
import os
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "run_c_team_staging_checks.sh"


def write_status(markdown: str) -> str:
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
        fp.write(markdown)
        return fp.name


class RunCTeamStagingChecksTest(unittest.TestCase):
    def run_script(
        self,
        markdown: str,
        extra_env: dict[str, str] | None = None,
        status_path: str | None = None,
    ) -> subprocess.CompletedProcess[str]:
        if status_path is None:
            status_path = write_status(markdown)
        env = os.environ.copy()
        env.setdefault("C_SKIP_NESTED_SELFTESTS", "1")
        if extra_env:
            env.update(extra_env)
        return subprocess.run(
            ["bash", str(SCRIPT_PATH), status_path],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
            env=env,
        )

    def test_pass_with_valid_c_section_entry(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-19
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - dryrun_result=fail
            """
        )
        proc = self.run_script(markdown)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("PASS: C-team staging checks", proc.stdout)
        self.assertIn("review_command_check=skipped", proc.stdout)
        self.assertIn("collect_preflight_check_reason=latest_not_found_default_skip", proc.stdout)

    def test_fail_trace_env_alias_is_honored_when_require_prefix_is_empty(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-58
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c58.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
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
            },
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        combined = proc.stdout + proc.stderr
        self.assertIn("skip_reason=C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY=0", combined)
        self.assertIn("fail_trace_retry_consistency_check=skipped", combined)

    def test_review_command_check_pass_when_required(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-42
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - missing_log_review_command=rg -n 'collect_preflight' /tmp/entry.md
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={"C_REQUIRE_REVIEW_COMMANDS": "1"},
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("review_command_check=pass", proc.stdout)
        self.assertIn("review_command_fail_reason=-", proc.stdout)
        self.assertIn("review_command_fail_reason_codes=-", proc.stdout)
        self.assertIn("review_command_fail_reason_codes_source=-", proc.stdout)
        self.assertIn(
            "review_command_retry_command=python scripts/check_c_team_review_commands.py --team-status ",
            proc.stdout,
        )

    def test_review_command_check_fail_when_required_and_missing(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-42
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={"C_REQUIRE_REVIEW_COMMANDS": "1"},
        )
        self.assertNotEqual(proc.returncode, 0)
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

    def test_review_command_check_fail_with_multiple_missing_reasons(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-58
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
            extra_env={"C_REQUIRE_REVIEW_COMMANDS": "1"},
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

    def test_fail_trace_retry_consistency_check_fails_when_finalize_retry_missing(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-50
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - fail_trace_audit_result=FAIL
              - fail_trace_audit_retry_reason=audit_result_FAIL
              - fail_trace_audit_retry_command=scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 30 | tee /tmp/c50_retry.log
            """
        )
        proc = self.run_script(markdown)
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn("missing fail_trace_finalize_retry_command", combined)
        self.assertIn("fail_trace_retry_consistency_check=fail", proc.stderr)

    def test_fail_trace_retry_consistency_check_can_be_skipped(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-50
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - fail_trace_audit_result=FAIL
              - fail_trace_audit_retry_reason=audit_result_FAIL
              - fail_trace_audit_retry_command=scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 30 | tee /tmp/c50_retry.log
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={"C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY": "0"},
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("fail_trace_retry_consistency_check=skipped", proc.stdout)

    def test_fail_trace_retry_consistency_key_required_and_missing(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-51
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - fail_trace_audit_result=PASS
              - fail_trace_audit_retry_command=scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 45 | tee /tmp/c51_retry.log
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={"C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY": "1"},
        )
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("missing fail_trace_retry_consistency_check", proc.stderr)

    def test_fail_trace_retry_consistency_key_required_and_present(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-51
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - fail_trace_audit_result=PASS
              - fail_trace_retry_consistency_check=pass
              - fail_trace_audit_retry_command=scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 45 | tee /tmp/c51_retry.log
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={"C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY": "1"},
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("fail_trace_retry_consistency_reasons=-", proc.stdout)
        self.assertIn("fail_trace_retry_consistency_reason_codes=-", proc.stdout)
        self.assertIn("fail_trace_retry_consistency_check=pass", proc.stdout)

    def test_fail_trace_retry_consistency_strict_env_required_and_present(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-53
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
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
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY": "1",
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV": "1",
            },
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("fail_trace_retry_consistency_check=pass", proc.stdout)

    def test_fail_trace_retry_consistency_strict_env_required_but_missing(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-53
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
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
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY": "1",
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV": "1",
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

    def test_fail_when_entry_outside_c_section(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-12
              - start_epoch=100
              - end_epoch=2000
              - elapsed_min=30
              - 実行コマンド: `make -C FEM4C`

            ## PMチーム
            - 実行タスク: C-19
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - dryrun_result=fail
            """
        )
        proc = self.run_script(markdown)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("latest C entry is outside ## Cチーム section", proc.stdout)

    def test_fail_when_coupled_freeze_path_detected(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-19
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - dryrun_result=pass
              - 変更ファイル: `FEM4C/src/analysis/runner.c`
            """
        )
        proc = self.run_script(markdown)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("forbidden_paths_detected", proc.stdout)

    def test_fail_when_coupled_freeze_file_missing(self):
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
        proc = self.run_script(markdown, extra_env={"COUPLED_FREEZE_FILE": "/tmp/not-found.txt"})
        self.assertEqual(proc.returncode, 1)
        self.assertIn("file_not_found", proc.stdout)

    def test_timer_policy_fails_without_end_epoch(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-19
              - start_epoch=200
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - dryrun_result=pass
            """
        )
        proc = self.run_script(markdown, extra_env={"C_DRYRUN_POLICY": "pass_section_freeze_timer"})
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("missing end_epoch", proc.stdout)

    def test_writes_team_status_block_output(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-22
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
            """
        )
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            out_path = fp.name
        proc = self.run_script(markdown, extra_env={"C_TEAM_STATUS_BLOCK_OUT": out_path})
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        rendered = Path(out_path).read_text(encoding="utf-8")
        self.assertIn("`dryrun_result=pass`", rendered)
        self.assertIn(f"team_status_block_output={out_path}", proc.stdout)

    def test_optionally_applies_block_to_team_status(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-23
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - pass/fail:
                - PASS
            """
        )
        status_path = write_status(markdown)
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            out_path = fp.name
        proc = self.run_script(
            markdown,
            status_path=status_path,
            extra_env={
                "C_TEAM_STATUS_BLOCK_OUT": out_path,
                "C_APPLY_BLOCK_TO_TEAM_STATUS": "1",
            },
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        updated = Path(status_path).read_text(encoding="utf-8")
        self.assertIn("dry-run 生出力（strict-safe 記録）", updated)
        self.assertIn("team_status_block_apply=updated", proc.stdout)

    def test_optionally_checks_collect_preflight_report(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-29
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
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
            preflight_log = fp.name
        proc = self.run_script(
            markdown,
            status_path=status_path,
            extra_env={"C_COLLECT_PREFLIGHT_LOG": preflight_log},
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("C_TEAM_COLLECT_PREFLIGHT_REPORT", proc.stdout)
        self.assertIn("verdict=PASS", proc.stdout)

    def test_collect_preflight_report_fails_when_not_enabled(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-29
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
            """
        )
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            fp.write(
                "collect_result=PASS\n"
                "preflight_mode=disabled\n"
                "preflight_result=skipped\n"
            )
            preflight_log = fp.name
        proc = self.run_script(markdown, extra_env={"C_COLLECT_PREFLIGHT_LOG": preflight_log})
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn("preflight_mode=disabled", combined)
        self.assertIn("verdict=FAIL", combined)

    def test_collect_preflight_report_fails_when_explicit_log_is_missing(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-37
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
            """
        )
        missing_log = "/tmp/c37_explicit_missing_staging.log"
        Path(missing_log).unlink(missing_ok=True)
        proc = self.run_script(
            markdown,
            extra_env={"C_COLLECT_PREFLIGHT_LOG": missing_log},
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn(f"collect_preflight_log_missing={missing_log}", combined)
        self.assertIn("collect_preflight_check_reason=explicit_log_missing", combined)
        self.assertIn("collect_preflight_check=fail", combined)

    def test_collect_preflight_report_fails_when_team_status_mismatch(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-29
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
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
            preflight_log = fp.name
        proc = self.run_script(
            markdown,
            status_path=status_path,
            extra_env={"C_COLLECT_PREFLIGHT_LOG": preflight_log},
        )
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("expected_preflight_team_status=", proc.stdout + proc.stderr)

    def test_collect_preflight_report_passes_with_explicit_expected_override(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-29
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
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
            preflight_log = fp.name
        proc = self.run_script(
            markdown,
            status_path=status_path,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": preflight_log,
                "C_COLLECT_EXPECT_TEAM_STATUS": explicit_expected,
            },
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("verdict=PASS", proc.stdout)

    def test_collect_preflight_report_latest_resolution_passes(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-30
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c30_latest_staging.log --require-enabled
            """
        )
        status_path = write_status(markdown)
        Path("/tmp/c30_latest_staging.log").write_text(
            "collect_result=PASS\n"
            "preflight_mode=enabled\n"
            f"preflight_team_status={status_path}\n"
            "preflight_result=pass\n",
            encoding="utf-8",
        )
        proc = self.run_script(
            markdown,
            status_path=status_path,
            extra_env={"C_COLLECT_PREFLIGHT_LOG": "latest"},
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("collect_preflight_log_resolved=/tmp/c30_latest_staging.log", proc.stdout)
        self.assertIn("collect_preflight_check=pass", proc.stdout)

    def test_collect_preflight_report_latest_auto_resolution_when_env_unset(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-30
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c30_latest_staging_auto.log --require-enabled
            """
        )
        status_path = write_status(markdown)
        Path("/tmp/c30_latest_staging_auto.log").write_text(
            "collect_result=PASS\n"
            "preflight_mode=enabled\n"
            f"preflight_team_status={status_path}\n"
            "preflight_result=pass\n",
            encoding="utf-8",
        )
        proc = self.run_script(markdown, status_path=status_path)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("collect_preflight_log_resolved=/tmp/c30_latest_staging_auto.log", proc.stdout)
        self.assertIn("collect_preflight_check=pass", proc.stdout)

    def test_collect_preflight_report_can_be_disabled_with_explicit_empty(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-30
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c30_latest_staging_disable.log --require-enabled
            """
        )
        status_path = write_status(markdown)
        Path("/tmp/c30_latest_staging_disable.log").write_text(
            "collect_result=PASS\n"
            "preflight_mode=enabled\n"
            f"preflight_team_status={status_path}\n"
            "preflight_result=pass\n",
            encoding="utf-8",
        )
        proc = self.run_script(
            markdown,
            status_path=status_path,
            extra_env={"C_COLLECT_PREFLIGHT_LOG": ""},
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("collect_preflight_check=skipped", proc.stdout)
        self.assertNotIn("collect_preflight_log_resolved=", proc.stdout)

    def test_collect_preflight_report_latest_required_fails_when_missing(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-30
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_COLLECT_LATEST_REQUIRE_FOUND": "1",
            },
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn("collect_log_not_found", combined)
        self.assertIn("collect_preflight_check_reason=latest_not_found_strict", combined)
        self.assertIn("collect_preflight_check=fail", combined)

    def test_collect_preflight_report_latest_resolved_log_missing_skips_by_default(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-37
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c37_latest_missing_staging_default.log --require-enabled
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={"C_COLLECT_PREFLIGHT_LOG": "latest"},
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        combined = proc.stdout + proc.stderr
        self.assertIn(
            "collect_preflight_log_resolved=/tmp/c37_latest_missing_staging_default.log",
            combined,
        )
        self.assertIn(
            "collect_preflight_log_missing=/tmp/c37_latest_missing_staging_default.log",
            combined,
        )
        self.assertIn(
            "collect_preflight_check_reason=latest_resolved_log_missing_default_skip",
            combined,
        )
        self.assertIn("collect_preflight_check=skipped", combined)
        self.assertIn("submission_readiness_collect_preflight_check=skipped", combined)
        self.assertIn(
            "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_default_skip",
            combined,
        )
        self.assertNotIn("submission_readiness_retry_command=", combined)
        self.assertNotIn("submission_readiness_fail_step=collect_preflight", combined)

    def test_collect_preflight_report_latest_resolved_missing_default_with_review_required(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-46
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - missing_log_review_command=rg -n 'collect_preflight' /tmp/c46_entry.md
              - collect_report_review_command=python scripts/check_c_team_collect_preflight_report.py /tmp/c46_latest_missing_staging_default.log --require-enabled --expect-team-status docs/team_status.md
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c46_latest_missing_staging_default.log --require-enabled
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_REQUIRE_REVIEW_COMMANDS": "1",
            },
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        combined = proc.stdout + proc.stderr
        self.assertIn(
            "collect_preflight_check_reason=latest_resolved_log_missing_default_skip",
            combined,
        )
        self.assertIn("submission_readiness_collect_preflight_check=skipped", combined)
        self.assertNotIn("submission_readiness_retry_command=", combined)
        self.assertNotIn("submission_readiness_fail_step=collect_preflight", combined)
        self.assertIn("review_command_check=pass", combined)

    def test_collect_preflight_report_latest_resolved_log_missing_fails_in_strict_mode(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-37
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c37_latest_missing_staging_strict.log --require-enabled
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_COLLECT_LATEST_REQUIRE_FOUND": "1",
            },
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn(
            "collect_preflight_log_resolved=/tmp/c37_latest_missing_staging_strict.log",
            combined,
        )
        self.assertIn(
            "collect_preflight_log_missing=/tmp/c37_latest_missing_staging_strict.log",
            combined,
        )
        self.assertIn(
            "collect_preflight_check_reason=latest_resolved_log_missing_strict",
            combined,
        )
        self.assertIn("collect_preflight_check=fail", combined)
        self.assertIn("submission_readiness_collect_preflight_check=fail", combined)
        self.assertIn(
            "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_strict",
            combined,
        )
        self.assertIn("submission_readiness_retry_command=", combined)
        self.assertIn("C_COLLECT_LATEST_REQUIRE_FOUND=1", combined)
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

    def test_collect_preflight_report_latest_resolved_missing_strict_with_review_required(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-46
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - missing_log_review_command=rg -n 'collect_preflight' /tmp/c46_entry.md
              - collect_report_review_command=python scripts/check_c_team_collect_preflight_report.py /tmp/c46_latest_missing_staging_strict.log --require-enabled --expect-team-status docs/team_status.md
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c46_latest_missing_staging_strict.log --require-enabled
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_COLLECT_LATEST_REQUIRE_FOUND": "1",
                "C_REQUIRE_REVIEW_COMMANDS": "1",
            },
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn(
            "collect_preflight_check_reason=latest_resolved_log_missing_strict",
            combined,
        )
        self.assertIn("submission_readiness_retry_command=", combined)
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
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c55_staging_missing_strict.log --require-enabled
            """
        )
        proc = self.run_script(
            markdown,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_COLLECT_LATEST_REQUIRE_FOUND": "1",
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV": "1",
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
            "--require-strict-env-prefix-match",
            combined,
        )
        self.assertIn("fail_trace_retry_consistency_check=unknown", combined)

    def test_collect_preflight_report_latest_invalid_report_skips_by_default(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-31
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c31_latest_invalid_staging_default.log --require-enabled
            """
        )
        status_path = write_status(markdown)
        Path("/tmp/c31_latest_invalid_staging_default.log").write_text(
            "preflight_mode=enabled\n",
            encoding="utf-8",
        )
        proc = self.run_script(
            markdown,
            status_path=status_path,
            extra_env={"C_COLLECT_PREFLIGHT_LOG": "latest"},
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        combined = proc.stdout + proc.stderr
        self.assertIn("missing_keys=", combined)
        self.assertIn("collect_preflight_check_reason=latest_invalid_report_default_skip", combined)
        self.assertIn("collect_preflight_check=skipped", combined)

    def test_collect_preflight_report_latest_required_fails_when_invalid_report(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-31
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c31_latest_invalid_staging.log --require-enabled
            """
        )
        status_path = write_status(markdown)
        Path("/tmp/c31_latest_invalid_staging.log").write_text(
            "preflight_mode=enabled\n",
            encoding="utf-8",
        )
        proc = self.run_script(
            markdown,
            status_path=status_path,
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_COLLECT_LATEST_REQUIRE_FOUND": "1",
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
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c55_staging_invalid_strict.log --require-enabled
            """
        )
        status_path = write_status(markdown)
        Path("/tmp/c55_staging_invalid_strict.log").write_text(
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

    def test_collect_preflight_report_latest_prefers_checker_command_over_collect_log_out(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-33
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c33_checker_staging.log --require-enabled
              - collect_log_out=/tmp/c33_collect_only_staging.log
            """
        )
        status_path = write_status(markdown)
        Path("/tmp/c33_checker_staging.log").write_text(
            "collect_result=PASS\n"
            "preflight_mode=enabled\n"
            f"preflight_team_status={status_path}\n"
            "preflight_result=pass\n",
            encoding="utf-8",
        )
        Path("/tmp/c33_collect_only_staging.log").write_text(
            "preflight_mode=enabled\n",
            encoding="utf-8",
        )
        proc = self.run_script(
            markdown,
            status_path=status_path,
            extra_env={"C_COLLECT_PREFLIGHT_LOG": "latest"},
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("collect_preflight_log_resolved=/tmp/c33_checker_staging.log", proc.stdout)
        self.assertIn("collect_preflight_check=pass", proc.stdout)

    def test_nested_selftests_ignore_strict_latest_env(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-35
              - start_epoch=200
              - end_epoch=2000
              - elapsed_min=30
              - scripts/c_stage_dryrun.sh --log /tmp/c.log
              - safe_stage_command=git add docs/team_status.md
              - dryrun_result=pass
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
            preflight_log = fp.name
        proc = self.run_script(
            markdown,
            status_path=status_path,
            extra_env={
                "C_SKIP_NESTED_SELFTESTS": "0",
                "C_COLLECT_PREFLIGHT_LOG": preflight_log,
                "C_COLLECT_EXPECT_TEAM_STATUS": status_path,
                "C_COLLECT_LATEST_REQUIRE_FOUND": "1",
            },
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("[18/22] test_run_c_team_collect_preflight_check.py", proc.stdout)
        self.assertIn("[21/22] test_c_team_review_reason_utils.py", proc.stdout)
        self.assertIn("PASS: C-team staging checks", proc.stdout)

    def test_script_unsets_strict_latest_env_before_nested_selftests(self):
        script_text = SCRIPT_PATH.read_text(encoding="utf-8")
        self.assertIn("unset C_COLLECT_LATEST_REQUIRE_FOUND", script_text)
        self.assertIn("unset C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY", script_text)
        self.assertIn("unset C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY", script_text)
        self.assertIn("unset C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV", script_text)
        self.assertIn("C_NESTED_SELFTEST_LOCK", script_text)
        self.assertIn("flock", script_text)


if __name__ == "__main__":
    unittest.main()
