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
        reasons = audit.failure_reasons(
            min_elapsed=30,
            max_elapsed=0,
            require_evidence=True,
            require_impl_changes=False,
        )
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
        self.assertEqual(audit.verdict(30, 0, True, False), "FAIL")
        self.assertIn(
            "artificial wait command detected",
            audit.failure_reasons(30, 0, True, False),
        )

    def test_evidence_optional(self):
        markdown = """## Aチーム
- 実行タスク: timer only
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=200
  - elapsed_min=35
"""
        audit = MOD.collect_latest_audits(markdown, ["A"])[0]
        self.assertEqual(audit.verdict(30, 0, True, False), "FAIL")
        self.assertEqual(audit.verdict(30, 0, False, False), "PASS")

    def test_elapsed_prefers_session_timer_end_over_guard(self):
        markdown = """## Bチーム
- 実行タスク: guard then end
  - SESSION_TIMER_START
  - start_epoch=100
  - SESSION_TIMER_GUARD
  - elapsed_min=24
  - guard_result=block
  - SESSION_TIMER_END
  - end_epoch=200
  - elapsed_min=30
  - 変更ファイル:
  - 実行コマンド:
  - pass/fail:
"""
        audit = MOD.collect_latest_audits(markdown, ["B"])[0]
        self.assertEqual(audit.elapsed_min, 30)
        self.assertEqual(audit.verdict(30, 0, True, False), "PASS")

    def test_elapsed_uses_latest_session_timer_end_when_multiple(self):
        markdown = """## Bチーム
- 実行タスク: multiple sessions in one entry
  - SESSION_TIMER_START
  - start_epoch=100
  - SESSION_TIMER_END
  - elapsed_min=19
  - SESSION_TIMER_START
  - start_epoch=200
  - SESSION_TIMER_END
  - elapsed_min=5
  - 変更ファイル:
  - 実行コマンド:
  - pass/fail:
"""
        audit = MOD.collect_latest_audits(markdown, ["B"])[0]
        self.assertEqual(audit.elapsed_min, 5)
        self.assertEqual(audit.start_epoch, 200)
        self.assertEqual(audit.verdict(30, 0, True, False), "FAIL")

    def test_require_impl_changes_fails_on_docs_only(self):
        markdown = """## Aチーム
- 実行タスク: docs only
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=100
  - elapsed_min=40
  - 変更ファイル: `docs/team_status.md`
  - 実行コマンド: `python scripts/audit_team_sessions.py --team-status docs/team_status.md`
  - pass/fail: PASS
"""
        audit = MOD.collect_latest_audits(markdown, ["A"])[0]
        self.assertFalse(audit.has_impl_changes)
        self.assertEqual(audit.verdict(30, 0, True, True), "FAIL")
        self.assertIn(
            "changes evidence does not include implementation files",
            audit.failure_reasons(30, 0, True, True),
        )

    def test_require_impl_changes_passes_with_script_change(self):
        markdown = """## Aチーム
- 実行タスク: impl change
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=100
  - elapsed_min=40
  - 変更ファイル: `scripts/audit_team_sessions.py`
  - 実行コマンド: `python scripts/test_audit_team_sessions.py`
  - pass/fail: PASS
"""
        audit = MOD.collect_latest_audits(markdown, ["A"])[0]
        self.assertTrue(audit.has_impl_changes)
        self.assertEqual(audit.verdict(30, 0, True, True), "PASS")

    def test_require_impl_changes_detects_non_backtick_paths(self):
        markdown = """## Cチーム
- 実行タスク: impl path without backticks
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=300
  - elapsed_min=40
  - 変更ファイル:
    - FEM4C/src/io/input.c
    - docs/team_status.md
  - 実行コマンド: scripts/c_stage_dryrun.sh
  - pass/fail: PASS
"""
        audit = MOD.collect_latest_audits(markdown, ["C"])[0]
        self.assertIn("FEM4C/src/io/input.c", audit.changed_paths)
        self.assertTrue(audit.has_impl_changes)
        self.assertEqual(audit.verdict(30, 0, True, True), "PASS")

    def test_changed_paths_ignores_wildcard_truncation(self):
        markdown = """## Cチーム
- 実行タスク: wildcard path
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=300
  - elapsed_min=40
  - 変更ファイル:
    - scripts/test_*.py
    - scripts/check_c_team_submission_readiness.sh
  - 実行コマンド: bash scripts/check_c_team_submission_readiness.sh docs/team_status.md 30
  - pass/fail: PASS
"""
        audit = MOD.collect_latest_audits(markdown, ["C"])[0]
        self.assertNotIn("scripts/test_", audit.changed_paths)
        self.assertIn("scripts/check_c_team_submission_readiness.sh", audit.changed_paths)

    def test_consecutive_identical_commands_fail(self):
        markdown = """## Aチーム
- 実行タスク: same command repeated
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=100
  - elapsed_min=40
  - 変更ファイル: `scripts/audit_team_sessions.py`
  - 実行コマンド:
    - `make -C FEM4C mbd_ci_contract_test` -> PASS
    - `make -C FEM4C mbd_ci_contract_test` -> PASS
  - pass/fail: PASS
"""
        audit = MOD.collect_latest_audits(markdown, ["A"])[0]
        self.assertEqual(audit.max_same_command_run, 2)
        self.assertEqual(audit.verdict(30, 0, True, True), "FAIL")
        reasons = audit.failure_reasons(30, 0, True, True)
        self.assertTrue(any("consecutive identical command detected" in r for r in reasons))

    def test_disable_consecutive_command_check(self):
        markdown = """## Bチーム
- 実行タスク: repeated command but allowed
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=200
  - elapsed_min=40
  - 変更ファイル: `FEM4C/scripts/run_b8_regression.sh`
  - 実行コマンド:
    - `make -C FEM4C mbd_b8_regression_test` -> PASS
    - `make -C FEM4C mbd_b8_regression_test` -> PASS
  - pass/fail: PASS
"""
        audit = MOD.collect_latest_audits(markdown, ["B"])[0]
        self.assertEqual(audit.max_same_command_run, 2)
        self.assertEqual(audit.verdict(30, 0, True, True, max_consecutive_same_command=0), "PASS")

    def test_non_consecutive_same_command_passes(self):
        markdown = """## Cチーム
- 実行タスク: same command non-consecutive
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=300
  - elapsed_min=40
  - 変更ファイル: `scripts/check_c_team_submission_readiness.sh`
  - 実行コマンド:
    - `python scripts/test_collect_c_team_session_evidence.py` -> PASS
    - `make -C FEM4C mbd_ci_contract_test` -> PASS
    - `python scripts/test_collect_c_team_session_evidence.py` -> PASS
  - pass/fail: PASS
"""
        audit = MOD.collect_latest_audits(markdown, ["C"])[0]
        self.assertEqual(audit.max_same_command_run, 1)
        self.assertEqual(audit.verdict(30, 0, True, True), "PASS")

    def test_max_elapsed_fails_when_exceeded(self):
        markdown = """## Cチーム
- 実行タスク: suspiciously long
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=300
  - elapsed_min=114
  - 変更ファイル: `scripts/check_c_team_submission_readiness.sh`
  - 実行コマンド: `python scripts/test_check_c_team_submission_readiness.py`
  - pass/fail: PASS
"""
        audit = MOD.collect_latest_audits(markdown, ["C"])[0]
        self.assertEqual(audit.verdict(30, 90, True, True), "FAIL")
        reasons = audit.failure_reasons(30, 90, True, True)
        self.assertIn("elapsed_min>90", reasons)

    def test_max_elapsed_disabled_with_zero(self):
        markdown = """## Bチーム
- 実行タスク: long but allowed when disabled
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=300
  - elapsed_min=114
  - 変更ファイル: `FEM4C/scripts/run_b8_regression_full.sh`
  - 実行コマンド: `make -C FEM4C mbd_b8_regression_full_test`
  - pass/fail: PASS
"""
        audit = MOD.collect_latest_audits(markdown, ["B"])[0]
        self.assertEqual(audit.verdict(30, 0, True, True), "PASS")

    def test_collect_latest_prefers_global_explicit_team_entry(self):
        markdown = """## Aチーム
- 実行タスク: A-41 old section entry
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=100
  - elapsed_min=31
  - 変更ファイル: `FEM4C/scripts/run_a24_regression_full.sh`
  - 実行コマンド: `make -C FEM4C mbd_a24_regression_full_test`
  - pass/fail: PASS

## PMチーム
- 実行タスク: A-team A-42 Done / A-43 In Progress
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=200
  - elapsed_min=30
  - 変更ファイル: `FEM4C/scripts/run_a24_batch.sh`
  - 実行コマンド: `make -C FEM4C mbd_a24_batch_test`
  - pass/fail: PASS
"""
        audit = MOD.collect_latest_audits(markdown, ["A"])[0]
        self.assertIn("A-42 Done / A-43", audit.title)
        self.assertEqual(audit.start_epoch, 200)

    def test_collect_latest_detects_b_prefix_entry_outside_b_section(self):
        markdown = """## Bチーム
- 実行タスク: B-36 old section entry
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=100
  - elapsed_min=31
  - 変更ファイル: `FEM4C/scripts/test_b8_knob_matrix.sh`
  - 実行コマンド: `make -C FEM4C mbd_b8_knob_matrix_test`
  - pass/fail: PASS

## PMチーム
- 実行タスク: B-37（Done）/ B-38（In Progress, Auto-Next）
  - SESSION_TIMER_START
  - SESSION_TIMER_END
  - start_epoch=300
  - elapsed_min=92
  - 変更ファイル: `FEM4C/scripts/run_b8_regression_full.sh`
  - 実行コマンド: `make -C FEM4C mbd_b8_regression_test`
  - pass/fail: PASS
"""
        audit = MOD.collect_latest_audits(markdown, ["B"])[0]
        self.assertIn("B-37", audit.title)
        self.assertEqual(audit.start_epoch, 300)


if __name__ == "__main__":
    unittest.main()
