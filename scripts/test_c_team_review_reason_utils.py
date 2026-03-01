#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
UTIL_PATH = REPO_ROOT / "scripts" / "c_team_review_reason_utils.sh"


def run_bash(snippet: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [
            "bash",
            "-lc",
            f"source {UTIL_PATH}; {snippet}",
        ],
        cwd=REPO_ROOT,
        text=True,
        capture_output=True,
        check=False,
    )


class CTeamReviewReasonUtilsTest(unittest.TestCase):
    def test_build_prefixed_reason_codes_from_semicolon_list(self) -> None:
        proc = run_bash(
            "c_team_build_prefixed_reason_codes review_command_ "
            "'missing_missing_log_review_command; missing_collect_report_review_command' "
            "'unused'"
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertEqual(
            proc.stdout.strip(),
            "review_command_missing_missing_log_review_command,review_command_missing_collect_report_review_command",
        )

    def test_build_prefixed_reason_codes_from_comma_list(self) -> None:
        proc = run_bash(
            "c_team_build_prefixed_reason_codes review_command_ "
            "'missing_missing_log_review_command,missing_collect_report_review_command' "
            "'unused'"
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertEqual(
            proc.stdout.strip(),
            "review_command_missing_missing_log_review_command,review_command_missing_collect_report_review_command",
        )

    def test_build_prefixed_reason_codes_falls_back_when_codes_absent(self) -> None:
        proc = run_bash(
            "c_team_build_prefixed_reason_codes review_command_ "
            "'' "
            "'missing collect_report_review_command'"
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertEqual(
            proc.stdout.strip(),
            "review_command_missing_collect_report_review_command",
        )

    def test_normalize_reason_code(self) -> None:
        proc = run_bash("c_team_normalize_reason_code 'Missing Collect Report!!!'")
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertEqual(proc.stdout.strip(), "missing_collect_report")

    def test_collect_missing_log_review_pattern_contract(self) -> None:
        proc = run_bash("c_team_collect_missing_log_review_pattern")
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertEqual(
            proc.stdout.strip(),
            "collect_preflight_log_resolved|collect_preflight_log_missing|collect_preflight_check_reason|submission_readiness_retry_command|review_command_fail_reason_codes_source",
        )

    def test_resolve_binary_toggle_prefers_primary(self) -> None:
        proc = run_bash(
            "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY=1 "
            "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=0 "
            "c_team_resolve_binary_toggle "
            "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY "
            "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY 9"
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertEqual(proc.stdout.strip(), "1")

    def test_resolve_binary_toggle_uses_fallback_when_primary_empty(self) -> None:
        proc = run_bash(
            "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY='' "
            "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=0 "
            "c_team_resolve_binary_toggle "
            "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY "
            "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY 9"
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertEqual(proc.stdout.strip(), "0")

    def test_resolve_binary_toggle_uses_default_when_both_unset(self) -> None:
        proc = run_bash(
            "unset C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY "
            "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=''; "
            "c_team_resolve_binary_toggle "
            "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY "
            "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY 9"
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertEqual(proc.stdout.strip(), "9")


if __name__ == "__main__":
    unittest.main()
