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

    def test_recover_marks_entry_and_starts_new_session(self) -> None:
        status_path = write_temp(self.sample_team_status(), ".md")
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
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("[1/2] mark token-missing entry", proc.stdout)
        self.assertIn("[2/2] start new session timer", proc.stdout)
        self.assertIn("next_finalize_command=", proc.stdout)
        self.assertIn("next_finalize_command_strict_latest=", proc.stdout)
        updated = Path(status_path).read_text(encoding="utf-8")
        self.assertNotIn("<pending>", updated)
        self.assertIn("token missing", updated)
        new_token = parse_session_token(proc.stdout)
        self.assertTrue(new_token)
        self.assertTrue(Path(new_token).exists())
        end_timer_if_present(new_token)

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
        updated = Path(status_path).read_text(encoding="utf-8")
        self.assertIn("C-25 finalize test", updated)
        self.assertIn("C-25 finalize mode done", updated)
        self.assertIn("safe_stage_command=git add", updated)

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
                "--check-compliance-policy",
                "pass_section_freeze_timer_safe",
                "--check-submission-readiness-minutes",
                "0",
                "--collect-latest-require-found",
                "1",
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


if __name__ == "__main__":
    unittest.main()
