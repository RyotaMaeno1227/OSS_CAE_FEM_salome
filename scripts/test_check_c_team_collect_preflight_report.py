#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "check_c_team_collect_preflight_report.py"


def write_report(text: str) -> str:
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
        fp.write(text)
        return fp.name


class CheckCTeamCollectPreflightReportTest(unittest.TestCase):
    def run_script(
        self,
        report_text: str,
        require_enabled: bool = False,
        expect_team_status: str | None = None,
    ) -> subprocess.CompletedProcess[str]:
        path = write_report(report_text)
        cmd = ["python", str(SCRIPT_PATH), path]
        if require_enabled:
            cmd.append("--require-enabled")
        if expect_team_status:
            cmd.extend(["--expect-team-status", expect_team_status])
        return subprocess.run(
            cmd,
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

    def test_pass_for_enabled_preflight(self):
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            status_path = fp.name
        proc = self.run_script(
            textwrap.dedent(
                """\
                collect_result=PASS
                preflight_mode=enabled
                preflight_team_status={status_path}
                preflight_result=pass
                """.format(status_path=status_path)
            ),
            require_enabled=True,
            expect_team_status=status_path,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("verdict=PASS", proc.stdout)

    def test_pass_for_disabled_preflight(self):
        proc = self.run_script(
            textwrap.dedent(
                """\
                collect_result=PASS
                preflight_mode=disabled
                preflight_result=skipped
                """
            ),
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("verdict=PASS", proc.stdout)

    def test_fail_when_required_key_missing(self):
        proc = self.run_script(
            textwrap.dedent(
                """\
                collect_result=PASS
                preflight_mode=enabled
                """
            )
        )
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("missing_keys=preflight_result", proc.stdout)

    def test_fail_when_enabled_without_team_status(self):
        proc = self.run_script(
            textwrap.dedent(
                """\
                collect_result=PASS
                preflight_mode=enabled
                preflight_result=pass
                """
            )
        )
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("missing_keys=preflight_team_status", proc.stdout)

    def test_fail_when_expected_team_status_mismatch(self):
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            logged_status = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            expected_status = fp.name
        proc = self.run_script(
            textwrap.dedent(
                """\
                collect_result=PASS
                preflight_mode=enabled
                preflight_team_status={logged_status}
                preflight_result=pass
                """.format(logged_status=logged_status)
            ),
            require_enabled=True,
            expect_team_status=expected_status,
        )
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("expected_preflight_team_status=", proc.stdout)
        self.assertIn("actual_preflight_team_status=", proc.stdout)

    def test_pass_when_expected_team_status_is_relative_path(self):
        with tempfile.NamedTemporaryFile(
            "w", encoding="utf-8", suffix=".md", delete=False, dir=REPO_ROOT
        ) as fp:
            status_path = Path(fp.name)
        proc = self.run_script(
            textwrap.dedent(
                """\
                collect_result=PASS
                preflight_mode=enabled
                preflight_team_status={status_path}
                preflight_result=pass
                """.format(status_path=status_path)
            ),
            require_enabled=True,
            expect_team_status=str(status_path.relative_to(REPO_ROOT)),
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("verdict=PASS", proc.stdout)


if __name__ == "__main__":
    unittest.main()
