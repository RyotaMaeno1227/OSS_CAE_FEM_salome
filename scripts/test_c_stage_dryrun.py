#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "c_stage_dryrun.sh"


def run(cmd: list[str], cwd: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        cmd,
        cwd=cwd,
        text=True,
        capture_output=True,
        check=False,
    )


class CStageDryrunTest(unittest.TestCase):
    def init_repo(self, tmpdir: Path) -> None:
        run(["git", "init", "-q"], cwd=tmpdir)
        run(["git", "config", "user.email", "c-team@example.com"], cwd=tmpdir)
        run(["git", "config", "user.name", "C Team"], cwd=tmpdir)

    def test_pass_for_safe_target(self):
        with tempfile.TemporaryDirectory(prefix="c_stage_dryrun_pass_") as td:
            root = Path(td)
            self.init_repo(root)
            target = root / "docs" / "team_status.md"
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text("baseline\n", encoding="utf-8")
            run(["git", "add", "docs/team_status.md"], cwd=root)
            run(["git", "commit", "-q", "-m", "init"], cwd=root)
            target.write_text("changed\n", encoding="utf-8")

            freeze = root / "freeze.txt"
            freeze.write_text("FEM4C/src/analysis/runner.c\n", encoding="utf-8")

            proc = run(
                [
                    "bash",
                    str(SCRIPT_PATH),
                    "--add-target",
                    "docs/team_status.md",
                    "--coupled-freeze-file",
                    str(freeze),
                ],
                cwd=root,
            )
            self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
            self.assertIn("coupled_freeze_check=pass", proc.stdout)
            self.assertIn("safe_stage_targets=docs/team_status.md", proc.stdout)
            self.assertIn("safe_stage_command=git add docs/team_status.md", proc.stdout)
            self.assertIn("dryrun_result=pass", proc.stdout)

    def test_fail_for_coupled_freeze_target(self):
        with tempfile.TemporaryDirectory(prefix="c_stage_dryrun_fail_") as td:
            root = Path(td)
            self.init_repo(root)
            target = root / "FEM4C" / "src" / "analysis" / "runner.c"
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text("int runner(void) { return 0; }\n", encoding="utf-8")
            run(["git", "add", "FEM4C/src/analysis/runner.c"], cwd=root)
            run(["git", "commit", "-q", "-m", "init"], cwd=root)
            target.write_text("int runner(void) { return 1; }\n", encoding="utf-8")

            freeze = root / "freeze.txt"
            freeze.write_text("FEM4C/src/analysis/runner.c\n", encoding="utf-8")

            proc = run(
                [
                    "bash",
                    str(SCRIPT_PATH),
                    "--add-target",
                    "FEM4C/src/analysis/runner.c",
                    "--coupled-freeze-file",
                    str(freeze),
                ],
                cwd=root,
            )
            self.assertNotEqual(proc.returncode, 0)
            self.assertIn("coupled_freeze_check=fail", proc.stdout)
            self.assertIn("coupled_freeze_hits=FEM4C/src/analysis/runner.c", proc.stdout)
            self.assertIn("safe_stage_command=git add FEM4C/src/analysis/runner.c", proc.stdout)
            self.assertIn("dryrun_result=fail", proc.stdout)

    def test_missing_freeze_file_uses_fallback_patterns(self):
        with tempfile.TemporaryDirectory(prefix="c_stage_dryrun_fallback_") as td:
            root = Path(td)
            self.init_repo(root)
            target = root / "FEM4C" / "src" / "analysis" / "runner.c"
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text("int runner(void) { return 0; }\n", encoding="utf-8")
            run(["git", "add", "FEM4C/src/analysis/runner.c"], cwd=root)
            run(["git", "commit", "-q", "-m", "init"], cwd=root)
            target.write_text("int runner(void) { return 2; }\n", encoding="utf-8")

            proc = run(
                [
                    "bash",
                    str(SCRIPT_PATH),
                    "--add-target",
                    "FEM4C/src/analysis/runner.c",
                    "--coupled-freeze-file",
                    "/tmp/not_found_freeze_patterns.txt",
                ],
                cwd=root,
            )
            self.assertNotEqual(proc.returncode, 0)
            self.assertIn("coupled_freeze_check=fail", proc.stdout)
            self.assertIn("dryrun_result=fail", proc.stdout)


if __name__ == "__main__":
    unittest.main()
