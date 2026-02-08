#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import sys
import unittest
from pathlib import Path


def load_module():
    script_path = Path(__file__).with_name("audit_team_history.py")
    spec = importlib.util.spec_from_file_location("audit_team_history", script_path)
    if spec is None or spec.loader is None:
        raise RuntimeError("failed to load audit_team_history module")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)  # type: ignore[union-attr]
    return module


MOD = load_module()


class AuditTeamHistoryTest(unittest.TestCase):
    def test_team_prefix_only_excludes_pm_entry(self):
        markdown = """## Cチーム
- 実行タスク: PM-3 作業
  - elapsed_min=35
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=100
- 実行タスク: C-12 作業
  - elapsed_min=35
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=200
  - 変更ファイル:
  - 実行コマンド:
  - pass/fail:
"""
        entries = MOD.collect_entries(markdown, ["C"], team_prefix_only=True)
        self.assertEqual(len(entries), 1)
        self.assertIn("C-12", entries[0].title)

    def test_summary_counts_short_elapsed(self):
        markdown = """## Aチーム
- 実行タスク: A-1
  - elapsed_min=10
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=100
  - 変更ファイル:
  - 実行コマンド:
  - pass/fail:
- 実行タスク: A-2
  - elapsed_min=40
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=200
  - 変更ファイル:
  - 実行コマンド:
  - pass/fail:
"""
        entries = MOD.collect_entries(markdown, ["A"], team_prefix_only=True)
        report = MOD.summarize(entries, min_elapsed=30, require_evidence=True)
        team = report["teams"]["A"]
        self.assertEqual(team["total_entries"], 2)
        self.assertEqual(team["pass_count"], 1)
        self.assertEqual(team["short_elapsed_count"], 1)


if __name__ == "__main__":
    unittest.main()
