#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "run_c_team_fail_trace_audit.sh"
ORDER_CHECKER = REPO_ROOT / "scripts" / "check_c_team_fail_trace_order.py"


def make_fake_script(body: str) -> str:
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".sh", delete=False) as fp:
        fp.write("#!/usr/bin/env bash\nset -euo pipefail\n")
        fp.write(body)
    path = fp.name
    os.chmod(path, 0o755)
    return path


def make_fake_python_script(body: str) -> str:
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".py", delete=False) as fp:
        fp.write(body)
    return fp.name


class RunCTeamFailTraceAuditTest(unittest.TestCase):
    def run_script(
        self,
        readiness_body: str,
        staging_body: str,
        retry_consistency_body: str | None = None,
        require_retry_consistency: str = "0",
        require_retry_consistency_key: str = "0",
        require_retry_consistency_strict_env: str = "0",
        extra_env: dict[str, str] | None = None,
    ) -> subprocess.CompletedProcess[str]:
        readiness = make_fake_script(
            textwrap.dedent(
                f"""\
                if [[ "${{C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY:-}}" != "{require_retry_consistency}" ]]; then
                  echo "missing C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY={require_retry_consistency}" >&2
                  exit 8
                fi
                if [[ "${{C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY:-}}" != "{require_retry_consistency_key}" ]]; then
                  echo "missing C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY={require_retry_consistency_key}" >&2
                  exit 9
                fi
                if [[ "${{C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV:-}}" != "{require_retry_consistency_strict_env}" ]]; then
                  echo "missing C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV={require_retry_consistency_strict_env}" >&2
                  exit 10
                fi
                """
            )
            + readiness_body
        )
        staging = make_fake_script(
            textwrap.dedent(
                f"""\
                if [[ "${{C_SKIP_NESTED_SELFTESTS:-}}" != "1" ]]; then
                  echo "missing C_SKIP_NESTED_SELFTESTS=1" >&2
                  exit 7
                fi
                if [[ "${{C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY:-}}" != "{require_retry_consistency}" ]]; then
                  echo "missing C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY={require_retry_consistency}" >&2
                  exit 8
                fi
                if [[ "${{C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY:-}}" != "{require_retry_consistency_key}" ]]; then
                  echo "missing C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY={require_retry_consistency_key}" >&2
                  exit 9
                fi
                if [[ "${{C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV:-}}" != "{require_retry_consistency_strict_env}" ]]; then
                  echo "missing C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV={require_retry_consistency_strict_env}" >&2
                  exit 10
                fi
                """
            )
            + staging_body
        )
        env = os.environ.copy()
        env["C_FAIL_TRACE_READINESS_SCRIPT"] = readiness
        env["C_FAIL_TRACE_STAGING_SCRIPT"] = staging
        env["C_FAIL_TRACE_ORDER_CHECKER_SCRIPT"] = str(ORDER_CHECKER)
        env["C_FAIL_TRACE_REQUIRE_REVIEW"] = "1"
        env["C_FAIL_TRACE_SKIP_NESTED_SELFTESTS"] = "1"
        env["C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY"] = require_retry_consistency
        env["C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY"] = require_retry_consistency_key
        env["C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV"] = require_retry_consistency_strict_env
        if extra_env:
            env.update(extra_env)
        if retry_consistency_body is not None:
            env["C_FAIL_TRACE_RETRY_CONSISTENCY_SCRIPT"] = make_fake_python_script(
                retry_consistency_body
            )
        return subprocess.run(
            ["bash", str(SCRIPT_PATH), "/tmp/unused_team_status.md", "30"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
            env=env,
        )

    def test_passes_with_expected_default_and_strict_contract(self):
        body = textwrap.dedent(
            """\
            if [[ "${C_COLLECT_LATEST_REQUIRE_FOUND:-0}" == "1" ]]; then
              echo "collect_preflight_check_reason=latest_resolved_log_missing_strict"
              echo "collect_preflight_check=fail"
              echo "submission_readiness_collect_preflight_check=fail"
              echo "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_strict"
              echo "submission_readiness_retry_command=retry_cmd"
              echo "submission_readiness_fail_step=collect_preflight"
              exit 1
            fi
            echo "collect_preflight_check_reason=latest_resolved_log_missing_default_skip"
            echo "collect_preflight_check=skipped"
            echo "submission_readiness_collect_preflight_check=skipped"
            echo "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_default_skip"
            """
        )
        proc = self.run_script(body, body)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("FAIL_TRACE_AUDIT_RESULT=PASS", proc.stdout)
        self.assertIn("fail_trace_require_retry_consistency=0", proc.stdout)
        self.assertIn("fail_trace_require_retry_consistency_key=0", proc.stdout)
        self.assertIn(
            "fail_trace_retry_consistency_retry_command=python scripts/check_c_team_fail_trace_retry_consistency.py --team-status /tmp/unused_team_status.md",
            proc.stdout,
        )
        self.assertIn("fail_trace_retry_consistency_reasons=-", proc.stdout)
        self.assertIn("fail_trace_retry_consistency_reason_codes=-", proc.stdout)
        self.assertIn("fail_trace_retry_consistency_check=skipped", proc.stdout)

    def test_fails_if_strict_path_unexpectedly_succeeds(self):
        bad_body = textwrap.dedent(
            """\
            echo "collect_preflight_check_reason=latest_resolved_log_missing_default_skip"
            echo "collect_preflight_check=skipped"
            echo "submission_readiness_collect_preflight_check=skipped"
            echo "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_default_skip"
            exit 0
            """
        )
        proc = self.run_script(bad_body, bad_body)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("strict readiness unexpectedly succeeded", proc.stderr)
        self.assertIn("readiness_strict_log=", proc.stderr)

    def test_fails_with_context_when_readiness_default_capture_fails(self):
        readiness_body = textwrap.dedent(
            """\
            if [[ "${C_COLLECT_LATEST_REQUIRE_FOUND:-0}" == "1" ]]; then
              echo "collect_preflight_check_reason=latest_resolved_log_missing_strict"
              echo "collect_preflight_check=fail"
              echo "submission_readiness_collect_preflight_check=fail"
              echo "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_strict"
              echo "submission_readiness_retry_command=retry_cmd"
              echo "submission_readiness_fail_step=collect_preflight"
              exit 1
            fi
            echo "default_readiness_failure_marker"
            exit 9
            """
        )
        passing_staging_body = textwrap.dedent(
            """\
            if [[ "${C_COLLECT_LATEST_REQUIRE_FOUND:-0}" == "1" ]]; then
              echo "collect_preflight_check_reason=latest_resolved_log_missing_strict"
              echo "collect_preflight_check=fail"
              echo "submission_readiness_collect_preflight_check=fail"
              echo "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_strict"
              echo "submission_readiness_retry_command=retry_cmd"
              echo "submission_readiness_fail_step=collect_preflight"
              exit 1
            fi
            echo "collect_preflight_check_reason=latest_resolved_log_missing_default_skip"
            echo "collect_preflight_check=skipped"
            echo "submission_readiness_collect_preflight_check=skipped"
            echo "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_default_skip"
            """
        )
        proc = self.run_script(readiness_body, passing_staging_body)
        self.assertEqual(proc.returncode, 9)
        self.assertIn("readiness default capture failed: rc=9", proc.stderr)
        self.assertIn("default_readiness_failure_marker", proc.stderr)
        self.assertIn("readiness_default_log=", proc.stderr)
        self.assertIn("FAIL_TRACE_AUDIT_RESULT=FAIL", proc.stderr)

    def test_runs_retry_consistency_checker_when_required(self):
        body = textwrap.dedent(
            """\
            if [[ "${C_COLLECT_LATEST_REQUIRE_FOUND:-0}" == "1" ]]; then
              echo "collect_preflight_check_reason=latest_resolved_log_missing_strict"
              echo "collect_preflight_check=fail"
              echo "submission_readiness_collect_preflight_check=fail"
              echo "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_strict"
              echo "submission_readiness_retry_command=retry_cmd"
              echo "submission_readiness_fail_step=collect_preflight"
              exit 1
            fi
            echo "collect_preflight_check_reason=latest_resolved_log_missing_default_skip"
            echo "collect_preflight_check=skipped"
            echo "submission_readiness_collect_preflight_check=skipped"
            echo "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_default_skip"
            """
        )
        retry_checker = textwrap.dedent(
            """\
            import argparse

            parser = argparse.ArgumentParser()
            parser.add_argument("--team-status", required=True)
            parser.add_argument("--require-retry-consistency-check-key", action="store_true")
            parser.add_argument("--require-strict-env-prefix-match", action="store_true")
            args = parser.parse_args()
            print(f"retry_checker_team_status={args.team_status}")
            print(
                "retry_checker_require_key="
                + ("yes" if args.require_retry_consistency_check_key else "no")
            )
            print(
                "retry_checker_require_strict_env="
                + ("yes" if args.require_strict_env_prefix_match else "no")
            )
            """
        )
        proc = self.run_script(
            body,
            body,
            retry_consistency_body=retry_checker,
            require_retry_consistency="1",
            require_retry_consistency_key="1",
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("retry_checker_team_status=/tmp/unused_team_status.md", proc.stdout)
        self.assertIn("retry_checker_require_key=yes", proc.stdout)
        self.assertIn("retry_checker_require_strict_env=no", proc.stdout)
        self.assertIn("fail_trace_require_retry_consistency=1", proc.stdout)
        self.assertIn("fail_trace_require_retry_consistency_key=1", proc.stdout)
        self.assertIn("fail_trace_require_retry_consistency_strict_env=0", proc.stdout)
        self.assertIn("fail_trace_retry_consistency_retry_command=python /tmp/", proc.stdout)
        self.assertIn("--team-status /tmp/unused_team_status.md --require-retry-consistency-check-key", proc.stdout)
        self.assertIn("fail_trace_retry_consistency_reasons=-", proc.stdout)
        self.assertIn("fail_trace_retry_consistency_reason_codes=-", proc.stdout)
        self.assertIn("fail_trace_retry_consistency_check=pass", proc.stdout)

    def test_runs_retry_consistency_checker_with_strict_env_flag_when_enabled(self):
        body = textwrap.dedent(
            """\
            if [[ "${C_COLLECT_LATEST_REQUIRE_FOUND:-0}" == "1" ]]; then
              echo "collect_preflight_check_reason=latest_resolved_log_missing_strict"
              echo "collect_preflight_check=fail"
              echo "submission_readiness_collect_preflight_check=fail"
              echo "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_strict"
              echo "submission_readiness_retry_command=retry_cmd"
              echo "submission_readiness_fail_step=collect_preflight"
              exit 1
            fi
            echo "collect_preflight_check_reason=latest_resolved_log_missing_default_skip"
            echo "collect_preflight_check=skipped"
            echo "submission_readiness_collect_preflight_check=skipped"
            echo "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_default_skip"
            """
        )
        retry_checker = textwrap.dedent(
            """\
            import argparse

            parser = argparse.ArgumentParser()
            parser.add_argument("--team-status", required=True)
            parser.add_argument("--require-retry-consistency-check-key", action="store_true")
            parser.add_argument("--require-strict-env-prefix-match", action="store_true")
            args = parser.parse_args()
            print(
                "retry_checker_require_strict_env="
                + ("yes" if args.require_strict_env_prefix_match else "no")
            )
            """
        )
        proc = self.run_script(
            body,
            body,
            retry_consistency_body=retry_checker,
            require_retry_consistency="1",
            require_retry_consistency_key="1",
            require_retry_consistency_strict_env="1",
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("retry_checker_require_strict_env=yes", proc.stdout)
        self.assertIn("fail_trace_require_retry_consistency_strict_env=1", proc.stdout)
        self.assertIn("fail_trace_retry_consistency_retry_command=python /tmp/", proc.stdout)
        self.assertIn(
            "--team-status /tmp/unused_team_status.md --require-retry-consistency-check-key --require-strict-env-prefix-match",
            proc.stdout,
        )

    def test_uses_c_require_retry_consistency_env_when_fail_trace_env_empty(self):
        body = textwrap.dedent(
            """\
            if [[ "${C_COLLECT_LATEST_REQUIRE_FOUND:-0}" == "1" ]]; then
              echo "collect_preflight_check_reason=latest_resolved_log_missing_strict"
              echo "collect_preflight_check=fail"
              echo "submission_readiness_collect_preflight_check=fail"
              echo "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_strict"
              echo "submission_readiness_retry_command=retry_cmd"
              echo "submission_readiness_fail_step=collect_preflight"
              exit 1
            fi
            echo "collect_preflight_check_reason=latest_resolved_log_missing_default_skip"
            echo "collect_preflight_check=skipped"
            echo "submission_readiness_collect_preflight_check=skipped"
            echo "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_default_skip"
            """
        )
        proc = self.run_script(
            body,
            body,
            require_retry_consistency="0",
            require_retry_consistency_key="1",
            require_retry_consistency_strict_env="1",
            extra_env={
                "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY": "",
                "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY": "",
                "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV": "",
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY": "0",
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_KEY": "1",
                "C_REQUIRE_FAIL_TRACE_RETRY_CONSISTENCY_STRICT_ENV": "1",
            },
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("fail_trace_require_retry_consistency=0", proc.stdout)
        self.assertIn("fail_trace_require_retry_consistency_key=1", proc.stdout)
        self.assertIn("fail_trace_require_retry_consistency_strict_env=1", proc.stdout)
        self.assertIn("fail_trace_retry_consistency_check=skipped", proc.stdout)

    def test_fails_when_retry_consistency_checker_fails(self):
        body = textwrap.dedent(
            """\
            if [[ "${C_COLLECT_LATEST_REQUIRE_FOUND:-0}" == "1" ]]; then
              echo "collect_preflight_check_reason=latest_resolved_log_missing_strict"
              echo "collect_preflight_check=fail"
              echo "submission_readiness_collect_preflight_check=fail"
              echo "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_strict"
              echo "submission_readiness_retry_command=retry_cmd"
              echo "submission_readiness_fail_step=collect_preflight"
              exit 1
            fi
            echo "collect_preflight_check_reason=latest_resolved_log_missing_default_skip"
            echo "collect_preflight_check=skipped"
            echo "submission_readiness_collect_preflight_check=skipped"
            echo "submission_readiness_collect_preflight_reason=latest_resolved_log_missing_default_skip"
            """
        )
        retry_checker = "raise SystemExit(9)\n"
        proc = self.run_script(
            body,
            body,
            retry_consistency_body=retry_checker,
            require_retry_consistency="1",
        )
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("fail_trace_retry_consistency_reasons=unknown", proc.stderr)
        self.assertIn("fail_trace_retry_consistency_reason_codes=unknown", proc.stderr)
        self.assertIn("fail_trace_retry_consistency_check=fail", proc.stderr)
        self.assertIn("FAIL_TRACE_AUDIT_RESULT=FAIL", proc.stderr)


if __name__ == "__main__":
    unittest.main()
