#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import sys
import unittest
from pathlib import Path


def load_module():
    script_path = Path(__file__).with_name("audit_team_sessions.py")
    spec = importlib.util.spec_from_file_location("audit_team_sessions", script_path)
    if spec is None or spec.loader is None:
        raise RuntimeError("failed to load audit_team_sessions module")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)  # type: ignore[union-attr]
    return module


MOD = load_module()


class AuditTeamSessionsTest(unittest.TestCase):
    def test_choose_latest_by_start_epoch(self):
        markdown = """# header

## Aチーム
- 実行タスク: older
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=100
  - elapsed_min=40
  - 変更ファイル:
  - 実行コマンド:
  - pass/fail:
- 実行タスク: newer
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=200
  - elapsed_min=10
  - 変更ファイル:
  - 実行コマンド:
  - pass/fail:

## Bチーム

## Cチーム
"""
        audits = MOD.collect_latest_audits(markdown, ["A"])
        self.assertEqual(len(audits), 1)
        self.assertIn("newer", audits[0].title)
        self.assertEqual(audits[0].start_epoch, 200)

    def test_fail_reasons_include_missing_timer_and_elapsed(self):
        markdown = """## Aチーム
- 実行タスク: no timer
  - 変更ファイル:
  - 実行コマンド:
  - pass/fail:
"""
        audit = MOD.collect_latest_audits(markdown, ["A"])[0]
        reasons = audit.failure_reasons(min_elapsed=30, require_evidence=True)
        self.assertIn("missing SESSION_TIMER_START", reasons)
        self.assertIn("missing SESSION_TIMER_END", reasons)
        self.assertIn("missing elapsed_min", reasons)

    def test_sleep_detection_fails(self):
        markdown = """## Aチーム
- 実行タスク: with sleep
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=200
  - elapsed_min=35
  - 実行コマンド: `sleep 120`
  - 変更ファイル:
  - pass/fail:
"""
        audit = MOD.collect_latest_audits(markdown, ["A"])[0]
        self.assertEqual(audit.verdict(30, True), "FAIL")
        self.assertIn("artificial wait command detected", audit.failure_reasons(30, True))

    def test_evidence_optional(self):
        markdown = """## Aチーム
- 実行タスク: timer only
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=200
  - elapsed_min=35
"""
        audit = MOD.collect_latest_audits(markdown, ["A"])[0]
        self.assertEqual(audit.verdict(30, True), "FAIL")
        self.assertEqual(audit.verdict(30, False), "PASS")


if __name__ == "__main__":
    unittest.main()
