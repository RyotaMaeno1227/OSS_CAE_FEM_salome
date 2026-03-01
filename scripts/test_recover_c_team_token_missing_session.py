#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import textwrap
import unittest
import os
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "recover_c_team_token_missing_session.sh"


def write_temp(text: str, suffix: str) -> str:
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=suffix, delete=False) as fp:
        fp.write(text)
        return fp.name


def parse_session_token(output: str) -> str:
    for line in output.splitlines():
        if line.startswith("session_token="):
            return line.split("=", 1)[1].strip()
    return ""


def end_timer_if_present(token_path: str) -> None:
    if not token_path:
        return
    token = Path(token_path)
    if not token.exists():
        return
    subprocess.run(
        ["bash", "scripts/session_timer.sh", "end", token_path],
        cwd=REPO_ROOT,
        text=True,
        capture_output=True,
        check=False,
    )


class RecoverCTeamTokenMissingSessionTest(unittest.TestCase):
    def sample_team_status(self) -> str:
        return textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-24
              - start_epoch=321
              - タイマー出力（終了）:
            ```text
            SESSION_TIMER_END
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

    def test_help_includes_collect_latest_require_found_option(self) -> None:
        proc = subprocess.run(
            ["bash", str(SCRIPT_PATH), "--help"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("--collect-latest-require-found 0|1", proc.stdout)
        self.assertIn("--guard-checkpoint-minutes <minutes>", proc.stdout)
        self.assertIn("--fail-trace-audit-log /tmp/c_team_fail_trace_audit.log", proc.stdout)

    def test_recover_marks_entry_and_starts_new_session(self) -> None:
        status_path = write_temp(self.sample_team_status(), ".md")
        env = os.environ.copy()
        env.pop("C_REQUIRE_REVIEW_COMMANDS", None)
        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--team-status",
                status_path,
                "--target-start-epoch",
                "321",
                "--token-path",
                "/tmp/missing.token",
                "--new-team-tag",
                "c_team",
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
            env=env,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("[1/2] mark token-missing entry", proc.stdout)
        self.assertIn("[2/2] start new session timer", proc.stdout)
        self.assertIn("next_finalize_command=", proc.stdout)
        self.assertIn("next_finalize_command_strict_latest=", proc.stdout)
        self.assertIn(
            f"next_finalize_fail_trace_audit_command=scripts/run_c_team_fail_trace_audit.sh {status_path} 30",
            proc.stdout,
        )
        new_token = parse_session_token(proc.stdout)
        self.assertTrue(new_token)
        fail_trace_log = f"/tmp/c_team_fail_trace_audit_{Path(new_token).name.removesuffix('.token')}.log"
        self.assertIn(
            f"next_finalize_fail_trace_audit_log={fail_trace_log}",
            proc.stdout,
        )
        self.assertIn(
            f"next_finalize_command_with_fail_trace_log=bash scripts/recover_c_team_token_missing_session.sh --team-status {status_path} --finalize-session-token {new_token} --task-title \"<task>\" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30 --fail-trace-audit-log {fail_trace_log}",
            proc.stdout,
        )
        self.assertIn(
            f"next_finalize_fail_trace_embed_command=scripts/run_c_team_fail_trace_audit.sh {status_path} 30 | tee {fail_trace_log} && bash scripts/recover_c_team_token_missing_session.sh --team-status {status_path} --finalize-session-token {new_token} --task-title \"<task>\" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30 --fail-trace-audit-log {fail_trace_log}",
            proc.stdout,
        )
        self.assertIn(
            f"next_finalize_command_strict_latest_with_fail_trace_log=bash scripts/recover_c_team_token_missing_session.sh --team-status {status_path} --finalize-session-token {new_token} --task-title \"<task>\" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30 --collect-latest-require-found 1 --fail-trace-audit-log {fail_trace_log}",
            proc.stdout,
        )
        self.assertIn(
            f"next_finalize_fail_trace_embed_command_strict_latest=scripts/run_c_team_fail_trace_audit.sh {status_path} 30 | tee {fail_trace_log} && bash scripts/recover_c_team_token_missing_session.sh --team-status {status_path} --finalize-session-token {new_token} --task-title \"<task>\" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30 --collect-latest-require-found 1 --fail-trace-audit-log {fail_trace_log}",
            proc.stdout,
        )
        self.assertIn(
            f"next_finalize_fail_trace_audit_command_strict_key=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 scripts/run_c_team_fail_trace_audit.sh {status_path} 30",
            proc.stdout,
        )
        self.assertIn(
            f"next_finalize_fail_trace_embed_command_strict_key=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 scripts/run_c_team_fail_trace_audit.sh {status_path} 30 | tee {fail_trace_log} && C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 bash scripts/recover_c_team_token_missing_session.sh --team-status {status_path} --finalize-session-token {new_token} --task-title \"<task>\" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30 --fail-trace-audit-log {fail_trace_log}",
            proc.stdout,
        )
        self.assertIn(
            f"next_finalize_fail_trace_embed_command_strict_latest_strict_key=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 scripts/run_c_team_fail_trace_audit.sh {status_path} 30 | tee {fail_trace_log} && C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 bash scripts/recover_c_team_token_missing_session.sh --team-status {status_path} --finalize-session-token {new_token} --task-title \"<task>\" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30 --collect-latest-require-found 1 --fail-trace-audit-log {fail_trace_log}",
            proc.stdout,
        )
        self.assertIn(
            f"next_finalize_fail_trace_audit_command_strict_key_strict_env=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=1 scripts/run_c_team_fail_trace_audit.sh {status_path} 30",
            proc.stdout,
        )
        self.assertIn(
            f"next_finalize_fail_trace_embed_command_strict_key_strict_env=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=1 scripts/run_c_team_fail_trace_audit.sh {status_path} 30 | tee {fail_trace_log} && C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=1 bash scripts/recover_c_team_token_missing_session.sh --team-status {status_path} --finalize-session-token {new_token} --task-title \"<task>\" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30 --fail-trace-audit-log {fail_trace_log}",
            proc.stdout,
        )
        self.assertIn(
            f"next_finalize_fail_trace_embed_command_strict_latest_strict_key_strict_env=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=1 scripts/run_c_team_fail_trace_audit.sh {status_path} 30 | tee {fail_trace_log} && C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=1 bash scripts/recover_c_team_token_missing_session.sh --team-status {status_path} --finalize-session-token {new_token} --task-title \"<task>\" --guard-minutes 30 --check-compliance-policy pass_section_freeze_timer_safe --check-submission-readiness-minutes 30 --collect-latest-require-found 1 --fail-trace-audit-log {fail_trace_log}",
            proc.stdout,
        )
        self.assertIn(
            "next_finalize_review_keys=collect_preflight_log_resolved collect_preflight_log_missing collect_preflight_check_reason submission_readiness_retry_command",
            proc.stdout,
        )
        self.assertIn("next_finalize_review_command=rg -n 'collect_preflight_log_resolved|collect_preflight_log_missing|collect_preflight_check_reason|submission_readiness_retry_command|review_command_fail_reason_codes_source' <entry_out_path>", proc.stdout)
        updated = Path(status_path).read_text(encoding="utf-8")
        self.assertNotIn("<pending>", updated)
        self.assertIn("token missing", updated)
        self.assertTrue(Path(new_token).exists())
        end_timer_if_present(new_token)

    def test_recover_start_mode_with_review_required_prefixes_next_commands(self) -> None:
        status_path = write_temp(self.sample_team_status(), ".md")
        env = os.environ.copy()
        env["C_REQUIRE_REVIEW_COMMANDS"] = "1"
        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--team-status",
                status_path,
                "--target-start-epoch",
                "321",
                "--token-path",
                "/tmp/missing.token",
                "--new-team-tag",
                "c_team",
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
            env=env,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn(
            "next_finalize_command=C_REQUIRE_REVIEW_COMMANDS=1 bash scripts/recover_c_team_token_missing_session.sh",
            proc.stdout,
        )
        self.assertIn(
            "next_finalize_fail_trace_audit_command=C_REQUIRE_REVIEW_COMMANDS=1 scripts/run_c_team_fail_trace_audit.sh",
            proc.stdout,
        )
        self.assertIn(
            "next_finalize_fail_trace_embed_command=C_REQUIRE_REVIEW_COMMANDS=1 scripts/run_c_team_fail_trace_audit.sh",
            proc.stdout,
        )
        self.assertIn(
            "&& C_REQUIRE_REVIEW_COMMANDS=1 bash scripts/recover_c_team_token_missing_session.sh",
            proc.stdout,
        )
        end_timer_if_present(parse_session_token(proc.stdout))

    def test_finalize_mode_collects_and_appends_entry(self) -> None:
        start = subprocess.run(
            ["bash", "scripts/session_timer.sh", "start", "c_team"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(start.returncode, 0, msg=start.stdout + start.stderr)
        token_path = parse_session_token(start.stdout)
        self.assertTrue(token_path)

        status_path = write_temp(
            textwrap.dedent(
                """\
                ## Cチーム
                - 実行タスク: C-25 precheck
                  - pass/fail:
                    - PASS

                ## PMチーム
                - 実行タスク: PM-3
                """
            ),
            ".md",
        )
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            dryrun_log = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            dryrun_block = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            guard_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            end_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            entry_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            fail_trace_audit_log = fp.name
            fp.write(
                "readiness_default_log=/tmp/c48_finalize_readiness_default.log\n"
                "readiness_strict_log=/tmp/c48_finalize_readiness_strict.log\n"
                "staging_default_log=/tmp/c48_finalize_staging_default.log\n"
                "staging_strict_log=/tmp/c48_finalize_staging_strict.log\n"
                "FAIL_TRACE_AUDIT_RESULT=PASS\n"
            )

        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--team-status",
                status_path,
                "--finalize-session-token",
                token_path,
                "--task-title",
                "C-25 finalize test",
                "--guard-checkpoint-minutes",
                "10",
                "--guard-checkpoint-minutes",
                "20",
                "--guard-minutes",
                "0",
                "--dryrun-log",
                dryrun_log,
                "--dryrun-block-out",
                dryrun_block,
                "--timer-guard-out",
                guard_out,
                "--timer-end-out",
                end_out,
                "--entry-out",
                entry_out,
                "--fail-trace-audit-log",
                fail_trace_audit_log,
                "--check-compliance-policy",
                "pass_section_freeze_timer_safe",
                "--done-line",
                "C-25 finalize mode done",
                "--in-progress-line",
                "C-26 start",
                "--command-line",
                "python scripts/test_recover_c_team_token_missing_session.py -> PASS",
                "--pass-fail-line",
                "PASS（finalize mode test）",
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("recovery_finalize_result=PASS", proc.stdout)
        self.assertIn("preflight_mode=enabled", proc.stdout)
        self.assertIn("preflight_result=pass", proc.stdout)
        guard_text = Path(guard_out).read_text(encoding="utf-8")
        self.assertIn("min_required=10", guard_text)
        self.assertIn("min_required=20", guard_text)
        self.assertIn("min_required=0", guard_text)
        updated = Path(status_path).read_text(encoding="utf-8")
        self.assertIn("C-25 finalize test", updated)
        self.assertIn("C-25 finalize mode done", updated)
        self.assertIn("guard_checkpoints=10,20", updated)
        self.assertIn("safe_stage_command=git add", updated)
        self.assertIn(f"fail_trace_audit_log={fail_trace_audit_log}", updated)
        self.assertIn("fail_trace_audit_result=PASS", updated)
        self.assertIn("fail_trace_readiness_strict_log=/tmp/c48_finalize_readiness_strict.log", updated)
        self.assertIn(
            f"fail_trace_audit_retry_command=scripts/run_c_team_fail_trace_audit.sh {status_path} 30 | tee {fail_trace_audit_log}",
            updated,
        )

    def test_finalize_mode_rejects_invalid_submission_minutes(self) -> None:
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".token", delete=False) as fp:
            token_path = fp.name
            fp.write("team_tag=c_team\nstart_utc=2026-02-14T00:00:00Z\nstart_epoch=1771000000\n")

        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--team-status",
                write_temp(self.sample_team_status(), ".md"),
                "--finalize-session-token",
                token_path,
                "--task-title",
                "C-26 invalid submission minutes",
                "--check-submission-readiness-minutes",
                "abc",
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 2, msg=proc.stdout + proc.stderr)
        self.assertIn("must be an integer", proc.stderr)

    def test_finalize_mode_rejects_invalid_guard_checkpoint_minutes(self) -> None:
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".token", delete=False) as fp:
            token_path = fp.name
            fp.write("team_tag=c_team\nstart_utc=2026-02-14T00:00:00Z\nstart_epoch=1771000000\n")

        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--team-status",
                write_temp(self.sample_team_status(), ".md"),
                "--finalize-session-token",
                token_path,
                "--task-title",
                "C-57 invalid guard checkpoint",
                "--guard-checkpoint-minutes",
                "x",
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 2, msg=proc.stdout + proc.stderr)
        self.assertIn("--guard-checkpoint-minutes must be an integer", proc.stderr)

    def test_finalize_mode_with_submission_readiness_option(self) -> None:
        start = subprocess.run(
            ["bash", "scripts/session_timer.sh", "start", "c_team"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(start.returncode, 0, msg=start.stdout + start.stderr)
        token_path = parse_session_token(start.stdout)
        self.assertTrue(token_path)

        status_path = write_temp(
            textwrap.dedent(
                """\
                ## Cチーム
                - 実行タスク: C-25 precheck
                  - pass/fail:
                    - PASS

                ## PMチーム
                - 実行タスク: PM-3
                """
            ),
            ".md",
        )
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            dryrun_log = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            dryrun_block_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            timer_guard_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            timer_end_out = fp.name

        env = os.environ.copy()
        env["C_TEAM_SKIP_STAGING_BUNDLE"] = "1"
        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--team-status",
                status_path,
                "--finalize-session-token",
                token_path,
                "--task-title",
                "C-26 finalize readiness test",
                "--guard-minutes",
                "0",
                "--check-compliance-policy",
                "pass_section_freeze_timer_safe",
                "--check-submission-readiness-minutes",
                "0",
                "--dryrun-log",
                dryrun_log,
                "--dryrun-block-out",
                dryrun_block_out,
                "--timer-guard-out",
                timer_guard_out,
                "--timer-end-out",
                timer_end_out,
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
            env=env,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("recovery_finalize_result=PASS", proc.stdout)
        self.assertIn("submission_readiness=pass", proc.stdout)
        self.assertIn("preflight_mode=enabled", proc.stdout)
        self.assertIn("preflight_result=pass", proc.stdout)

    def test_finalize_mode_review_required_fail_fast_emits_review_reason_codes(self) -> None:
        start = subprocess.run(
            ["bash", "scripts/session_timer.sh", "start", "c_team"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(start.returncode, 0, msg=start.stdout + start.stderr)
        token_path = parse_session_token(start.stdout)
        self.assertTrue(token_path)

        status_path = write_temp(
            textwrap.dedent(
                """\
                ## Cチーム
                - 実行タスク: C-58 precheck
                  - pass/fail:
                    - PASS

                ## PMチーム
                - 実行タスク: PM-3
                """
            ),
            ".md",
        )

        env = os.environ.copy()
        env["C_TEAM_SKIP_STAGING_BUNDLE"] = "1"
        env["C_REQUIRE_REVIEW_COMMANDS"] = "1"
        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--team-status",
                status_path,
                "--finalize-session-token",
                token_path,
                "--task-title",
                "C-58 finalize review required fail-fast test",
                "--guard-minutes",
                "0",
                "--check-compliance-policy",
                "pass_section_freeze_timer_safe",
                "--check-submission-readiness-minutes",
                "0",
                "--command-line",
                "python scripts/check_c_team_collect_preflight_report.py /tmp/c58_recover_collect.log --require-enabled -> PASS",
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
            env=env,
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn("review_command_check=fail", combined)
        self.assertIn(
            "review_command_fail_reason=missing collect_report_review_command",
            combined,
        )
        self.assertIn(
            "review_command_fail_reason_codes=review_command_missing_collect_report_review_command",
            combined,
        )
        self.assertIn("review_command_fail_reason_codes_source=checker", combined)
        self.assertIn("submission_readiness_fail_step=review_command", combined)
        self.assertIn(
            "review_command_retry_command=python scripts/check_c_team_review_commands.py --team-status ",
            combined,
        )
        self.assertIn("submission_readiness_retry_command=C_REQUIRE_REVIEW_COMMANDS=1", combined)

    def test_finalize_mode_with_collect_log_out(self) -> None:
        start = subprocess.run(
            ["bash", "scripts/session_timer.sh", "start", "c_team"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(start.returncode, 0, msg=start.stdout + start.stderr)
        token_path = parse_session_token(start.stdout)
        self.assertTrue(token_path)

        status_path = write_temp(
            textwrap.dedent(
                """\
                ## Cチーム
                - 実行タスク: C-28 precheck
                  - pass/fail:
                    - PASS

                ## PMチーム
                - 実行タスク: PM-3
                """
            ),
            ".md",
        )
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            collect_log_out = fp.name

        env = os.environ.copy()
        env["C_TEAM_SKIP_STAGING_BUNDLE"] = "1"
        env["C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY"] = ""
        env["C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY"] = ""
        env["C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV"] = ""
        env["C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY"] = "1"
        env["C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY"] = "1"
        env["C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV"] = "1"
        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--team-status",
                status_path,
                "--finalize-session-token",
                token_path,
                "--task-title",
                "C-28 finalize collect-log test",
                "--guard-minutes",
                "0",
                "--check-compliance-policy",
                "pass_section_freeze_timer_safe",
                "--check-submission-readiness-minutes",
                "0",
                "--collect-log-out",
                collect_log_out,
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
            env=env,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("C_TEAM_COLLECT_PREFLIGHT_REPORT", proc.stdout)
        self.assertIn("collect_log_out=", proc.stdout)
        saved = Path(collect_log_out).read_text(encoding="utf-8")
        self.assertIn("preflight_mode=enabled", saved)
        self.assertIn("preflight_result=pass", saved)
        self.assertIn(f"preflight_team_status={status_path}", saved)
        updated = Path(status_path).read_text(encoding="utf-8")
        self.assertIn(
            f"python scripts/check_c_team_collect_preflight_report.py {collect_log_out} --require-enabled -> PASS",
            updated,
        )
        self.assertIn(
            f"collect_report_review_command=python scripts/check_c_team_collect_preflight_report.py {collect_log_out} --require-enabled --expect-team-status {status_path}",
            updated,
        )

    def test_finalize_mode_rejects_invalid_collect_latest_require_found_option(self) -> None:
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".token", delete=False) as fp:
            token_path = fp.name
            fp.write("team_tag=c_team\nstart_utc=2026-02-14T00:00:00Z\nstart_epoch=1771000000\n")

        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--team-status",
                write_temp(self.sample_team_status(), ".md"),
                "--finalize-session-token",
                token_path,
                "--task-title",
                "C-32 invalid strict option",
                "--collect-latest-require-found",
                "2",
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 2, msg=proc.stdout + proc.stderr)
        self.assertIn("must be 0 or 1", proc.stderr)

    def test_finalize_mode_rejects_missing_fail_trace_audit_log_option(self) -> None:
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".token", delete=False) as fp:
            token_path = fp.name
            fp.write("team_tag=c_team\nstart_utc=2026-02-14T00:00:00Z\nstart_epoch=1771000000\n")

        status_path = write_temp(self.sample_team_status(), ".md")
        missing_path = "/tmp/c48_missing_fail_trace_finalize.log"
        Path(missing_path).unlink(missing_ok=True)
        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--team-status",
                status_path,
                "--finalize-session-token",
                token_path,
                "--task-title",
                "C-48 missing fail-trace finalize",
                "--fail-trace-audit-log",
                missing_path,
                "--check-submission-readiness-minutes",
                "30",
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 2, msg=proc.stdout + proc.stderr)
        self.assertIn("--fail-trace-audit-log not found", proc.stderr)
        self.assertIn("fail_trace_audit_retry_command=", proc.stderr)
        self.assertIn(
            f"scripts/run_c_team_fail_trace_audit.sh {status_path} 30 | tee {missing_path}",
            proc.stderr,
        )
        self.assertIn(
            f"fail_trace_retry_consistency_retry_command=python scripts/check_c_team_fail_trace_retry_consistency.py --team-status {status_path}",
            proc.stderr,
        )
        self.assertIn("fail_trace_finalize_retry_command=", proc.stderr)

    def test_finalize_mode_missing_fail_trace_audit_log_emits_strict_retry_command(self) -> None:
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".token", delete=False) as fp:
            token_path = fp.name
            fp.write("team_tag=c_team\nstart_utc=2026-02-14T00:00:00Z\nstart_epoch=1771000000\n")

        status_path = write_temp(self.sample_team_status(), ".md")
        missing_path = "/tmp/c49_missing_fail_trace_finalize.log"
        Path(missing_path).unlink(missing_ok=True)
        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--team-status",
                status_path,
                "--finalize-session-token",
                token_path,
                "--task-title",
                "C-49 missing fail-trace finalize strict",
                "--check-compliance-policy",
                "pass_section_freeze_timer_safe",
                "--check-submission-readiness-minutes",
                "30",
                "--collect-latest-require-found",
                "1",
                "--fail-trace-audit-log",
                missing_path,
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 2, msg=proc.stdout + proc.stderr)
        self.assertIn("fail_trace_audit_retry_command=", proc.stderr)
        self.assertIn(
            f"scripts/run_c_team_fail_trace_audit.sh {status_path} 30 | tee {missing_path}",
            proc.stderr,
        )
        self.assertIn(
            f"fail_trace_retry_consistency_retry_command=python scripts/check_c_team_fail_trace_retry_consistency.py --team-status {status_path}",
            proc.stderr,
        )
        self.assertIn("fail_trace_finalize_retry_command=", proc.stderr)
        self.assertIn("--collect-latest-require-found 1", proc.stderr)
        self.assertIn("--check-compliance-policy pass_section_freeze_timer_safe", proc.stderr)
        self.assertIn("--check-submission-readiness-minutes 30", proc.stderr)

    def test_finalize_mode_missing_fail_trace_log_propagates_fail_trace_env_in_retry_command(self) -> None:
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".token", delete=False) as fp:
            token_path = fp.name
            fp.write("team_tag=c_team\nstart_utc=2026-02-14T00:00:00Z\nstart_epoch=1771000000\n")

        status_path = write_temp(self.sample_team_status(), ".md")
        missing_path = "/tmp/c52_missing_fail_trace_finalize.log"
        Path(missing_path).unlink(missing_ok=True)
        env = os.environ.copy()
        env.pop("C_REQUIRE_REVIEW_COMMANDS", None)
        env["C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY"] = "1"
        env["C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY"] = "1"
        env["C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV"] = "1"
        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--team-status",
                status_path,
                "--finalize-session-token",
                token_path,
                "--task-title",
                "C-52 missing fail-trace finalize strict env",
                "--fail-trace-audit-log",
                missing_path,
                "--check-submission-readiness-minutes",
                "30",
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
            env=env,
        )
        self.assertEqual(proc.returncode, 2, msg=proc.stdout + proc.stderr)
        self.assertIn(
            f"fail_trace_audit_retry_command=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=1 scripts/run_c_team_fail_trace_audit.sh {status_path} 30 | tee {missing_path}",
            proc.stderr,
        )
        self.assertIn(
            f"fail_trace_retry_consistency_retry_command=python scripts/check_c_team_fail_trace_retry_consistency.py --team-status {status_path} --require-retry-consistency-check-key --require-strict-env-prefix-match",
            proc.stderr,
        )
        self.assertIn(
            "fail_trace_finalize_retry_command=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=1",
            proc.stderr,
        )

    def test_finalize_mode_strict_latest_requires_resolved_log(self) -> None:
        start = subprocess.run(
            ["bash", "scripts/session_timer.sh", "start", "c_team"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(start.returncode, 0, msg=start.stdout + start.stderr)
        token_path = parse_session_token(start.stdout)
        self.assertTrue(token_path)

        status_path = write_temp(
            textwrap.dedent(
                """\
                ## Cチーム
                - 実行タスク: C-31 precheck
                  - pass/fail:
                    - PASS

                ## PMチーム
                - 実行タスク: PM-3
                """
            ),
            ".md",
        )
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            entry_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            dryrun_log = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            dryrun_block_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            timer_guard_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            timer_end_out = fp.name

        env = os.environ.copy()
        env["C_TEAM_SKIP_STAGING_BUNDLE"] = "1"
        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--team-status",
                status_path,
                "--finalize-session-token",
                token_path,
                "--task-title",
                "C-32 finalize strict latest fail test",
                "--guard-minutes",
                "0",
                "--dryrun-log",
                dryrun_log,
                "--dryrun-block-out",
                dryrun_block_out,
                "--timer-guard-out",
                timer_guard_out,
                "--timer-end-out",
                timer_end_out,
                "--check-compliance-policy",
                "pass_section_freeze_timer_safe",
                "--check-submission-readiness-minutes",
                "0",
                "--collect-latest-require-found",
                "1",
                "--entry-out",
                entry_out,
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
            env=env,
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn("collect_log_not_found", combined)
        self.assertIn("collect_preflight_check_reason=latest_not_found_strict", combined)
        self.assertIn("collect_preflight_check=fail", combined)
        self.assertIn(
            "submission_readiness_retry_command=C_COLLECT_LATEST_REQUIRE_FOUND=1",
            combined,
        )
        self.assertIn(
            f"bash scripts/check_c_team_submission_readiness.sh {status_path} 0",
            combined,
        )
        self.assertIn(f"entry_out={entry_out}", combined)
        self.assertIn(
            f"missing_log_review_command=rg -n 'collect_preflight_log_resolved|collect_preflight_log_missing|collect_preflight_check_reason|submission_readiness_retry_command|review_command_fail_reason_codes_source' {entry_out}",
            combined,
        )
        entry_text = Path(entry_out).read_text(encoding="utf-8")
        self.assertIn("collect_preflight_check_reason=latest_not_found_strict", entry_text)
        self.assertIn(
            "collect_preflight_reasons=collect_preflight_check_reason=latest_not_found_strict",
            entry_text,
        )

    def test_finalize_mode_strict_latest_fails_when_resolved_log_file_is_missing(self) -> None:
        start = subprocess.run(
            ["bash", "scripts/session_timer.sh", "start", "c_team"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(start.returncode, 0, msg=start.stdout + start.stderr)
        token_path = parse_session_token(start.stdout)
        self.assertTrue(token_path)

        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            missing_log = fp.name
        Path(missing_log).unlink(missing_ok=True)
        status_path = write_temp(
            textwrap.dedent(
                f"""\
                ## Cチーム
                - 実行タスク: C-37 precheck
                  - 実行コマンド / pass-fail:
                    - python scripts/check_c_team_collect_preflight_report.py {missing_log} --require-enabled -> PASS
                  - pass/fail:
                    - PASS

                ## PMチーム
                - 実行タスク: PM-3
                """
            ),
            ".md",
        )
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            dryrun_log = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            dryrun_block_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            timer_guard_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            timer_end_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            entry_out = fp.name
        env = os.environ.copy()
        env["C_TEAM_SKIP_STAGING_BUNDLE"] = "1"
        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--team-status",
                status_path,
                "--finalize-session-token",
                token_path,
                "--task-title",
                "C-37 finalize strict latest missing resolved log test",
                "--guard-minutes",
                "0",
                "--dryrun-log",
                dryrun_log,
                "--dryrun-block-out",
                dryrun_block_out,
                "--timer-guard-out",
                timer_guard_out,
                "--timer-end-out",
                timer_end_out,
                "--entry-out",
                entry_out,
                "--check-compliance-policy",
                "pass_section_freeze_timer_safe",
                "--check-submission-readiness-minutes",
                "0",
                "--collect-latest-require-found",
                "1",
                "--command-line",
                f"python scripts/check_c_team_collect_preflight_report.py {missing_log} --require-enabled -> RUN",
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
            env=env,
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn(f"collect_preflight_log_resolved={missing_log}", combined)
        self.assertIn(f"collect_preflight_log_missing={missing_log}", combined)
        self.assertIn("collect_preflight_check_reason=latest_resolved_log_missing_strict", combined)
        self.assertIn("collect_preflight_check=fail", combined)
        self.assertIn(
            "submission_readiness_retry_command=C_COLLECT_LATEST_REQUIRE_FOUND=1",
            combined,
        )
        self.assertIn(
            f"bash scripts/check_c_team_submission_readiness.sh {status_path} 0",
            combined,
        )
        self.assertIn(f"entry_out={entry_out}", combined)
        self.assertIn(
            f"missing_log_review_command=rg -n 'collect_preflight_log_resolved|collect_preflight_log_missing|collect_preflight_check_reason|submission_readiness_retry_command|review_command_fail_reason_codes_source' {entry_out}",
            combined,
        )
        entry_text = Path(entry_out).read_text(encoding="utf-8")
        self.assertIn(f"collect_preflight_log_resolved={missing_log}", entry_text)
        self.assertIn(f"collect_preflight_log_missing={missing_log}", entry_text)
        self.assertIn(
            "collect_preflight_check_reason=latest_resolved_log_missing_strict",
            entry_text,
        )

    def test_finalize_mode_strict_latest_failure_with_collect_log_out_prints_review_commands(self) -> None:
        start = subprocess.run(
            ["bash", "scripts/session_timer.sh", "start", "c_team"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(start.returncode, 0, msg=start.stdout + start.stderr)
        token_path = parse_session_token(start.stdout)
        self.assertTrue(token_path)

        status_path = write_temp(
            textwrap.dedent(
                """\
                ## Cチーム
                - 実行タスク: C-40 strict precheck
                  - pass/fail:
                    - PASS

                ## PMチーム
                - 実行タスク: PM-3
                """
            ),
            ".md",
        )
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            collect_log_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            entry_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            dryrun_log = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            dryrun_block_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            timer_guard_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            timer_end_out = fp.name

        env = os.environ.copy()
        env["C_TEAM_SKIP_STAGING_BUNDLE"] = "1"
        env["C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY"] = "1"
        env["C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV"] = "1"
        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--team-status",
                status_path,
                "--finalize-session-token",
                token_path,
                "--task-title",
                "C-40 strict latest fail with collect-log-out",
                "--guard-minutes",
                "0",
                "--check-compliance-policy",
                "pass_section_freeze_timer_safe",
                "--check-submission-readiness-minutes",
                "0",
                "--collect-latest-require-found",
                "1",
                "--collect-log-out",
                collect_log_out,
                "--entry-out",
                entry_out,
                "--dryrun-log",
                dryrun_log,
                "--dryrun-block-out",
                dryrun_block_out,
                "--timer-guard-out",
                timer_guard_out,
                "--timer-end-out",
                timer_end_out,
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
            env=env,
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn(f"collect_log_out={collect_log_out}", combined)
        self.assertIn(f"entry_out={entry_out}", combined)
        self.assertIn(
            f"missing_log_review_command=rg -n 'collect_preflight_log_resolved|collect_preflight_log_missing|collect_preflight_check_reason|submission_readiness_retry_command|review_command_fail_reason_codes_source' {entry_out}",
            combined,
        )
        self.assertIn(
            f"collect_report_review_command=python scripts/check_c_team_collect_preflight_report.py {collect_log_out} --require-enabled --expect-team-status {status_path}",
            combined,
        )
        self.assertIn(
            "fail_trace_retry_consistency_reason_codes=collect_preflight_latest_invalid_report_strict_before_retry_consistency",
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
        self.assertIn("C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY=1", combined)
        self.assertIn("C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY=1", combined)
        self.assertIn("C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV=1", combined)


if __name__ == "__main__":
    unittest.main()
