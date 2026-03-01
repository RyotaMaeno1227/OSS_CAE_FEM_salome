#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / "scripts" / "render_c_team_session_entry.py"


def write_temp(text: str, suffix: str) -> str:
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=suffix, delete=False) as fp:
        fp.write(text)
        return fp.name


class RenderCTeamSessionEntryTest(unittest.TestCase):
    def sample_token(self) -> str:
        return textwrap.dedent(
            """\
            team_tag=c_team
            start_utc=2026-02-09T10:00:00Z
            start_epoch=1770631200
            """
        )

    def sample_end(self, token_path: str) -> str:
        return textwrap.dedent(
            f"""\
            SESSION_TIMER_END
            session_token={token_path}
            team_tag=c_team
            start_utc=2026-02-09T10:00:00Z
            end_utc=2026-02-09T10:30:00Z
            start_epoch=1770631200
            end_epoch=1770633000
            elapsed_sec=1800
            elapsed_min=30
            """
        )

    def sample_guard(self, token_path: str) -> str:
        return textwrap.dedent(
            f"""\
            SESSION_TIMER_GUARD
            session_token={token_path}
            team_tag=c_team
            start_utc=2026-02-09T10:00:00Z
            now_utc=2026-02-09T10:30:00Z
            start_epoch=1770631200
            now_epoch=1770633000
            elapsed_sec=1800
            elapsed_min=30
            min_required=30
            guard_result=pass
            """
        )

    def sample_block(self) -> str:
        return textwrap.dedent(
            """\
            - dry-run 生出力（strict-safe 記録）:
              - `safe_stage_targets=docs/team_status.md docs/session_continuity_log.md`
              - `dryrun_result=pass`
            """
        )

    def run_script(
        self,
        token_path: str,
        end_path: str = "",
        guard_path: str = "",
        block_path: str = "",
        output_path: str = "",
        collect_timer_end: bool = False,
        collect_timer_guard: bool = False,
        guard_minutes: int | None = None,
        timer_end_output: str = "",
        timer_guard_output: str = "",
        c_stage_dryrun_log: str = "",
        collect_preflight_log: str = "",
        collect_latest_require_found: str = "0",
        done_lines: list[str] | None = None,
        in_progress_lines: list[str] | None = None,
        command_lines: list[str] | None = None,
        change_lines: list[str] | None = None,
        pass_fail_line: str = "",
    ) -> subprocess.CompletedProcess[str]:
        cmd = [
            "python",
            str(SCRIPT_PATH),
            "--task-title",
            "C-24 着手",
            "--session-token",
            token_path,
        ]
        if end_path:
            cmd.extend(["--timer-end-file", end_path])
        if collect_timer_end:
            cmd.append("--collect-timer-end")
        if guard_path:
            cmd.extend(["--timer-guard-file", guard_path])
        if collect_timer_guard:
            cmd.append("--collect-timer-guard")
        if guard_minutes is not None:
            cmd.extend(["--guard-minutes", str(guard_minutes)])
        if timer_end_output:
            cmd.extend(["--timer-end-output", timer_end_output])
        if timer_guard_output:
            cmd.extend(["--timer-guard-output", timer_guard_output])
        if block_path:
            cmd.extend(["--dryrun-block-file", block_path])
        if c_stage_dryrun_log:
            cmd.extend(["--c-stage-dryrun-log", c_stage_dryrun_log])
        if collect_preflight_log:
            cmd.extend(["--collect-preflight-log", collect_preflight_log])
        if collect_latest_require_found != "0":
            cmd.extend(["--collect-latest-require-found", collect_latest_require_found])
        for line in done_lines or []:
            cmd.extend(["--done-line", line])
        for line in in_progress_lines or []:
            cmd.extend(["--in-progress-line", line])
        for line in command_lines or []:
            cmd.extend(["--command-line", line])
        for line in change_lines or []:
            cmd.extend(["--change-line", line])
        if pass_fail_line:
            cmd.extend(["--pass-fail-line", pass_fail_line])
        if output_path:
            cmd.extend(["--output", output_path])
        return subprocess.run(
            cmd,
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

    def test_render_with_dryrun_block(self):
        token_path = write_temp(self.sample_token(), ".token")
        end_path = write_temp(self.sample_end(token_path), ".txt")
        block_path = write_temp(self.sample_block(), ".md")
        proc = self.run_script(token_path, end_path, block_path=block_path)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("SESSION_TIMER_START", proc.stdout)
        self.assertIn("SESSION_TIMER_END", proc.stdout)
        self.assertIn("dry-run 生出力（strict-safe 記録）", proc.stdout)
        self.assertIn("変更ファイル", proc.stdout)
        self.assertIn("docs/team_status.md docs/session_continuity_log.md", proc.stdout)

    def test_render_with_dryrun_command_evidence(self):
        token_path = write_temp(self.sample_token(), ".token")
        end_path = write_temp(self.sample_end(token_path), ".txt")
        proc = self.run_script(
            token_path,
            end_path,
            c_stage_dryrun_log="/tmp/c_stage.log",
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("scripts/c_stage_dryrun.sh --log /tmp/c_stage.log -> PASS", proc.stdout)

    def test_render_with_collect_preflight_log_evidence(self):
        token_path = write_temp(self.sample_token(), ".token")
        end_path = write_temp(self.sample_end(token_path), ".txt")
        proc = self.run_script(
            token_path,
            end_path,
            collect_preflight_log="/tmp/c30_collect.log",
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("preflight_latest_require_found=0 (disabled)", proc.stdout)
        self.assertIn(
            "python scripts/check_c_team_collect_preflight_report.py /tmp/c30_collect.log --require-enabled -> PASS",
            proc.stdout,
        )

    def test_render_with_collect_preflight_log_strict_latest_evidence(self):
        token_path = write_temp(self.sample_token(), ".token")
        end_path = write_temp(self.sample_end(token_path), ".txt")
        proc = self.run_script(
            token_path,
            end_path,
            collect_preflight_log="/tmp/c31_collect.log",
            collect_latest_require_found="1",
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("preflight_latest_require_found=1 (enabled)", proc.stdout)
        self.assertIn(
            "C_COLLECT_LATEST_REQUIRE_FOUND=1 python scripts/check_c_team_collect_preflight_report.py /tmp/c31_collect.log --require-enabled -> PASS",
            proc.stdout,
        )

    def test_render_with_guard_block(self):
        token_path = write_temp(self.sample_token(), ".token")
        end_path = write_temp(self.sample_end(token_path), ".txt")
        guard_path = write_temp(self.sample_guard(token_path), ".txt")
        proc = self.run_script(token_path, end_path, guard_path=guard_path)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("SESSION_TIMER_GUARD", proc.stdout)
        self.assertIn("guard_result=pass", proc.stdout)

    def test_render_uses_latest_complete_end_block(self):
        token_path = write_temp(self.sample_token(), ".token")
        end_text = textwrap.dedent(
            f"""\
            SESSION_TIMER_END
            session_token={token_path}
            team_tag=c_team
            start_utc=2026-02-09T10:00:00Z
            end_utc=2026-02-09T10:10:00Z
            start_epoch=1770631200
            end_epoch=1770631800
            elapsed_sec=600
            elapsed_min=10
            SESSION_TIMER_GUARD
            session_token={token_path}
            team_tag=c_team
            start_utc=2026-02-09T10:00:00Z
            now_utc=2026-02-09T10:20:00Z
            start_epoch=1770631200
            now_epoch=1770632400
            elapsed_sec=1200
            elapsed_min=20
            min_required=30
            guard_result=block
            SESSION_TIMER_END
            session_token={token_path}
            team_tag=c_team
            start_utc=2026-02-09T10:00:00Z
            end_utc=2026-02-09T10:30:00Z
            start_epoch=1770631200
            end_epoch=1770633000
            elapsed_sec=1800
            elapsed_min=30
            """
        )
        end_path = write_temp(end_text, ".txt")
        proc = self.run_script(token_path, end_path)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("end_epoch=1770633000", proc.stdout)
        self.assertIn("elapsed_min=30", proc.stdout)
        self.assertNotIn("end_epoch=1770631800", proc.stdout)

    def test_render_ignores_incomplete_latest_end_block(self):
        token_path = write_temp(self.sample_token(), ".token")
        end_text = textwrap.dedent(
            f"""\
            SESSION_TIMER_END
            session_token={token_path}
            team_tag=c_team
            start_utc=2026-02-09T10:00:00Z
            end_utc=2026-02-09T10:30:00Z
            start_epoch=1770631200
            end_epoch=1770633000
            elapsed_sec=1800
            elapsed_min=30
            SESSION_TIMER_END
            session_token={token_path}
            team_tag=c_team
            start_utc=2026-02-09T10:00:00Z
            end_utc=2026-02-09T10:31:00Z
            start_epoch=1770631200
            end_epoch=1770633060
            elapsed_sec=1860
            """
        )
        end_path = write_temp(end_text, ".txt")
        proc = self.run_script(token_path, end_path)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("end_epoch=1770633000", proc.stdout)
        self.assertIn("elapsed_min=30", proc.stdout)
        self.assertNotIn("end_epoch=1770633060", proc.stdout)

    def test_render_uses_latest_complete_guard_block(self):
        token_path = write_temp(self.sample_token(), ".token")
        end_path = write_temp(self.sample_end(token_path), ".txt")
        guard_text = textwrap.dedent(
            f"""\
            SESSION_TIMER_GUARD
            session_token={token_path}
            team_tag=c_team
            start_utc=2026-02-09T10:00:00Z
            now_utc=2026-02-09T10:30:00Z
            start_epoch=1770631200
            now_epoch=1770633000
            elapsed_sec=1800
            elapsed_min=30
            min_required=30
            guard_result=pass
            SESSION_TIMER_GUARD
            session_token={token_path}
            team_tag=c_team
            start_utc=2026-02-09T10:00:00Z
            now_utc=2026-02-09T10:31:00Z
            start_epoch=1770631200
            now_epoch=1770633060
            elapsed_sec=1860
            elapsed_min=31
            min_required=30
            """
        )
        guard_path = write_temp(guard_text, ".txt")
        proc = self.run_script(token_path, end_path, guard_path=guard_path)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("guard_result=pass", proc.stdout)
        self.assertIn("now_utc=2026-02-09T10:30:00Z", proc.stdout)
        self.assertNotIn("now_utc=2026-02-09T10:31:00Z", proc.stdout)

    def test_output_file(self):
        token_path = write_temp(self.sample_token(), ".token")
        end_path = write_temp(self.sample_end(token_path), ".txt")
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".md", delete=False) as fp:
            out_path = fp.name
        proc = self.run_script(token_path, end_path, output_path=out_path)
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        saved = Path(out_path).read_text(encoding="utf-8")
        self.assertIn("- 実行タスク: C-24 着手", saved)
        self.assertIn("render_output_path=", proc.stderr)

    def test_fail_when_end_missing_required_key(self):
        token_path = write_temp(self.sample_token(), ".token")
        broken_end = "SESSION_TIMER_END\nteam_tag=c_team\n"
        end_path = write_temp(broken_end, ".txt")
        proc = self.run_script(token_path, end_path)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("missing:", proc.stderr + proc.stdout)

    def test_collect_timer_end_mode(self):
        start_proc = subprocess.run(
            ["bash", "scripts/session_timer.sh", "start", "c_team"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(start_proc.returncode, 0, msg=start_proc.stdout + start_proc.stderr)
        token_path = ""
        for line in start_proc.stdout.splitlines():
            if line.startswith("session_token="):
                token_path = line.split("=", 1)[1].strip()
                break
        self.assertTrue(token_path)

        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            timer_end_output = fp.name
        proc = self.run_script(
            token_path=token_path,
            collect_timer_end=True,
            timer_end_output=timer_end_output,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        saved = Path(timer_end_output).read_text(encoding="utf-8")
        self.assertIn("SESSION_TIMER_END", saved)
        self.assertIn("SESSION_TIMER_START", proc.stdout)

    def test_collect_timer_guard_mode(self):
        start_proc = subprocess.run(
            ["bash", "scripts/session_timer.sh", "start", "c_team"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(start_proc.returncode, 0, msg=start_proc.stdout + start_proc.stderr)
        token_path = ""
        for line in start_proc.stdout.splitlines():
            if line.startswith("session_token="):
                token_path = line.split("=", 1)[1].strip()
                break
        self.assertTrue(token_path)

        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            timer_end_output = fp.name
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
            timer_guard_output = fp.name
        proc = self.run_script(
            token_path=token_path,
            collect_timer_end=True,
            collect_timer_guard=True,
            guard_minutes=0,
            timer_end_output=timer_end_output,
            timer_guard_output=timer_guard_output,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        guard_saved = Path(timer_guard_output).read_text(encoding="utf-8")
        self.assertIn("SESSION_TIMER_GUARD", guard_saved)
        self.assertIn("SESSION_TIMER_GUARD", proc.stdout)

    def test_custom_done_command_pass_fail_lines(self):
        token_path = write_temp(self.sample_token(), ".token")
        end_path = write_temp(self.sample_end(token_path), ".txt")
        proc = self.run_script(
            token_path,
            end_path,
            done_lines=["C-24 完了"],
            in_progress_lines=["C-25 着手"],
            command_lines=["python scripts/test_render_c_team_session_entry.py -> PASS"],
            pass_fail_line="PASS（custom）",
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("C-24 完了", proc.stdout)
        self.assertIn("C-25 着手", proc.stdout)
        self.assertIn("python scripts/test_render_c_team_session_entry.py -> PASS", proc.stdout)
        self.assertIn("PASS（custom）", proc.stdout)

    def test_custom_change_lines(self):
        token_path = write_temp(self.sample_token(), ".token")
        end_path = write_temp(self.sample_end(token_path), ".txt")
        proc = self.run_script(
            token_path,
            end_path,
            change_lines=["scripts/recover_c_team_token_missing_session.sh"],
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("変更ファイル", proc.stdout)
        self.assertIn("scripts/recover_c_team_token_missing_session.sh", proc.stdout)

    def test_render_with_missing_log_boundary_command_lines(self):
        token_path = write_temp(self.sample_token(), ".token")
        end_path = write_temp(self.sample_end(token_path), ".txt")
        proc = self.run_script(
            token_path,
            end_path,
            command_lines=[
                "collect_preflight_log_resolved=/tmp/c38_missing.log",
                "collect_preflight_log_missing=/tmp/c38_missing.log",
                "collect_preflight_check_reason=latest_resolved_log_missing_strict",
            ],
            collect_latest_require_found="1",
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stdout + proc.stderr)
        self.assertIn("preflight_latest_require_found=1 (enabled)", proc.stdout)
        self.assertIn("collect_preflight_log_resolved=/tmp/c38_missing.log", proc.stdout)
        self.assertIn("collect_preflight_log_missing=/tmp/c38_missing.log", proc.stdout)
        self.assertIn(
            "collect_preflight_check_reason=latest_resolved_log_missing_strict",
            proc.stdout,
        )


if __name__ == "__main__":
    unittest.main()
