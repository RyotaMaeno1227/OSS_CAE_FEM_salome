#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import unittest
import os
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "collect_c_team_session_evidence.sh"


def parse_session_token(output: str) -> str:
    for line in output.splitlines():
        if line.startswith("session_token="):
            return line.split("=", 1)[1].strip()
    return ""


def end_timer_if_present(token_path: str) -> None:
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


class CollectCTeamSessionEvidenceTest(unittest.TestCase):
    def sample_team_status(self) -> str:
        return (
            "## Cチーム\n"
            "- 実行タスク: C-24\n"
            "  - pass/fail:\n"
            "    - PASS\n\n"
            "## PMチーム\n"
            "- 実行タスク: PM-3\n"
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

    def test_collect_guard_checkpoints_preserve_latest_guard_block(self) -> None:
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

        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            timer_guard_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            timer_end_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            entry_out = fp.name

        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-57 guard checkpoint boundary",
                "--session-token",
                token_path,
                "--guard-checkpoint-minutes",
                "10",
                "--guard-checkpoint-minutes",
                "20",
                "--guard-minutes",
                "0",
                "--timer-guard-out",
                timer_guard_out,
                "--timer-end-out",
                timer_end_out,
                "--entry-out",
                entry_out,
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)

        guard_text = Path(timer_guard_out).read_text(encoding="utf-8")
        self.assertIn("min_required=10", guard_text)
        self.assertIn("min_required=20", guard_text)
        self.assertIn("min_required=0", guard_text)
        self.assertIn("guard_result=block", guard_text)
        self.assertIn("guard_result=pass", guard_text)

        entry_text = Path(entry_out).read_text(encoding="utf-8")
        self.assertIn("SESSION_TIMER_GUARD", entry_text)
        self.assertIn("guard_checkpoints=10,20", entry_text)
        self.assertIn("min_required=0", entry_text)
        self.assertIn("guard_result=pass", entry_text)

    def test_collect_uses_mktemp_defaults_for_dryrun_outputs(self) -> None:
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

        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-56 mktemp default outputs",
                "--session-token",
                token_path,
                "--guard-minutes",
                "0",
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)

        dryrun_log = ""
        dryrun_block_out = ""
        for line in proc.stdout.splitlines():
            if line.startswith("dryrun_log="):
                dryrun_log = line.split("=", 1)[1].strip()
            if line.startswith("dryrun_block_out="):
                dryrun_block_out = line.split("=", 1)[1].strip()

        self.assertTrue(dryrun_log.startswith("/tmp/c_stage_dryrun_auto."))
        self.assertTrue(dryrun_log.endswith(".log"))
        self.assertNotEqual(dryrun_log, "/tmp/c_stage_dryrun_auto.log")
        self.assertFalse(Path(dryrun_log).exists())

        self.assertTrue(dryrun_block_out.startswith("/tmp/c_stage_team_status_block."))
        self.assertTrue(dryrun_block_out.endswith(".md"))
        self.assertNotEqual(dryrun_block_out, "/tmp/c_stage_team_status_block.md")
        self.assertFalse(Path(dryrun_block_out).exists())

    def test_collect_includes_fail_trace_review_commands_when_audit_log_is_provided(self) -> None:
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
            fail_trace_audit_log = fp.name
            fp.write(
                "readiness_default_log=/tmp/c48_readiness_default.log\n"
                "readiness_strict_log=/tmp/c48_readiness_strict.log\n"
                "staging_default_log=/tmp/c48_staging_default.log\n"
                "staging_strict_log=/tmp/c48_staging_strict.log\n"
                "FAIL_TRACE_AUDIT_RESULT=PASS\n"
            )
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            entry_out = fp.name

        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-48 fail-trace audit log include test",
                "--session-token",
                token_path,
                "--guard-minutes",
                "0",
                "--entry-out",
                entry_out,
                "--fail-trace-audit-log",
                fail_trace_audit_log,
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        entry_text = Path(entry_out).read_text(encoding="utf-8")
        self.assertIn(f"fail_trace_audit_log={fail_trace_audit_log}", entry_text)
        self.assertIn("fail_trace_audit_result=PASS", entry_text)
        self.assertIn("fail_trace_readiness_default_log=/tmp/c48_readiness_default.log", entry_text)
        self.assertIn("fail_trace_readiness_strict_log=/tmp/c48_readiness_strict.log", entry_text)
        self.assertIn("fail_trace_staging_default_log=/tmp/c48_staging_default.log", entry_text)
        self.assertIn("fail_trace_staging_strict_log=/tmp/c48_staging_strict.log", entry_text)
        self.assertIn(
            "fail_trace_readiness_default_review_command=python scripts/check_c_team_fail_trace_order.py /tmp/c48_readiness_default.log --mode default",
            entry_text,
        )
        self.assertIn(
            "fail_trace_staging_strict_review_command=python scripts/check_c_team_fail_trace_order.py /tmp/c48_staging_strict.log --mode strict",
            entry_text,
        )
        self.assertIn(
            f"fail_trace_audit_retry_command=scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 30 | tee {fail_trace_audit_log}",
            entry_text,
        )
        self.assertIn(
            "fail_trace_retry_consistency_command=python scripts/check_c_team_fail_trace_retry_consistency.py --team-status docs/team_status.md",
            entry_text,
        )
        self.assertIn("fail_trace_retry_consistency_check=unknown", entry_text)
        self.assertIn(
            "fail_trace_retry_consistency_retry_command=python scripts/check_c_team_fail_trace_retry_consistency.py --team-status docs/team_status.md",
            entry_text,
        )

    def test_collect_includes_fail_trace_retry_hints_when_audit_log_is_incomplete(self) -> None:
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
            fail_trace_audit_log = fp.name
            fp.write(
                "readiness_default_log=/tmp/c49_readiness_default.log\n"
                "FAIL_TRACE_AUDIT_RESULT=FAIL\n"
            )
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            entry_out = fp.name

        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-49 fail-trace retry hint test",
                "--session-token",
                token_path,
                "--guard-minutes",
                "0",
                "--entry-out",
                entry_out,
                "--fail-trace-audit-log",
                fail_trace_audit_log,
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        entry_text = Path(entry_out).read_text(encoding="utf-8")
        self.assertIn("fail_trace_audit_result=FAIL", entry_text)
        self.assertIn(
            f"fail_trace_audit_retry_command=scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 30 | tee {fail_trace_audit_log}",
            entry_text,
        )
        self.assertIn("fail_trace_audit_retry_reason=audit_result_FAIL", entry_text)
        self.assertIn(
            "fail_trace_audit_missing_keys=readiness_strict_log staging_default_log staging_strict_log",
            entry_text,
        )
        self.assertIn("fail_trace_retry_consistency_check=unknown", entry_text)
        self.assertIn(
            "fail_trace_retry_consistency_retry_command=python scripts/check_c_team_fail_trace_retry_consistency.py --team-status docs/team_status.md",
            entry_text,
        )
        self.assertIn("fail_trace_finalize_retry_command=bash scripts/recover_c_team_token_missing_session.sh", entry_text)
        self.assertIn("--task-title \"C-49 fail-trace retry hint test\"", entry_text)
        self.assertIn(f"--finalize-session-token {token_path}", entry_text)
        self.assertIn(f"--fail-trace-audit-log {fail_trace_audit_log}", entry_text)

    def test_collect_includes_strict_retry_consistency_replay_context(self) -> None:
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
            fail_trace_audit_log = fp.name
            fp.write(
                "fail_trace_require_retry_consistency=1\n"
                "fail_trace_require_retry_consistency_key=1\n"
                "fail_trace_require_retry_consistency_strict_env=1\n"
                "C_TEAM_FAIL_TRACE_RETRY_CONSISTENCY\n"
                "require_retry_consistency_check_key=yes\n"
                "verdict=FAIL\n"
                "reasons=missing fail_trace_retry_consistency_check\n"
                "reason_codes=missing_fail_trace_retry_consistency_check\n"
                "FAIL_TRACE_AUDIT_RESULT=FAIL\n"
                "readiness_default_log=/tmp/c52_readiness_default.log\n"
            )
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            entry_out = fp.name

        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-52 strict key-required replay context test",
                "--session-token",
                token_path,
                "--guard-minutes",
                "0",
                "--entry-out",
                entry_out,
                "--fail-trace-audit-log",
                fail_trace_audit_log,
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        entry_text = Path(entry_out).read_text(encoding="utf-8")
        self.assertIn("fail_trace_retry_consistency_required=1", entry_text)
        self.assertIn("fail_trace_retry_consistency_require_key=1", entry_text)
        self.assertIn(
            "fail_trace_retry_consistency_command=python scripts/check_c_team_fail_trace_retry_consistency.py --team-status docs/team_status.md --require-retry-consistency-check-key --require-strict-env-prefix-match",
            entry_text,
        )
        self.assertIn(
            "fail_trace_retry_consistency_retry_command=python scripts/check_c_team_fail_trace_retry_consistency.py --team-status docs/team_status.md --require-retry-consistency-check-key --require-strict-env-prefix-match",
            entry_text,
        )
        self.assertIn(
            "fail_trace_audit_retry_command=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=1 scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 30 | tee ",
            entry_text,
        )
        self.assertIn(
            "fail_trace_finalize_retry_command=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=1 bash scripts/recover_c_team_token_missing_session.sh",
            entry_text,
        )
        self.assertIn("fail_trace_retry_consistency_require_strict_env=1", entry_text)
        self.assertIn("fail_trace_retry_consistency_reasons=missing fail_trace_retry_consistency_check", entry_text)
        self.assertIn(
            "fail_trace_retry_consistency_reason_codes=missing_fail_trace_retry_consistency_check",
            entry_text,
        )

    def test_collect_reads_retry_consistency_reason_fields_without_checker_block(self) -> None:
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
            fail_trace_audit_log = fp.name
            fp.write(
                "fail_trace_require_retry_consistency=1\n"
                "fail_trace_require_retry_consistency_key=1\n"
                "fail_trace_require_retry_consistency_strict_env=1\n"
                "fail_trace_retry_consistency_check=fail\n"
                "fail_trace_retry_consistency_reasons=missing strict env prefix\n"
                "fail_trace_retry_consistency_reason_codes=missing_strict_env_prefix\n"
                "FAIL_TRACE_AUDIT_RESULT=FAIL\n"
                "readiness_default_log=/tmp/c54_readiness_default.log\n"
            )
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            entry_out = fp.name

        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-54 strict env reason passthrough test",
                "--session-token",
                token_path,
                "--guard-minutes",
                "0",
                "--entry-out",
                entry_out,
                "--fail-trace-audit-log",
                fail_trace_audit_log,
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        entry_text = Path(entry_out).read_text(encoding="utf-8")
        self.assertIn("fail_trace_retry_consistency_reasons=missing strict env prefix", entry_text)
        self.assertIn("fail_trace_retry_consistency_reason_codes=missing_strict_env_prefix", entry_text)

    def test_rejects_missing_fail_trace_audit_log(self) -> None:
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

        missing_path = "/tmp/c48_missing_fail_trace_audit.log"
        Path(missing_path).unlink(missing_ok=True)
        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-48 missing fail-trace audit log",
                "--session-token",
                token_path,
                "--guard-minutes",
                "0",
                "--fail-trace-audit-log",
                missing_path,
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 2, msg=proc.stdout + proc.stderr)
        self.assertIn("--fail-trace-audit-log not found", proc.stderr)
        end_timer_if_present(token_path)

    def test_collects_entry_artifacts(self) -> None:
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
            dryrun_log = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            block_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            guard_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            end_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            entry_out = fp.name

        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-24 collect wrapper test",
                "--session-token",
                token_path,
                "--guard-minutes",
                "0",
                "--dryrun-log",
                dryrun_log,
                "--dryrun-block-out",
                block_out,
                "--timer-guard-out",
                guard_out,
                "--timer-end-out",
                end_out,
                "--entry-out",
                entry_out,
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("collect_result=PASS", proc.stdout)
        self.assertIn("preflight_mode=disabled", proc.stdout)
        self.assertIn("preflight_result=skipped", proc.stdout)
        self.assertIn("collect_preflight_reasons=-", proc.stdout)
        entry_text = Path(entry_out).read_text(encoding="utf-8")
        self.assertIn("SESSION_TIMER_START", entry_text)
        self.assertIn("SESSION_TIMER_GUARD", entry_text)
        self.assertIn("SESSION_TIMER_END", entry_text)
        self.assertIn("dryrun_result=pass", entry_text)
        self.assertIn("preflight_latest_require_found=0 (disabled)", entry_text)

    def test_collect_and_append_team_status(self) -> None:
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

        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            team_status = fp.name
            fp.write(self.sample_team_status())
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            dryrun_log = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            block_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            guard_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            end_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            entry_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            collect_log = fp.name
            fp.write(
                "collect_result=PASS\n"
                "preflight_mode=enabled\n"
                f"preflight_team_status={team_status}\n"
                "preflight_result=pass\n"
            )

        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-24 collect wrapper append test",
                "--session-token",
                token_path,
                "--guard-minutes",
                "0",
                "--dryrun-log",
                dryrun_log,
                "--dryrun-block-out",
                block_out,
                "--timer-guard-out",
                guard_out,
                "--timer-end-out",
                end_out,
                "--entry-out",
                entry_out,
                "--team-status",
                team_status,
                "--append-to-team-status",
                "--check-compliance-policy",
                "pass_section_freeze_timer_safe",
                "--collect-preflight-log",
                collect_log,
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("preflight_mode=enabled", proc.stdout)
        self.assertIn("preflight_result=pass", proc.stdout)
        self.assertIn(f"preflight_team_status={team_status}", proc.stdout)
        self.assertIn("collect_preflight_reasons=-", proc.stdout)
        self.assertIn("team_status_append=pending_validation", proc.stdout)
        self.assertIn("team_status_append=updated", proc.stdout)
        self.assertIn("compliance_check=pass", proc.stdout)
        updated = Path(team_status).read_text(encoding="utf-8")
        self.assertIn("C-24 collect wrapper append test", updated)
        self.assertNotIn("<記入>", updated)
        self.assertIn(
            "bash scripts/check_c_team_dryrun_compliance.sh",
            Path(entry_out).read_text(encoding="utf-8"),
        )
        self.assertIn(
            f"python scripts/check_c_team_collect_preflight_report.py {collect_log} --require-enabled -> PASS",
            Path(entry_out).read_text(encoding="utf-8"),
        )
        self.assertIn("collect_preflight_reasons=-", Path(entry_out).read_text(encoding="utf-8"))
        self.assertIn(
            f"collect_report_review_command=python scripts/check_c_team_collect_preflight_report.py {collect_log} --require-enabled --expect-team-status {team_status}",
            Path(entry_out).read_text(encoding="utf-8"),
        )

    def test_collect_submission_readiness_with_explicit_collect_log_uses_canonical_team_status(self) -> None:
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

        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            team_status = fp.name
            fp.write(self.sample_team_status())
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            collect_log = fp.name
            fp.write(
                "collect_result=PASS\n"
                "preflight_mode=enabled\n"
                f"preflight_team_status={team_status}\n"
                "preflight_result=pass\n"
            )

        env = os.environ.copy()
        env["C_TEAM_SKIP_STAGING_BUNDLE"] = "1"
        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-43 explicit collect-log canonical status test",
                "--session-token",
                token_path,
                "--guard-minutes",
                "0",
                "--team-status",
                team_status,
                "--append-to-team-status",
                "--check-compliance-policy",
                "pass_section_freeze_timer_safe",
                "--check-submission-readiness-minutes",
                "0",
                "--collect-preflight-log",
                collect_log,
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
            env=env,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("submission_readiness=pass", proc.stdout)
        self.assertIn(f"preflight_team_status={team_status}", proc.stdout)
        updated = Path(team_status).read_text(encoding="utf-8")
        self.assertIn("C-43 explicit collect-log canonical status test", updated)

    def test_collect_submission_readiness_with_explicit_collect_log_strict_and_review_required(self) -> None:
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

        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            team_status = fp.name
            fp.write(self.sample_team_status())
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            collect_log = fp.name
            fp.write(
                "collect_result=PASS\n"
                "preflight_mode=enabled\n"
                f"preflight_team_status={team_status}\n"
                "preflight_result=pass\n"
            )
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            entry_out = fp.name

        env = os.environ.copy()
        env["C_TEAM_SKIP_STAGING_BUNDLE"] = "1"
        env["C_REQUIRE_REVIEW_COMMANDS"] = "1"
        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-43 explicit collect-log strict+review test",
                "--session-token",
                token_path,
                "--guard-minutes",
                "0",
                "--team-status",
                team_status,
                "--append-to-team-status",
                "--check-compliance-policy",
                "pass_section_freeze_timer_safe",
                "--check-submission-readiness-minutes",
                "0",
                "--collect-preflight-log",
                collect_log,
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
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        combined = proc.stdout + proc.stderr
        self.assertIn("submission_readiness=pass", combined)
        self.assertNotIn("collect_preflight_check_reason=latest_invalid_report_strict", combined)
        self.assertIn(f"preflight_team_status={team_status}", combined)
        updated = Path(team_status).read_text(encoding="utf-8")
        self.assertIn("C-43 explicit collect-log strict+review test", updated)
        entry_text = Path(entry_out).read_text(encoding="utf-8")
        self.assertIn(
            "C_REQUIRE_REVIEW_COMMANDS=1",
            entry_text,
        )
        self.assertIn(
            f"bash scripts/check_c_team_submission_readiness.sh {team_status} 0 -> RUN（preflight gate）",
            entry_text,
        )
        self.assertIn(
            f"collect_report_review_command=python scripts/check_c_team_collect_preflight_report.py {collect_log} --require-enabled --expect-team-status {team_status}",
            entry_text,
        )
        self.assertIn(
            f"review_command_audit_command=python scripts/check_c_team_review_commands.py --team-status {team_status}",
            entry_text,
        )
        self.assertIn("review_command_required=1", entry_text)
        self.assertIn("review_command_fail_reason_codes_source=-", entry_text)
        self.assertIn(
            f"fail_trace_audit_command=scripts/run_c_team_fail_trace_audit.sh {team_status} 0",
            entry_text,
        )
        self.assertIn(
            f"fail_trace_retry_consistency_command=python scripts/check_c_team_fail_trace_retry_consistency.py --team-status {team_status}",
            entry_text,
        )
        self.assertIn("fail_trace_retry_consistency_check=unknown", entry_text)
        self.assertIn(
            f"fail_trace_retry_consistency_retry_command=python scripts/check_c_team_fail_trace_retry_consistency.py --team-status {team_status}",
            entry_text,
        )

    def test_rejects_submission_readiness_without_append(self) -> None:
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
            dryrun_log = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            block_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            guard_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            end_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            entry_out = fp.name

        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-26 invalid option test",
                "--session-token",
                token_path,
                "--guard-minutes",
                "0",
                "--dryrun-log",
                dryrun_log,
                "--dryrun-block-out",
                block_out,
                "--timer-guard-out",
                guard_out,
                "--timer-end-out",
                end_out,
                "--entry-out",
                entry_out,
                "--check-submission-readiness-minutes",
                "30",
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 2, msg=proc.stdout + proc.stderr)
        self.assertIn("requires --append-to-team-status", proc.stderr)
        end_timer_if_present(token_path)

    def test_rejects_invalid_submission_readiness_minutes(self) -> None:
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

        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-26 invalid minutes test",
                "--session-token",
                token_path,
                "--guard-minutes",
                "0",
                "--check-submission-readiness-minutes",
                "abc",
                "--append-to-team-status",
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 2, msg=proc.stdout + proc.stderr)
        self.assertIn("must be an integer", proc.stderr)
        end_timer_if_present(token_path)

    def test_compliance_preflight_without_append(self) -> None:
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

        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            team_status = fp.name
            fp.write(self.sample_team_status())
        original = Path(team_status).read_text(encoding="utf-8")
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            dryrun_log = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            block_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            guard_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            end_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            entry_out = fp.name

        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-28 preflight only test",
                "--session-token",
                token_path,
                "--guard-minutes",
                "0",
                "--dryrun-log",
                dryrun_log,
                "--dryrun-block-out",
                block_out,
                "--timer-guard-out",
                guard_out,
                "--timer-end-out",
                end_out,
                "--entry-out",
                entry_out,
                "--team-status",
                team_status,
                "--check-compliance-policy",
                "pass_section_freeze_timer_safe",
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("team_status_append=skipped", proc.stdout)
        self.assertIn("preflight_mode=enabled", proc.stdout)
        self.assertIn("preflight_result=pass", proc.stdout)
        self.assertIn("compliance_check=pass", proc.stdout)
        self.assertEqual(Path(team_status).read_text(encoding="utf-8"), original)

    def test_collect_with_submission_readiness_option(self) -> None:
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

        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            team_status = fp.name
            fp.write(self.sample_team_status())
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            dryrun_log = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            block_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            guard_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            end_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            entry_out = fp.name

        env = os.environ.copy()
        env["C_TEAM_SKIP_STAGING_BUNDLE"] = "1"
        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-26 collect submission readiness test",
                "--session-token",
                token_path,
                "--guard-minutes",
                "0",
                "--dryrun-log",
                dryrun_log,
                "--dryrun-block-out",
                block_out,
                "--timer-guard-out",
                guard_out,
                "--timer-end-out",
                end_out,
                "--entry-out",
                entry_out,
                "--team-status",
                team_status,
                "--append-to-team-status",
                "--check-compliance-policy",
                "pass_section_freeze_timer_safe",
                "--check-submission-readiness-minutes",
                "0",
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
            env=env,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("submission_readiness=pass", proc.stdout)
        self.assertIn("preflight_mode=enabled", proc.stdout)
        self.assertIn("preflight_result=pass", proc.stdout)
        entry_text = Path(entry_out).read_text(encoding="utf-8")
        self.assertIn("提出前ゲート実行（strict-safe + elapsed監査）", entry_text)
        self.assertIn("PASS（strict-safe + submission readiness）", entry_text)
        self.assertNotIn("<記入>", entry_text)
        self.assertIn(
            "bash scripts/check_c_team_dryrun_compliance.sh",
            entry_text,
        )
        self.assertIn(
            "bash scripts/check_c_team_submission_readiness.sh",
            entry_text,
        )
        self.assertIn(
            f"missing_log_review_command=rg -n 'collect_preflight_log_resolved|collect_preflight_log_missing|collect_preflight_check_reason|submission_readiness_retry_command|review_command_fail_reason_codes_source' {entry_out}",
            entry_text,
        )

    def test_compliance_failure_does_not_mutate_team_status(self) -> None:
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

        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            team_status = fp.name
            fp.write(self.sample_team_status())
        original = Path(team_status).read_text(encoding="utf-8")
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            dryrun_log = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            block_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            guard_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            end_out = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            entry_out = fp.name

        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-27 collect invalid placeholder test",
                "--session-token",
                token_path,
                "--guard-minutes",
                "0",
                "--dryrun-log",
                dryrun_log,
                "--dryrun-block-out",
                block_out,
                "--timer-guard-out",
                guard_out,
                "--timer-end-out",
                end_out,
                "--entry-out",
                entry_out,
                "--team-status",
                team_status,
                "--append-to-team-status",
                "--check-compliance-policy",
                "pass_section_freeze_timer_safe",
                "--pass-fail-line",
                "<PASS|FAIL>",
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("template_placeholder_detected", proc.stderr + proc.stdout)
        updated = Path(team_status).read_text(encoding="utf-8")
        self.assertEqual(updated, original)

    def test_rejects_invalid_collect_latest_require_found_option(self) -> None:
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

        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-32 invalid strict option",
                "--session-token",
                token_path,
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
        end_timer_if_present(token_path)

    def test_collect_strict_latest_requires_resolved_log(self) -> None:
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

        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            team_status = fp.name
            fp.write(self.sample_team_status())
        original = Path(team_status).read_text(encoding="utf-8")
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            entry_out = fp.name

        env = os.environ.copy()
        env["C_TEAM_SKIP_STAGING_BUNDLE"] = "1"
        env.pop("C_REQUIRE_REVIEW_COMMANDS", None)
        env["C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY"] = "1"
        env["C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV"] = "1"
        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-32 strict latest fail test",
                "--session-token",
                token_path,
                "--guard-minutes",
                "0",
                "--team-status",
                team_status,
                "--append-to-team-status",
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
        self.assertIn("collect_preflight_check=fail", combined)
        self.assertIn("submission_readiness_retry_command=", combined)
        self.assertIn("C_COLLECT_LATEST_REQUIRE_FOUND=1", combined)
        self.assertIn(
            f"bash scripts/check_c_team_submission_readiness.sh {team_status} 0",
            combined,
        )
        self.assertEqual(Path(team_status).read_text(encoding="utf-8"), original)
        entry_text = Path(entry_out).read_text(encoding="utf-8")
        self.assertIn("preflight_latest_require_found=1 (enabled)", entry_text)
        self.assertIn("C_COLLECT_LATEST_REQUIRE_FOUND=1", entry_text)
        self.assertIn("collect_preflight_check_reason=latest_not_found_strict", entry_text)
        self.assertIn(
            "collect_preflight_reasons=collect_preflight_check_reason=latest_not_found_strict",
            entry_text,
        )
        self.assertIn(
            f"review_command_audit_command=python scripts/check_c_team_review_commands.py --team-status {team_status}",
            entry_text,
        )
        self.assertIn("review_command_required=0", entry_text)
        self.assertIn("review_command_fail_reason_codes_source=-", entry_text)
        self.assertIn(
            f"missing_log_review_command=rg -n 'collect_preflight_log_resolved|collect_preflight_log_missing|collect_preflight_check_reason|submission_readiness_retry_command|review_command_fail_reason_codes_source' {entry_out}",
            entry_text,
        )

    def test_collect_strict_latest_with_review_required_keeps_retry_prefix(self) -> None:
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

        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            team_status = fp.name
            fp.write(self.sample_team_status())
        original = Path(team_status).read_text(encoding="utf-8")
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            entry_out = fp.name

        env = os.environ.copy()
        env["C_TEAM_SKIP_STAGING_BUNDLE"] = "1"
        env["C_REQUIRE_REVIEW_COMMANDS"] = "1"
        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-43 strict+review retry prefix test",
                "--session-token",
                token_path,
                "--guard-minutes",
                "0",
                "--team-status",
                team_status,
                "--append-to-team-status",
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
        self.assertIn("collect_preflight_check_reason=latest_not_found_strict", combined)
        self.assertIn(
            "submission_readiness_retry_command=",
            combined,
        )
        self.assertIn(
            "C_COLLECT_LATEST_REQUIRE_FOUND=1",
            combined,
        )
        self.assertIn(
            "C_REQUIRE_REVIEW_COMMANDS=1",
            combined,
        )
        self.assertIn(
            f"bash scripts/check_c_team_submission_readiness.sh {team_status} 0",
            combined,
        )
        self.assertEqual(Path(team_status).read_text(encoding="utf-8"), original)
        entry_text = Path(entry_out).read_text(encoding="utf-8")
        self.assertIn(
            "submission_readiness_retry_command=",
            entry_text,
        )
        self.assertIn(
            "C_COLLECT_LATEST_REQUIRE_FOUND=1",
            entry_text,
        )
        self.assertIn(
            "C_REQUIRE_REVIEW_COMMANDS=1",
            entry_text,
        )
        self.assertIn(
            f"bash scripts/check_c_team_submission_readiness.sh {team_status} 0",
            entry_text,
        )
        self.assertIn(
            "collect_preflight_reasons=collect_preflight_check_reason=latest_not_found_strict",
            entry_text,
        )

    def test_collect_strict_latest_uses_c_fail_trace_env_when_c_require_env_empty(self) -> None:
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

        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            team_status = fp.name
            fp.write(self.sample_team_status())
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            entry_out = fp.name

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
                "--task-title",
                "C-58 strict latest env fallback test",
                "--session-token",
                token_path,
                "--guard-minutes",
                "0",
                "--team-status",
                team_status,
                "--append-to-team-status",
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
        self.assertIn("C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY=1", combined)
        self.assertIn("C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY=1", combined)
        self.assertIn("C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV=1", combined)

    def test_collect_review_required_fail_fast_emits_review_reason_codes(self) -> None:
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

        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            team_status = fp.name
            fp.write(self.sample_team_status())
        original = Path(team_status).read_text(encoding="utf-8")
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            entry_out = fp.name

        env = os.environ.copy()
        env["C_TEAM_SKIP_STAGING_BUNDLE"] = "1"
        env["C_REQUIRE_REVIEW_COMMANDS"] = "1"
        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-58 collect review required fail-fast test",
                "--session-token",
                token_path,
                "--guard-minutes",
                "0",
                "--team-status",
                team_status,
                "--append-to-team-status",
                "--check-compliance-policy",
                "pass_section_freeze_timer_safe",
                "--check-submission-readiness-minutes",
                "0",
                "--entry-out",
                entry_out,
                "--command-line",
                "python scripts/check_c_team_collect_preflight_report.py /tmp/c58_collect.log --require-enabled -> PASS",
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
        self.assertIn("submission_readiness_fail_step=review_command", combined)
        self.assertIn(
            "review_command_retry_command=python scripts/check_c_team_review_commands.py --team-status ",
            combined,
        )
        self.assertIn("C_REQUIRE_REVIEW_COMMANDS=1", combined)
        self.assertEqual(Path(team_status).read_text(encoding="utf-8"), original)

    def test_collect_records_explicit_missing_log_context_in_entry(self) -> None:
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

        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            team_status = fp.name
            fp.write(self.sample_team_status())
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
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            explicit_missing_log = fp.name
        Path(explicit_missing_log).unlink(missing_ok=True)

        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-38 explicit missing log context test",
                "--session-token",
                token_path,
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
                "--team-status",
                team_status,
                "--append-to-team-status",
                "--check-compliance-policy",
                "pass_section_freeze_timer_safe",
                "--collect-preflight-log",
                explicit_missing_log,
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        entry_text = Path(entry_out).read_text(encoding="utf-8")
        self.assertIn(f"collect_preflight_log_missing={explicit_missing_log}", entry_text)
        self.assertIn("collect_preflight_check_reason=explicit_log_missing", entry_text)
        self.assertIn(
            "collect_preflight_reasons=collect_preflight_check_reason=explicit_log_missing",
            entry_text,
        )

    def test_collect_strict_latest_fails_when_resolved_log_file_is_missing(self) -> None:
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

        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            team_status = fp.name
            fp.write(
                "## Cチーム\n"
                "- 実行タスク: C-37 seed latest missing\n"
                "  - 実行コマンド / pass-fail:\n"
                f"    - python scripts/check_c_team_collect_preflight_report.py {missing_log} --require-enabled -> PASS\n"
                "  - pass/fail:\n"
                "    - PASS\n\n"
                "## PMチーム\n"
                "- 実行タスク: PM-3\n"
            )
        original = Path(team_status).read_text(encoding="utf-8")
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
        env["C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY"] = "1"
        env["C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV"] = "1"
        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-37 strict latest missing resolved log test",
                "--session-token",
                token_path,
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
                "--team-status",
                team_status,
                "--append-to-team-status",
                "--check-compliance-policy",
                "pass_section_freeze_timer_safe",
                "--check-submission-readiness-minutes",
                "0",
                "--collect-latest-require-found",
                "1",
                "--entry-out",
                entry_out,
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
        self.assertIn("submission_readiness_retry_command=", combined)
        self.assertIn("C_COLLECT_LATEST_REQUIRE_FOUND=1", combined)
        self.assertIn(
            f"bash scripts/check_c_team_submission_readiness.sh {team_status} 0",
            combined,
        )
        self.assertEqual(Path(team_status).read_text(encoding="utf-8"), original)
        entry_text = Path(entry_out).read_text(encoding="utf-8")
        self.assertIn(f"collect_preflight_log_resolved={missing_log}", entry_text)
        self.assertIn(f"collect_preflight_log_missing={missing_log}", entry_text)
        self.assertIn(
            "collect_preflight_check_reason=latest_resolved_log_missing_strict",
            entry_text,
        )
        self.assertIn(
            "collect_preflight_reasons=collect_preflight_check_reason=latest_resolved_log_missing_strict",
            entry_text,
        )
        self.assertIn(
            f"missing_log_review_command=rg -n 'collect_preflight_log_resolved|collect_preflight_log_missing|collect_preflight_check_reason|submission_readiness_retry_command|review_command_fail_reason_codes_source' {entry_out}",
            entry_text,
        )

    def test_collect_records_latest_resolved_missing_default_context_in_entry(self) -> None:
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

        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            team_status = fp.name
            fp.write(
                "## Cチーム\n"
                "- 実行タスク: C-37 seed latest missing default\n"
                "  - 実行コマンド / pass-fail:\n"
                f"    - python scripts/check_c_team_collect_preflight_report.py {missing_log} --require-enabled -> PASS\n"
                "  - pass/fail:\n"
                "    - PASS\n\n"
                "## PMチーム\n"
                "- 実行タスク: PM-3\n"
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

        proc = subprocess.run(
            [
                "bash",
                str(SCRIPT_PATH),
                "--task-title",
                "C-38 latest resolved missing default context test",
                "--session-token",
                token_path,
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
                "--team-status",
                team_status,
                "--append-to-team-status",
                "--check-compliance-policy",
                "pass_section_freeze_timer_safe",
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        entry_text = Path(entry_out).read_text(encoding="utf-8")
        self.assertIn(f"collect_preflight_log_resolved={missing_log}", entry_text)
        self.assertIn(f"collect_preflight_log_missing={missing_log}", entry_text)
        self.assertIn(
            "collect_preflight_check_reason=latest_resolved_log_missing_default_skip",
            entry_text,
        )
        self.assertIn(
            "collect_preflight_reasons=collect_preflight_check_reason=latest_resolved_log_missing_default_skip",
            entry_text,
        )


if __name__ == "__main__":
    unittest.main()
