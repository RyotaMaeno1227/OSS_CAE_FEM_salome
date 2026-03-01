#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "extract_c_team_latest_collect_log.py"


def write_status(markdown: str) -> str:
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
        fp.write(markdown)
        return fp.name


class ExtractCTeamLatestCollectLogTest(unittest.TestCase):
    def run_script(self, markdown: str, *args: str) -> subprocess.CompletedProcess[str]:
        status_path = write_status(markdown)
        return subprocess.run(
            ["python", str(SCRIPT_PATH), "--team-status", status_path, *args],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

    def test_extracts_from_checker_command(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-30
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c30_collect.log --require-enabled
            """
        )
        proc = self.run_script(markdown, "--print-path-only")
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertEqual(proc.stdout.strip(), "/tmp/c30_collect.log")

    def test_extracts_from_collect_log_out(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-30
              - collect_log_out=/tmp/c30_finalize.log
            """
        )
        proc = self.run_script(markdown)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("collect_log=/tmp/c30_finalize.log", proc.stdout)
        self.assertIn("verdict=PASS", proc.stdout)

    def test_fails_when_log_is_missing(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-30
              - 実行コマンド: make -C FEM4C test
            """
        )
        proc = self.run_script(markdown)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("reason=collect_log_not_found", proc.stdout)

    def test_fails_when_c_section_is_missing(self):
        markdown = textwrap.dedent(
            """\
            ## Aチーム
            - 実行タスク: A-1
              - 実行コマンド: make -C FEM4C test
            """
        )
        proc = self.run_script(markdown)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("reason=missing_c_section", proc.stdout)

    def test_falls_back_to_previous_entry_when_latest_has_no_log(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-30 previous
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c30_previous.log --require-enabled

            - 実行タスク: C-30 latest
              - 実行コマンド: make -C FEM4C test
            """
        )
        proc = self.run_script(markdown)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("entry=- 実行タスク: C-30 previous", proc.stdout)
        self.assertIn("fallback_from_latest=1", proc.stdout)
        self.assertIn("collect_log=/tmp/c30_previous.log", proc.stdout)

    def test_prefers_checker_command_over_collect_log_out(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-33
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c33_checker.log --require-enabled
              - collect_log_out=/tmp/c33_collect_only.log
            """
        )
        proc = self.run_script(markdown, "--print-path-only")
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertEqual(proc.stdout.strip(), "/tmp/c33_checker.log")

    def test_require_existing_fails_when_resolved_log_is_missing(self):
        markdown = textwrap.dedent(
            """\
            ## Cチーム
            - 実行タスク: C-37
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py /tmp/c37_extract_missing.log --require-enabled
            """
        )
        proc = self.run_script(markdown, "--require-existing")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("collect_log=/tmp/c37_extract_missing.log", proc.stdout)
        self.assertIn("collect_log_exists=0", proc.stdout)
        self.assertIn("reason=collect_log_missing", proc.stdout)

    def test_require_existing_passes_when_resolved_log_exists(self):
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            log_path = fp.name
            fp.write("collect_result=PASS\n")
        markdown = textwrap.dedent(
            f"""\
            ## Cチーム
            - 実行タスク: C-37
              - 実行コマンド:
                - python scripts/check_c_team_collect_preflight_report.py {log_path} --require-enabled
            """
        )
        proc = self.run_script(markdown, "--require-existing")
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn(f"collect_log={log_path}", proc.stdout)
        self.assertIn("collect_log_exists=1", proc.stdout)
        self.assertIn("verdict=PASS", proc.stdout)


if __name__ == "__main__":
    unittest.main()
