#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "check_c_team_fail_trace_order.py"


def write_log(text: str) -> str:
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
        fp.write(text)
        return fp.name


class CheckCTeamFailTraceOrderTest(unittest.TestCase):
    def run_script(self, log_text: str, mode: str) -> subprocess.CompletedProcess[str]:
        log_path = write_log(log_text)
        return subprocess.run(
            ["python", str(SCRIPT_PATH), log_path, "--mode", mode],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

    def test_strict_passes_with_expected_order(self):
        proc = self.run_script(
            "\n".join(
                [
                    "collect_preflight_check_reason=latest_resolved_log_missing_strict",
                    "collect_preflight_check=fail",
                    "submission_readiness_collect_preflight_check=fail",
                    "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_strict",
                    "submission_readiness_retry_command=C_COLLECT_LATEST_REQUIRE_FOUND=1 ...",
                    "submission_readiness_fail_step=collect_preflight",
                ]
            ),
            mode="strict",
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("verdict=PASS", proc.stdout)

    def test_strict_fails_when_order_is_invalid(self):
        proc = self.run_script(
            "\n".join(
                [
                    "submission_readiness_collect_preflight_check=fail",
                    "collect_preflight_check_reason=latest_resolved_log_missing_strict",
                    "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_strict",
                    "submission_readiness_retry_command=C_COLLECT_LATEST_REQUIRE_FOUND=1 ...",
                    "submission_readiness_fail_step=collect_preflight",
                    "collect_preflight_check=fail",
                ]
            ),
            mode="strict",
        )
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("invalid_order(strict)", proc.stdout)

    def test_strict_passes_when_review_trace_is_outside_fail_trace_block(self):
        proc = self.run_script(
            "\n".join(
                [
                    "C_TEAM_REVIEW_COMMAND_AUDIT",
                    "review_command_check=pass",
                    "collect_preflight_check_reason=latest_resolved_log_missing_strict",
                    "collect_preflight_check=fail",
                    "submission_readiness_collect_preflight_check=fail",
                    "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_strict",
                    "submission_readiness_retry_command=C_COLLECT_LATEST_REQUIRE_FOUND=1 ...",
                    "submission_readiness_fail_step=collect_preflight",
                ]
            ),
            mode="strict",
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("verdict=PASS", proc.stdout)

    def test_strict_fails_when_review_trace_is_interleaved(self):
        proc = self.run_script(
            "\n".join(
                [
                    "collect_preflight_check_reason=latest_resolved_log_missing_strict",
                    "collect_preflight_check=fail",
                    "submission_readiness_collect_preflight_check=fail",
                    "review_command_check=pass",
                    "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_strict",
                    "submission_readiness_retry_command=C_COLLECT_LATEST_REQUIRE_FOUND=1 ...",
                    "submission_readiness_fail_step=collect_preflight",
                ]
            ),
            mode="strict",
        )
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("invalid_review_trace_boundary", proc.stdout)

    def test_default_passes_without_retry_and_fail_step(self):
        proc = self.run_script(
            "\n".join(
                [
                    "collect_preflight_check_reason=latest_resolved_log_missing_default_skip",
                    "collect_preflight_check=skipped",
                    "submission_readiness_collect_preflight_check=skipped",
                    "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_default_skip",
                ]
            ),
            mode="default",
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("verdict=PASS", proc.stdout)

    def test_default_fails_if_review_trace_is_interleaved(self):
        proc = self.run_script(
            "\n".join(
                [
                    "collect_preflight_check_reason=latest_resolved_log_missing_default_skip",
                    "collect_preflight_check=skipped",
                    "C_TEAM_REVIEW_COMMAND_AUDIT",
                    "review_command_check=pass",
                    "submission_readiness_collect_preflight_check=skipped",
                    "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_default_skip",
                ]
            ),
            mode="default",
        )
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("invalid_review_trace_boundary", proc.stdout)

    def test_default_fails_if_retry_or_fail_step_exists(self):
        proc = self.run_script(
            "\n".join(
                [
                    "collect_preflight_check_reason=latest_resolved_log_missing_default_skip",
                    "collect_preflight_check=skipped",
                    "submission_readiness_collect_preflight_check=skipped",
                    "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_default_skip",
                    "submission_readiness_retry_command=should_not_exist",
                    "submission_readiness_fail_step=collect_preflight",
                ]
            ),
            mode="default",
        )
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("unexpected retry in default skip", proc.stdout)
        self.assertIn("unexpected fail_step in default skip", proc.stdout)


if __name__ == "__main__":
    unittest.main()
