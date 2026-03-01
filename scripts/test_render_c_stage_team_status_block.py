#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "render_c_stage_team_status_block.py"


class RenderCStageTeamStatusBlockTest(unittest.TestCase):
    def run_script(
        self,
        report_text: str,
        output_path: str = "",
    ) -> subprocess.CompletedProcess[str]:
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".log", delete=False) as fp:
            fp.write(report_text)
            report_path = fp.name
        cmd = ["python", str(SCRIPT_PATH), report_path]
        if output_path:
            cmd.extend(["--output", output_path])
        return subprocess.run(
            cmd,
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

    def sample_report(self) -> str:
        return textwrap.dedent(
            """\
            dryrun_method=GIT_INDEX_FILE
            dryrun_targets=docs/team_status.md
            dryrun_changed_targets=docs/team_status.md
            forbidden_check=pass
            coupled_freeze_file=scripts/c_coupled_freeze_forbidden_paths.txt
            coupled_freeze_hits=-
            coupled_freeze_check=pass
            required_set_check=pass
            safe_stage_targets=docs/team_status.md
            safe_stage_command=git add docs/team_status.md
            dryrun_result=pass
            """
        )

    def test_render_markdown_block(self):
        proc = self.run_script(self.sample_report())
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("- dry-run 生出力（strict-safe 記録）:", proc.stdout)
        self.assertIn("`safe_stage_command=git add docs/team_status.md`", proc.stdout)

    def test_fail_when_required_key_missing(self):
        broken = self.sample_report().replace("safe_stage_command=git add docs/team_status.md\n", "")
        proc = self.run_script(broken)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("verdict=FAIL", proc.stdout)
        self.assertIn("missing_keys=safe_stage_command", proc.stdout)

    def test_write_output_file(self):
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            output_path = fp.name
        proc = self.run_script(self.sample_report(), output_path=output_path)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        output_text = Path(output_path).read_text(encoding="utf-8")
        self.assertIn("`dryrun_result=pass`", output_text)
        self.assertIn("render_output_path=", proc.stderr)


if __name__ == "__main__":
    unittest.main()
