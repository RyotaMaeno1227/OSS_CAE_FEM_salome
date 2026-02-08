#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import sys
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


class AuditCTeamStagingTest(unittest.TestCase):
    def test_pass_with_dryrun_result_and_command(self):
        markdown = """## Cチーム
- 実行タスク: C-18 実行
  - start_epoch=200
  - 実行コマンド: scripts/c_stage_dryrun.sh --log /tmp/c.log
  - dryrun_result=pass
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(audit.verdict(), "PASS")
        self.assertEqual(audit.dryrun_result_values, ["pass"])

    def test_fail_when_missing_dryrun_result(self):
        markdown = """## Cチーム
- 実行タスク: C-18 実行
  - start_epoch=100
  - 実行コマンド: scripts/c_stage_dryrun.sh --log /tmp/c.log
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(audit.verdict(), "FAIL")
        self.assertIn("missing dryrun_result", audit.reasons())

    def test_choose_latest_by_start_epoch(self):
        markdown = """## Cチーム
- 実行タスク: C-18 older
  - start_epoch=100
  - 実行コマンド: scripts/c_stage_dryrun.sh --log /tmp/old.log
  - dryrun_result=pass
- 実行タスク: C-18 newer
  - start_epoch=300
  - 実行コマンド: scripts/c_stage_dryrun.sh --log /tmp/new.log
  - dryrun_result=fail
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertIn("newer", audit.title)
        self.assertEqual(audit.dryrun_result_values, ["fail"])

    def test_require_both_detects_missing_fail_evidence(self):
        markdown = """## Cチーム
- 実行タスク: C-18 実行
  - start_epoch=200
  - 実行コマンド: scripts/c_stage_dryrun.sh --log /tmp/c.log
  - dryrun_result=pass
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertEqual(audit.verdict(require_both=True), "FAIL")
        self.assertIn("missing dryrun_result=fail", audit.reasons(require_both=True))

    def test_prefix_filter_skips_pm_entry(self):
        markdown = """## Cチーム
- 実行タスク: PM-3 note
  - start_epoch=500
  - dryrun_result=pass
  - 実行コマンド: scripts/c_stage_dryrun.sh --log /tmp/pm.log
- 実行タスク: C-19 task
  - start_epoch=300
  - dryrun_result=pass
  - 実行コマンド: scripts/c_stage_dryrun.sh --log /tmp/c.log
"""
        audit = MOD.collect_latest(markdown, team_prefix_only=True)
        self.assertIn("C-19", audit.title)


if __name__ == "__main__":
    unittest.main()
