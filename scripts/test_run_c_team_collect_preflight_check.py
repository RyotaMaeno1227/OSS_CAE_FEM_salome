#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "run_c_team_collect_preflight_check.sh"


def write_log(text: str) -> str:
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
        fp.write(text)
        return fp.name


class RunCTeamCollectPreflightCheckTest(unittest.TestCase):
    def run_script(
        self,
        extra_env: dict[str, str] | None = None,
        status_path: str = "docs/team_status.md",
    ) -> subprocess.CompletedProcess[str]:
        env = os.environ.copy()
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

    def test_skips_without_log(self):
        proc = self.run_script()
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("collect_preflight_check=skipped", proc.stdout)

    def test_passes_when_enabled_log_matches_expected(self):
        log_path = write_log(
            "collect_result=PASS\n"
            "preflight_mode=enabled\n"
            "preflight_team_status=docs/team_status.md\n"
            "preflight_result=pass\n"
        )
        proc = self.run_script(extra_env={"C_COLLECT_PREFLIGHT_LOG": log_path})
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("C_TEAM_COLLECT_PREFLIGHT_REPORT", proc.stdout)
        self.assertIn("verdict=PASS", proc.stdout)
        self.assertIn("collect_preflight_check=pass", proc.stdout)

    def test_fails_with_reason_when_explicit_log_is_missing(self):
        missing_log = "/tmp/c37_explicit_missing_collect.log"
        Path(missing_log).unlink(missing_ok=True)
        proc = self.run_script(extra_env={"C_COLLECT_PREFLIGHT_LOG": missing_log})
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn(f"collect_preflight_log_missing={missing_log}", combined)
        self.assertIn("collect_preflight_check_reason=explicit_log_missing", combined)
        self.assertIn("collect_preflight_check=fail", combined)

    def test_fails_when_require_enabled_is_violated(self):
        log_path = write_log(
            "collect_result=PASS\n"
            "preflight_mode=disabled\n"
            "preflight_result=skipped\n"
        )
        proc = self.run_script(extra_env={"C_COLLECT_PREFLIGHT_LOG": log_path})
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("preflight_mode=disabled", proc.stdout)
        self.assertIn("verdict=FAIL", proc.stdout)

    def test_resolves_latest_collect_log_from_team_status(self):
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            status_path = fp.name
            fp.write(
                textwrap.dedent(
                    """\
                    ## Cチーム
                    - 実行タスク: C-30
                      - 実行コマンド:
                        - python scripts/check_c_team_collect_preflight_report.py /tmp/c30_latest_collect.log --require-enabled
                    """
                )
            )
        Path("/tmp/c30_latest_collect.log").write_text(
            "collect_result=PASS\n"
            "preflight_mode=enabled\n"
            f"preflight_team_status={status_path}\n"
            "preflight_result=pass\n",
            encoding="utf-8",
        )
        proc = self.run_script(
            extra_env={"C_COLLECT_PREFLIGHT_LOG": "latest"},
            status_path=status_path,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("collect_preflight_log_resolved=/tmp/c30_latest_collect.log", proc.stdout)
        self.assertIn("collect_preflight_check=pass", proc.stdout)

    def test_skips_when_latest_collect_log_is_not_found_by_default(self):
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            status_path = fp.name
            fp.write(
                textwrap.dedent(
                    """\
                    ## Cチーム
                    - 実行タスク: C-30
                      - 実行コマンド: make -C FEM4C test
                    """
                )
            )
        proc = self.run_script(
            extra_env={"C_COLLECT_PREFLIGHT_LOG": "latest"},
            status_path=status_path,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        combined = proc.stdout + proc.stderr
        self.assertIn("collect_log_not_found", combined)
        self.assertIn("collect_preflight_check_reason=latest_not_found_default_skip", combined)
        self.assertIn("collect_preflight_check=skipped", combined)

    def test_fails_when_latest_collect_log_is_required(self):
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            status_path = fp.name
            fp.write(
                textwrap.dedent(
                    """\
                    ## Cチーム
                    - 実行タスク: C-30
                      - 実行コマンド: make -C FEM4C test
                    """
                )
            )
        proc = self.run_script(
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_COLLECT_LATEST_REQUIRE_FOUND": "1",
            },
            status_path=status_path,
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn("collect_log_not_found", combined)
        self.assertIn("collect_preflight_check_reason=latest_not_found_strict", combined)
        self.assertIn("collect_preflight_check=fail", combined)

    def test_skips_when_latest_resolved_log_file_is_missing_by_default(self):
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            status_path = fp.name
            fp.write(
                textwrap.dedent(
                    """\
                    ## Cチーム
                    - 実行タスク: C-37
                      - 実行コマンド:
                        - python scripts/check_c_team_collect_preflight_report.py /tmp/c37_missing_default.log --require-enabled
                    """
                )
            )
        proc = self.run_script(
            extra_env={"C_COLLECT_PREFLIGHT_LOG": "latest"},
            status_path=status_path,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        combined = proc.stdout + proc.stderr
        self.assertIn("collect_preflight_log_resolved=/tmp/c37_missing_default.log", combined)
        self.assertIn("collect_preflight_log_missing=/tmp/c37_missing_default.log", combined)
        self.assertIn("collect_preflight_check_reason=latest_resolved_log_missing_default_skip", combined)
        self.assertIn("collect_preflight_check=skipped", combined)

    def test_fails_when_latest_resolved_log_file_is_missing_in_strict_mode(self):
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            status_path = fp.name
            fp.write(
                textwrap.dedent(
                    """\
                    ## Cチーム
                    - 実行タスク: C-37
                      - 実行コマンド:
                        - python scripts/check_c_team_collect_preflight_report.py /tmp/c37_missing_strict.log --require-enabled
                    """
                )
            )
        proc = self.run_script(
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_COLLECT_LATEST_REQUIRE_FOUND": "1",
            },
            status_path=status_path,
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn("collect_preflight_log_resolved=/tmp/c37_missing_strict.log", combined)
        self.assertIn("collect_preflight_log_missing=/tmp/c37_missing_strict.log", combined)
        self.assertIn("collect_preflight_check_reason=latest_resolved_log_missing_strict", combined)
        self.assertIn("collect_preflight_check=fail", combined)

    def test_skips_when_latest_resolves_to_invalid_report_by_default(self):
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            status_path = fp.name
            fp.write(
                textwrap.dedent(
                    """\
                    ## Cチーム
                    - 実行タスク: C-30
                      - 実行コマンド:
                        - python scripts/check_c_team_collect_preflight_report.py /tmp/c30_latest_invalid_collect.log --require-enabled
                    """
                )
            )
        Path("/tmp/c30_latest_invalid_collect.log").write_text(
            "preflight_mode=enabled\n",
            encoding="utf-8",
        )
        proc = self.run_script(
            extra_env={"C_COLLECT_PREFLIGHT_LOG": "latest"},
            status_path=status_path,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        combined = proc.stdout + proc.stderr
        self.assertIn("missing_keys=", combined)
        self.assertIn("collect_preflight_check_reason=latest_invalid_report_default_skip", combined)
        self.assertIn("collect_preflight_check=skipped", combined)

    def test_fails_when_latest_invalid_report_and_strict_mode_is_enabled(self):
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            status_path = fp.name
            fp.write(
                textwrap.dedent(
                    """\
                    ## Cチーム
                    - 実行タスク: C-30
                      - 実行コマンド:
                        - python scripts/check_c_team_collect_preflight_report.py /tmp/c30_latest_invalid_collect_strict.log --require-enabled
                    """
                )
            )
        Path("/tmp/c30_latest_invalid_collect_strict.log").write_text(
            "preflight_mode=enabled\n",
            encoding="utf-8",
        )
        proc = self.run_script(
            extra_env={
                "C_COLLECT_PREFLIGHT_LOG": "latest",
                "C_COLLECT_LATEST_REQUIRE_FOUND": "1",
            },
            status_path=status_path,
        )
        self.assertNotEqual(proc.returncode, 0)
        combined = proc.stdout + proc.stderr
        self.assertIn("missing_keys=", combined)
        self.assertIn("collect_preflight_check_reason=latest_invalid_report_strict", combined)
        self.assertIn("collect_preflight_check=fail", combined)

    def test_latest_prefers_checker_command_over_collect_log_out(self):
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            status_path = fp.name
            fp.write(
                textwrap.dedent(
                    """\
                    ## Cチーム
                    - 実行タスク: C-33
                      - 実行コマンド:
                        - python scripts/check_c_team_collect_preflight_report.py /tmp/c33_checker_preflight.log --require-enabled
                      - collect_log_out=/tmp/c33_collect_only_preflight.log
                    """
                )
            )
        Path("/tmp/c33_checker_preflight.log").write_text(
            "collect_result=PASS\n"
            "preflight_mode=enabled\n"
            f"preflight_team_status={status_path}\n"
            "preflight_result=pass\n",
            encoding="utf-8",
        )
        Path("/tmp/c33_collect_only_preflight.log").write_text(
            "preflight_mode=enabled\n",
            encoding="utf-8",
        )
        proc = self.run_script(
            extra_env={"C_COLLECT_PREFLIGHT_LOG": "latest"},
            status_path=status_path,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("collect_preflight_log_resolved=/tmp/c33_checker_preflight.log", proc.stdout)
        self.assertIn("collect_preflight_check=pass", proc.stdout)


if __name__ == "__main__":
    unittest.main()
