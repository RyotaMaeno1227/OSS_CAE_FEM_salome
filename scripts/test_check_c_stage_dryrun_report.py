#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "check_c_stage_dryrun_report.py"


class CheckCStageDryrunReportTest(unittest.TestCase):
    def run_script(self, report_text: str, policy: str = "pass") -> subprocess.CompletedProcess[str]:
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            fp.write(report_text)
            report_path = fp.name
        return subprocess.run(
            ["python", str(SCRIPT_PATH), report_path, "--policy", policy],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

    def sample_report(self, dryrun_result: str = "pass") -> str:
        return textwrap.dedent(
            f"""\
            dryrun_method=GIT_INDEX_FILE
            dryrun_targets=docs/team_status.md
            dryrun_changed_targets=docs/team_status.md
            dryrun_cached_list<<EOF
            M\tdocs/team_status.md
            EOF
            forbidden_check=pass
            coupled_freeze_file=scripts/c_coupled_freeze_forbidden_paths.txt
            coupled_freeze_hits=-
            coupled_freeze_check=pass
            required_set_check=pass
            safe_stage_targets=docs/team_status.md
            safe_stage_command=git add docs/team_status.md
            dryrun_result={dryrun_result}
            """
        )

    def test_pass_report(self):
        proc = self.run_script(self.sample_report("pass"), policy="pass")
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("verdict=PASS", proc.stdout)

    def test_fail_when_required_key_missing(self):
        broken = self.sample_report("pass").replace("safe_stage_command=git add docs/team_status.md\n", "")
        proc = self.run_script(broken, policy="any")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("missing_keys=safe_stage_command", proc.stdout)

    def test_fail_when_policy_pass_and_result_fail(self):
        proc = self.run_script(self.sample_report("fail"), policy="pass")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("dryrun_result_not_pass", proc.stdout)

    def test_fail_when_safe_stage_command_not_git_add(self):
        broken = self.sample_report("pass").replace(
            "safe_stage_command=git add docs/team_status.md",
            "safe_stage_command=python scripts/run_team_audit.sh docs/team_status.md",
        )
        proc = self.run_script(broken, policy="pass")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("safe_stage_command_not_git_add", proc.stdout)

    def test_fail_when_safe_stage_targets_mismatch(self):
        broken = self.sample_report("pass").replace(
            "safe_stage_targets=docs/team_status.md",
            "safe_stage_targets=docs/team_status.md docs/session_continuity_log.md",
        )
        proc = self.run_script(broken, policy="pass")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("safe_stage_command_target_mismatch", proc.stdout)


if __name__ == "__main__":
    unittest.main()
