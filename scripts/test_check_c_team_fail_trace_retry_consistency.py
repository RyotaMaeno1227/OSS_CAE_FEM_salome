#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "check_c_team_fail_trace_retry_consistency.py"


def write_status(markdown: str) -> str:
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
        fp.write(markdown)
        return fp.name


class CheckCTeamFailTraceRetryConsistencyTest(unittest.TestCase):
    def run_script(self, markdown: str, *args: str) -> subprocess.CompletedProcess[str]:
        status_path = write_status(markdown)
        return subprocess.run(
            ["python", str(SCRIPT_PATH), "--team-status", status_path, *args],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

    def test_pass_with_consistent_retry_commands(self) -> None:
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-50
              - start_epoch=200
              - fail_trace_audit_log=/tmp/c50_retry.log
              - fail_trace_retry_consistency_check=pass
              - fail_trace_audit_retry_command=scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 30 | tee /tmp/c50_retry.log
              - fail_trace_finalize_retry_command=bash scripts/recover_c_team_token_missing_session.sh --team-status docs/team_status.md --finalize-session-token /tmp/c_team_session.token --task-title "C-50" --guard-minutes 30 --check-submission-readiness-minutes 30 --collect-latest-require-found 1 --fail-trace-audit-log /tmp/c50_retry.log
            """
        )
        proc = self.run_script(markdown)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("verdict=PASS", proc.stdout)
        self.assertIn("reason_codes=-", proc.stdout)

    def test_fail_when_finalize_retry_missing(self) -> None:
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-50
              - start_epoch=200
              - fail_trace_audit_retry_command=scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 30 | tee /tmp/c50_retry.log
              - fail_trace_audit_retry_reason=audit_result_FAIL
            """
        )
        proc = self.run_script(markdown)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("missing fail_trace_finalize_retry_command", proc.stdout)
        self.assertIn("reason_codes=missing_fail_trace_finalize_retry_command", proc.stdout)

    def test_fail_when_retry_log_mismatch(self) -> None:
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-50
              - start_epoch=200
              - fail_trace_audit_log=/tmp/c50_retry.log
              - fail_trace_audit_retry_command=scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 30 | tee /tmp/c50_retry.log
              - fail_trace_finalize_retry_command=bash scripts/recover_c_team_token_missing_session.sh --team-status docs/team_status.md --finalize-session-token /tmp/c_team_session.token --task-title "C-50" --guard-minutes 30 --check-submission-readiness-minutes 30 --fail-trace-audit-log /tmp/c50_retry_other.log
            """
        )
        proc = self.run_script(markdown)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("log mismatch", proc.stdout)

    def test_pass_when_requirement_disabled(self) -> None:
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-50
              - start_epoch=200
              - fail_trace_audit_retry_command=scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 30 | tee /tmp/c50_retry.log
              - fail_trace_audit_retry_reason=audit_result_FAIL
            """
        )
        proc = self.run_script(markdown, "--no-require-finalize-retry-when-audit-retry")
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("verdict=PASS", proc.stdout)

    def test_pass_when_audit_retry_exists_without_issue_marker(self) -> None:
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-50
              - start_epoch=200
              - fail_trace_audit_result=PASS
              - fail_trace_audit_retry_command=scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 30 | tee /tmp/c50_retry.log
            """
        )
        proc = self.run_script(markdown)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("has_audit_issue=no", proc.stdout)

    def test_fail_when_retry_consistency_key_required_but_missing(self) -> None:
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-51
              - start_epoch=300
              - fail_trace_audit_result=PASS
              - fail_trace_audit_retry_command=scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 45 | tee /tmp/c51_retry.log
            """
        )
        proc = self.run_script(markdown, "--require-retry-consistency-check-key")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("missing fail_trace_retry_consistency_check", proc.stdout)

    def test_fail_when_retry_consistency_check_is_fail(self) -> None:
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-51
              - start_epoch=300
              - fail_trace_retry_consistency_check=fail
              - fail_trace_audit_result=PASS
              - fail_trace_audit_retry_command=scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 45 | tee /tmp/c51_retry.log
            """
        )
        proc = self.run_script(markdown)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("fail_trace_retry_consistency_check=fail", proc.stdout)

    def test_pass_when_strict_env_prefix_matches_entry_keys(self) -> None:
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-53
              - start_epoch=400
              - fail_trace_retry_consistency_required=1
              - fail_trace_retry_consistency_require_key=1
              - fail_trace_audit_log=/tmp/c53_retry.log
              - fail_trace_audit_retry_reason=audit_result_FAIL
              - fail_trace_audit_retry_command=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 45 | tee /tmp/c53_retry.log
              - fail_trace_finalize_retry_command=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 bash scripts/recover_c_team_token_missing_session.sh --team-status docs/team_status.md --finalize-session-token /tmp/c_team_session.token --task-title "C-53" --guard-minutes 30 --check-submission-readiness-minutes 45 --fail-trace-audit-log /tmp/c53_retry.log
            """
        )
        proc = self.run_script(markdown, "--require-strict-env-prefix-match")
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("verdict=PASS", proc.stdout)

    def test_fail_when_strict_env_prefix_missing_from_finalize_retry(self) -> None:
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-53
              - start_epoch=400
              - fail_trace_retry_consistency_required=1
              - fail_trace_retry_consistency_require_key=1
              - fail_trace_audit_log=/tmp/c53_retry.log
              - fail_trace_audit_retry_reason=audit_result_FAIL
              - fail_trace_audit_retry_command=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 45 | tee /tmp/c53_retry.log
              - fail_trace_finalize_retry_command=bash scripts/recover_c_team_token_missing_session.sh --team-status docs/team_status.md --finalize-session-token /tmp/c_team_session.token --task-title "C-53" --guard-minutes 30 --check-submission-readiness-minutes 45 --fail-trace-audit-log /tmp/c53_retry.log
            """
        )
        proc = self.run_script(markdown, "--require-strict-env-prefix-match")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn(
            "missing C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY in fail_trace_finalize_retry_command",
            proc.stdout,
        )
        self.assertIn(
            "missing_c_fail_trace_require_retry_consistency_in_fail_trace_finalize_retry_command",
            proc.stdout,
        )

    def test_pass_when_strict_env_key_matches_commands(self) -> None:
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-53
              - start_epoch=410
              - fail_trace_retry_consistency_required=1
              - fail_trace_retry_consistency_require_key=1
              - fail_trace_retry_consistency_require_strict_env=1
              - fail_trace_audit_log=/tmp/c53_retry.log
              - fail_trace_audit_retry_reason=audit_result_FAIL
              - fail_trace_audit_retry_command=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=1 scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 45 | tee /tmp/c53_retry.log
              - fail_trace_finalize_retry_command=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=1 bash scripts/recover_c_team_token_missing_session.sh --team-status docs/team_status.md --finalize-session-token /tmp/c_team_session.token --task-title "C-53" --guard-minutes 30 --check-submission-readiness-minutes 45 --fail-trace-audit-log /tmp/c53_retry.log
            """
        )
        proc = self.run_script(markdown, "--require-strict-env-prefix-match")
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("verdict=PASS", proc.stdout)

    def test_fail_when_strict_env_key_is_required_but_missing_from_command(self) -> None:
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-53
              - start_epoch=410
              - fail_trace_retry_consistency_required=1
              - fail_trace_retry_consistency_require_key=1
              - fail_trace_retry_consistency_require_strict_env=1
              - fail_trace_audit_log=/tmp/c53_retry.log
              - fail_trace_audit_retry_reason=audit_result_FAIL
              - fail_trace_audit_retry_command=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 45 | tee /tmp/c53_retry.log
              - fail_trace_finalize_retry_command=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 bash scripts/recover_c_team_token_missing_session.sh --team-status docs/team_status.md --finalize-session-token /tmp/c_team_session.token --task-title "C-53" --guard-minutes 30 --check-submission-readiness-minutes 45 --fail-trace-audit-log /tmp/c53_retry.log
            """
        )
        proc = self.run_script(markdown, "--require-strict-env-prefix-match")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn(
            "missing C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV in fail_trace_audit_retry_command",
            proc.stdout,
        )

    def test_pass_when_strict_env_requirements_match_audit_only_retry(self) -> None:
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-53
              - start_epoch=420
              - fail_trace_retry_consistency_required=1
              - fail_trace_retry_consistency_require_key=1
              - fail_trace_retry_consistency_require_strict_env=1
              - fail_trace_audit_result=PASS
              - fail_trace_audit_retry_command=C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY=1 C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV=1 scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 45 | tee /tmp/c53_retry.log
            """
        )
        proc = self.run_script(markdown, "--require-strict-env-prefix-match")
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("verdict=PASS", proc.stdout)

    def test_fail_when_strict_env_requirements_miss_audit_only_retry(self) -> None:
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-53
              - start_epoch=420
              - fail_trace_retry_consistency_required=1
              - fail_trace_retry_consistency_require_key=1
              - fail_trace_retry_consistency_require_strict_env=1
              - fail_trace_audit_result=PASS
              - fail_trace_audit_retry_command=scripts/run_c_team_fail_trace_audit.sh docs/team_status.md 45 | tee /tmp/c53_retry.log
            """
        )
        proc = self.run_script(markdown, "--require-strict-env-prefix-match")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn(
            "missing C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY in fail_trace_audit_retry_command",
            proc.stdout,
        )


if __name__ == "__main__":
    unittest.main()
