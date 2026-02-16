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
                "/tmp/c_collect_test.log",
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
            "python scripts/check_c_team_collect_preflight_report.py /tmp/c_collect_test.log --require-enabled -> PASS",
            Path(entry_out).read_text(encoding="utf-8"),
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
        self.assertEqual(Path(team_status).read_text(encoding="utf-8"), original)
        entry_text = Path(entry_out).read_text(encoding="utf-8")
        self.assertIn("preflight_latest_require_found=1 (enabled)", entry_text)
        self.assertIn("C_COLLECT_LATEST_REQUIRE_FOUND=1", entry_text)
        self.assertIn("collect_preflight_check_reason=latest_not_found_strict", entry_text)


if __name__ == "__main__":
    unittest.main()
