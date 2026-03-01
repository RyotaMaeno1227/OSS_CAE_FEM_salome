#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "check_c_team_review_commands.py"


def write_status(markdown: str) -> str:
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
        fp.write(markdown)
        return fp.name


class CheckCTeamReviewCommandsTest(unittest.TestCase):
    def run_script(self, markdown: str, *args: str) -> subprocess.CompletedProcess[str]:
        status_path = write_status(markdown)
        return subprocess.run(
            ["python", str(SCRIPT_PATH), "--team-status", status_path, *args],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

    def test_pass_with_required_review_commands(self) -> None:
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-41
              - start_epoch=100
              - missing_log_review_command=rg -n 'collect_preflight' /tmp/entry.md
              - python scripts/check_c_team_collect_preflight_report.py /tmp/c.log --require-enabled -> PASS
              - collect_report_review_command=python scripts/check_c_team_collect_preflight_report.py /tmp/c.log --require-enabled --expect-team-status docs/team_status.md
            """
        )
        proc = self.run_script(markdown)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("verdict=PASS", proc.stdout)
        self.assertIn("reasons=-", proc.stdout)
        self.assertIn("reason_codes=-", proc.stdout)
        self.assertIn("reason_codes_source=-", proc.stdout)

    def test_fail_without_missing_log_review_command(self) -> None:
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-41
              - start_epoch=100
              - python scripts/check_c_team_collect_preflight_report.py /tmp/c.log --require-enabled -> PASS
              - collect_report_review_command=python scripts/check_c_team_collect_preflight_report.py /tmp/c.log --require-enabled --expect-team-status docs/team_status.md
            """
        )
        proc = self.run_script(markdown)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("missing missing_log_review_command", proc.stdout)
        self.assertIn("reason_codes=missing_missing_log_review_command", proc.stdout)
        self.assertIn("reason_codes_source=checker", proc.stdout)

    def test_fail_when_collect_report_command_missing_if_collect_check_exists(self) -> None:
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-41
              - start_epoch=100
              - missing_log_review_command=rg -n 'collect_preflight' /tmp/entry.md
              - python scripts/check_c_team_collect_preflight_report.py /tmp/c.log --require-enabled -> PASS
            """
        )
        proc = self.run_script(markdown)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("missing collect_report_review_command", proc.stdout)
        self.assertIn("reason_codes=missing_collect_report_review_command", proc.stdout)
        self.assertIn("reason_codes_source=checker", proc.stdout)

    def test_pass_without_collect_report_when_collect_check_absent(self) -> None:
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-41
              - start_epoch=100
              - missing_log_review_command=rg -n 'collect_preflight' /tmp/entry.md
            """
        )
        proc = self.run_script(markdown)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("verdict=PASS", proc.stdout)
        self.assertIn("reason_codes=-", proc.stdout)
        self.assertIn("reason_codes_source=-", proc.stdout)

    def test_pass_when_collect_report_requirement_disabled(self) -> None:
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-41
              - start_epoch=100
              - missing_log_review_command=rg -n 'collect_preflight' /tmp/entry.md
              - python scripts/check_c_team_collect_preflight_report.py /tmp/c.log --require-enabled -> PASS
            """
        )
        proc = self.run_script(markdown, "--no-require-collect-report-if-check-command")
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("verdict=PASS", proc.stdout)
        self.assertIn("reason_codes=-", proc.stdout)
        self.assertIn("reason_codes_source=-", proc.stdout)

    def test_json_output_includes_reason_codes(self) -> None:
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-58
              - start_epoch=100
              - python scripts/check_c_team_collect_preflight_report.py /tmp/c.log --require-enabled -> PASS
            """
        )
        proc = self.run_script(markdown, "--json")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn('"reason_codes": [', proc.stdout)
        self.assertIn('"missing_missing_log_review_command"', proc.stdout)
        self.assertIn('"reason_codes_source": "checker"', proc.stdout)

    def test_json_output_pass_has_empty_reason_codes(self) -> None:
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-58
              - start_epoch=100
              - missing_log_review_command=rg -n 'collect_preflight' /tmp/entry.md
            """
        )
        proc = self.run_script(markdown, "--json")
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn('"reason_codes": []', proc.stdout)
        self.assertIn('"reason_codes_source": "-"', proc.stdout)


if __name__ == "__main__":
    unittest.main()
