#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


def load_module():
    script_path = Path(__file__).with_name("audit_c_team_staging.py")
    spec = importlib.util.spec_from_file_location("audit_c_team_staging", script_path)
    if spec is None or spec.loader is None:
        raise RuntimeError("failed to load audit_c_team_staging module")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)  # type: ignore[union-attr]
    return module


MOD = load_module()
REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "audit_c_team_staging.py"


class AuditCTeamStagingTest(unittest.TestCase):
    def test_choose_latest_c_entry_by_start_epoch(self):
        markdown = """## Cチーム
- 実行タスク: C-17
  - start_epoch=100
  - scripts/c_stage_dryrun.sh --log /tmp/old.log
  - dryrun_result=pass
- 実行タスク: C-18
  - start_epoch=200
  - scripts/c_stage_dryrun.sh --log /tmp/new.log
  - dryrun_result=pass
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertIn("C-18", audit.title)
        self.assertEqual(audit.start_epoch, 200)

    def test_fail_when_dryrun_missing(self):
        markdown = """## Cチーム
- 実行タスク: C-18
  - start_epoch=200
  - 実行コマンド: `make -C FEM4C`
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(audit.verdict(), "FAIL")
        self.assertIn("missing dryrun_result", audit.reasons())

    def test_require_pass(self):
        markdown = """## Cチーム
- 実行タスク: C-18
  - start_epoch=200
  - scripts/c_stage_dryrun.sh --log /tmp/fail.log
  - dryrun_result=fail
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(audit.verdict(require_pass=True), "FAIL")
        self.assertIn("missing dryrun_result=pass", audit.reasons(require_pass=True))

    def test_require_both_pass_fail(self):
        markdown = """## Cチーム
- 実行タスク: C-18
  - start_epoch=200
  - scripts/c_stage_dryrun.sh --log /tmp/c18.log
  - dryrun_result=pass
  - dryrun_result=fail
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(audit.verdict(require_both=True), "PASS")

    def test_global_fallback_reads_c_entry_outside_c_section(self):
        markdown = """## Cチーム
- 実行タスク: C-12
  - start_epoch=100
  - 実行コマンド: `make -C FEM4C`

## PMチーム
- 実行タスク: C-18
  - start_epoch=200
  - scripts/c_stage_dryrun.sh --log /tmp/c18.log
  - dryrun_result=pass
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True, global_fallback=True)
        self.assertIn("C-18", audit.title)
        self.assertEqual(audit.verdict(require_pass=True), "PASS")
        self.assertEqual(audit.source, "global_fallback")
        self.assertEqual(audit.verdict(require_pass=True, require_c_section=True), "FAIL")

    def test_no_global_fallback_uses_c_section_only(self):
        markdown = """## Cチーム
- 実行タスク: C-12
  - start_epoch=100
  - scripts/c_stage_dryrun.sh --log /tmp/c12.log
  - dryrun_result=pass

## PMチーム
- 実行タスク: C-18
  - start_epoch=200
  - scripts/c_stage_dryrun.sh --log /tmp/c18.log
  - dryrun_result=pass
  - dryrun_result=fail
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True, global_fallback=False)
        self.assertIn("C-12", audit.title)
        self.assertEqual(audit.source, "c_section")

    def test_require_coupled_freeze_detects_forbidden_path(self):
        markdown = """## Cチーム
- 実行タスク: C-19
  - start_epoch=200
  - scripts/c_stage_dryrun.sh --log /tmp/c.log
  - dryrun_result=pass
  - 変更ファイル: `FEM4C/src/analysis/runner.c`
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(audit.verdict(require_coupled_freeze=True), "FAIL")
        reasons = audit.reasons(require_coupled_freeze=True)
        self.assertTrue(any("forbidden_paths_detected" in r for r in reasons))

    def test_load_path_patterns_ignores_comments_and_blank_lines(self):
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", delete=False) as fp:
            fp.write("# comment\n\nFEM4C/src/analysis/runner.c\n")
            fp.write("FEM4C/src/fem4c.c\n")
            path = fp.name
        patterns = MOD.load_path_patterns(path)
        self.assertEqual(
            patterns,
            ["FEM4C/src/analysis/runner.c", "FEM4C/src/fem4c.c"],
        )

    def test_structured_path_parse_ignores_command_reference(self):
        markdown = """## Cチーム
- 実行タスク: C-19
  - start_epoch=200
  - scripts/c_stage_dryrun.sh --log /tmp/c.log
  - dryrun_result=pass
  - 変更ファイル: `docs/team_status.md`
  - 実行コマンド: `git diff FEM4C/src/analysis/runner.c`
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(
            audit.verdict(
                require_coupled_freeze=True,
                coupled_freeze_patterns=["FEM4C/src/analysis/runner.c"],
            ),
            "PASS",
        )

    def test_parse_paths_uses_dryrun_cached_list(self):
        markdown = """## Cチーム
- 実行タスク: C-19
  - start_epoch=200
  - scripts/c_stage_dryrun.sh --log /tmp/c.log
  - dryrun_result=pass
  - dryrun_cached_list<<EOF
M\tdocs/team_status.md
M\tdocs/fem4c_team_next_queue.md
EOF
  - 実行コマンド: `git diff FEM4C/src/analysis/runner.c`
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(
            audit.verdict(
                require_coupled_freeze=True,
                coupled_freeze_patterns=["FEM4C/src/analysis/runner.c"],
            ),
            "PASS",
        )

    def test_print_coupled_freeze_patterns_option(self):
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as md:
            md.write("## Cチーム\n- 実行タスク: C-1\n  - dryrun_result=pass\n")
            status_path = md.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            fp.write("FEM4C/src/analysis/runner.c\n")
            fp.write("FEM4C/src/fem4c.c\n")
            freeze_path = fp.name
        proc = subprocess.run(
            [
                "python",
                str(SCRIPT_PATH),
                "--team-status",
                status_path,
                "--coupled-freeze-file",
                freeze_path,
                "--print-coupled-freeze-patterns",
            ],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("FEM4C/src/analysis/runner.c", proc.stdout)

    def test_require_complete_timer(self):
        markdown = """## Cチーム
- 実行タスク: C-19
  - start_epoch=200
  - scripts/c_stage_dryrun.sh --log /tmp/c.log
  - dryrun_result=pass
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(audit.verdict(require_complete_timer=True), "FAIL")
        self.assertIn("missing end_epoch", audit.reasons(require_complete_timer=True))

    def test_require_complete_timer_passes_when_fields_exist(self):
        markdown = """## Cチーム
- 実行タスク: C-19
  - start_epoch=200
  - end_epoch=500
  - elapsed_min=30
  - scripts/c_stage_dryrun.sh --log /tmp/c.log
  - dryrun_result=pass
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(audit.verdict(require_complete_timer=True), "PASS")

    def test_require_complete_timer_prefers_latest_elapsed_and_end_epoch(self):
        markdown = """## Cチーム
- 実行タスク: C-57
  - start_epoch=200
  - タイマー出力（終了）:
    - end_epoch=480
  - タイマーガード出力（途中確認）:
    - elapsed_min=10
  - タイマーガード出力（報告前）:
    - elapsed_min=30
  - タイマー出力（終了）:
    - end_epoch=500
  - scripts/c_stage_dryrun.sh --log /tmp/c.log
  - dryrun_result=pass
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(audit.elapsed_min, 30)
        self.assertEqual(audit.end_epoch, 500)
        self.assertEqual(audit.verdict(require_complete_timer=True), "PASS")

    def test_require_complete_timer_ignores_incomplete_latest_end_block(self):
        markdown = """## Cチーム
- 実行タスク: C-57
  - start_epoch=200
  - タイマー出力（終了）:
```text
SESSION_TIMER_END
end_epoch=500
elapsed_min=30
SESSION_TIMER_END
end_epoch=510
```
  - scripts/c_stage_dryrun.sh --log /tmp/c.log
  - dryrun_result=pass
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(audit.elapsed_min, 30)
        self.assertEqual(audit.end_epoch, 500)
        self.assertEqual(audit.verdict(require_complete_timer=True), "PASS")

    def test_require_complete_timer_fails_with_pending_placeholder(self):
        markdown = """## Cチーム
- 実行タスク: C-24
  - start_epoch=200
  - end_epoch=<pending>
  - elapsed_min=<pending>
  - scripts/c_stage_dryrun.sh --log /tmp/c.log
  - dryrun_result=pass
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(audit.verdict(require_complete_timer=True), "FAIL")
        self.assertIn(
            "pending_placeholder_detected",
            audit.reasons(require_complete_timer=True),
        )

    def test_require_complete_timer_fails_with_token_missing_marker(self):
        markdown = """## Cチーム
- 実行タスク: C-24
  - start_epoch=200
  - scripts/c_stage_dryrun.sh --log /tmp/c.log
  - dryrun_result=pass
  - ERROR: token file not found: /tmp/c_team_missing.token
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(audit.verdict(require_complete_timer=True), "FAIL")
        self.assertIn(
            "token_missing_marker_detected",
            audit.reasons(require_complete_timer=True),
        )

    def test_require_safe_stage_command(self):
        markdown = """## Cチーム
- 実行タスク: C-20
  - start_epoch=200
  - end_epoch=500
  - elapsed_min=30
  - scripts/c_stage_dryrun.sh --log /tmp/c.log
  - dryrun_result=pass
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(audit.verdict(require_safe_stage_command=True), "FAIL")
        self.assertIn("missing safe_stage_command", audit.reasons(require_safe_stage_command=True))

    def test_require_safe_stage_command_passes_with_evidence(self):
        markdown = """## Cチーム
- 実行タスク: C-20
  - start_epoch=200
  - end_epoch=500
  - elapsed_min=30
  - scripts/c_stage_dryrun.sh --log /tmp/c.log
  - safe_stage_command=git add docs/team_status.md
  - dryrun_result=pass
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(audit.verdict(require_safe_stage_command=True), "PASS")

    def test_require_safe_stage_command_rejects_non_git_add(self):
        markdown = """## Cチーム
- 実行タスク: C-20
  - start_epoch=200
  - end_epoch=500
  - elapsed_min=30
  - scripts/c_stage_dryrun.sh --log /tmp/c.log
  - safe_stage_command=python scripts/run_team_audit.sh docs/team_status.md
  - dryrun_result=pass
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(audit.verdict(require_safe_stage_command=True), "FAIL")
        self.assertIn(
            "safe_stage_command_not_git_add",
            audit.reasons(require_safe_stage_command=True),
        )

    def test_require_no_template_placeholder_fails(self):
        markdown = """## Cチーム
- 実行タスク: C-26
  - start_epoch=200
  - end_epoch=500
  - elapsed_min=30
  - scripts/c_stage_dryrun.sh --log /tmp/c.log
  - safe_stage_command=git add docs/team_status.md
  - dryrun_result=pass
  - 実行コマンド / pass-fail:
    - <記入>
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(
            audit.verdict(require_safe_stage_command=True, require_no_template_placeholder=True),
            "FAIL",
        )
        self.assertIn(
            "template_placeholder_detected",
            audit.reasons(require_safe_stage_command=True, require_no_template_placeholder=True),
        )

    def test_require_no_template_placeholder_passes(self):
        markdown = """## Cチーム
- 実行タスク: C-26
  - start_epoch=200
  - end_epoch=500
  - elapsed_min=30
  - scripts/c_stage_dryrun.sh --log /tmp/c.log
  - safe_stage_command=git add docs/team_status.md
  - dryrun_result=pass
  - 実行コマンド / pass-fail:
    - scripts/c_stage_dryrun.sh --log /tmp/c.log -> PASS
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(
            audit.verdict(require_safe_stage_command=True, require_no_template_placeholder=True),
            "PASS",
        )

    def test_require_no_template_placeholder_fails_on_pass_fail_placeholder(self):
        markdown = """## Cチーム
- 実行タスク: C-27
  - start_epoch=200
  - end_epoch=500
  - elapsed_min=30
  - scripts/c_stage_dryrun.sh --log /tmp/c.log
  - safe_stage_command=git add docs/team_status.md
  - dryrun_result=pass
  - pass/fail:
    - <PASS|FAIL>
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(
            audit.verdict(require_safe_stage_command=True, require_no_template_placeholder=True),
            "FAIL",
        )
        self.assertIn("<PASS|FAIL>", audit.template_placeholders)
        self.assertIn(
            "template_placeholder_detected",
            audit.reasons(require_safe_stage_command=True, require_no_template_placeholder=True),
        )


if __name__ == "__main__":
    unittest.main()
