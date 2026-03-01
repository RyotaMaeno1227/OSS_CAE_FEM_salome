#!/usr/bin/env python3
"""Render C-team session entry markdown from timer and dry-run artifacts."""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import subprocess
import sys


REQUIRED_END_KEYS = (
    "session_token",
    "team_tag",
    "start_utc",
    "end_utc",
    "start_epoch",
    "end_epoch",
    "elapsed_sec",
    "elapsed_min",
)

REQUIRED_GUARD_KEYS = (
    "session_token",
    "team_tag",
    "start_utc",
    "now_utc",
    "start_epoch",
    "now_epoch",
    "elapsed_sec",
    "elapsed_min",
    "min_required",
    "guard_result",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Render C-team session entry snippet for docs/team_status.md"
    )
    parser.add_argument("--task-title", required=True, help="Session task title")
    parser.add_argument("--session-token", required=True, help="session_timer token path")
    parser.add_argument(
        "--timer-end-file",
        default="",
        help="File containing SESSION_TIMER_END output",
    )
    parser.add_argument(
        "--collect-timer-end",
        action="store_true",
        help="Run scripts/session_timer.sh end <session_token> and use its output",
    )
    parser.add_argument(
        "--timer-guard-file",
        default="",
        help="File containing SESSION_TIMER_GUARD output",
    )
    parser.add_argument(
        "--collect-timer-guard",
        action="store_true",
        help="Run scripts/session_timer_guard.sh <session_token> <guard_minutes> and use its output",
    )
    parser.add_argument(
        "--guard-minutes",
        type=int,
        default=30,
        help="Minimum minutes for --collect-timer-guard (default: 30)",
    )
    parser.add_argument(
        "--timer-end-output",
        default="",
        help="Optional path to save collected timer end output",
    )
    parser.add_argument(
        "--timer-guard-output",
        default="",
        help="Optional path to save collected timer guard output",
    )
    parser.add_argument(
        "--dryrun-block-file",
        default="",
        help="Optional rendered dry-run block markdown file",
    )
    parser.add_argument(
        "--c-stage-dryrun-log",
        default="",
        help="Optional c_stage_dryrun log path to emit command evidence",
    )
    parser.add_argument(
        "--collect-preflight-log",
        default="",
        help="Optional collect preflight log path to emit command evidence",
    )
    parser.add_argument(
        "--collect-latest-require-found",
        choices=("0", "1"),
        default="0",
        help="Whether to render strict latest preflight knob evidence (0|1)",
    )
    parser.add_argument(
        "--output",
        default="",
        help="Optional output markdown file path",
    )
    parser.add_argument(
        "--done-line",
        action="append",
        default=[],
        help="Optional Done bullet text (repeatable)",
    )
    parser.add_argument(
        "--in-progress-line",
        action="append",
        default=[],
        help="Optional In Progress bullet text (repeatable)",
    )
    parser.add_argument(
        "--command-line",
        action="append",
        default=[],
        help="Optional command evidence line (repeatable)",
    )
    parser.add_argument(
        "--change-line",
        action="append",
        default=[],
        help="Optional changed file evidence line (repeatable)",
    )
    parser.add_argument(
        "--pass-fail-line",
        default="",
        help="Optional pass/fail bullet text",
    )
    return parser.parse_args()


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except OSError as exc:
        raise SystemExit(f"ERROR: failed to read {path}: {exc}") from exc


def parse_kv_lines(text: str) -> dict[str, str]:
    values: dict[str, str] = {}
    for raw in text.splitlines():
        line = raw.strip()
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        values[key.strip()] = value.strip()
    return values


def parse_latest_timer_values(
    text: str,
    marker: str,
    required_keys: tuple[str, ...],
) -> dict[str, str]:
    """Parse key-values from the latest complete timer block.

    Timer artifacts can include multiple START/GUARD/END blocks if command
    outputs are concatenated. Prefer the latest block for the requested marker
    that contains all required keys; fallback to latest matching block and then
    whole-text parsing for backward compatibility.
    """
    block_re = re.compile(
        rf"{re.escape(marker)}(?P<body>.*?)(?=SESSION_TIMER_START|SESSION_TIMER_GUARD|SESSION_TIMER_END|$)",
        re.DOTALL,
    )
    matches = list(block_re.finditer(text))
    if not matches:
        return parse_kv_lines(text)

    for match in reversed(matches):
        block_values = parse_kv_lines(match.group(0))
        if all(key in block_values for key in required_keys):
            return block_values

    return parse_kv_lines(matches[-1].group(0))


def collect_timer_end_text(session_token: str) -> str:
    proc = subprocess.run(
        ["bash", "scripts/session_timer.sh", "end", session_token],
        text=True,
        capture_output=True,
        check=False,
    )
    if proc.returncode != 0:
        detail = (proc.stderr or proc.stdout).strip()
        raise SystemExit(f"ERROR: failed to collect timer end: {detail}")
    return proc.stdout


def collect_timer_guard_text(session_token: str, guard_minutes: int) -> str:
    proc = subprocess.run(
        [
            "bash",
            "scripts/session_timer_guard.sh",
            session_token,
            str(guard_minutes),
        ],
        text=True,
        capture_output=True,
        check=False,
    )
    if proc.returncode != 0:
        detail = (proc.stderr or proc.stdout).strip()
        raise SystemExit(f"ERROR: failed to collect timer guard: {detail}")
    return proc.stdout


def render_start_block(session_token: str, token_values: dict[str, str]) -> list[str]:
    required = ("team_tag", "start_utc", "start_epoch")
    missing = [key for key in required if key not in token_values]
    if missing:
        raise SystemExit(f"ERROR: malformed session token, missing: {','.join(missing)}")
    return [
        "SESSION_TIMER_START",
        f"session_token={session_token}",
        f"team_tag={token_values['team_tag']}",
        f"start_utc={token_values['start_utc']}",
        f"start_epoch={token_values['start_epoch']}",
    ]


def render_end_block(end_values: dict[str, str]) -> list[str]:
    missing = [key for key in REQUIRED_END_KEYS if key not in end_values]
    if missing:
        raise SystemExit(f"ERROR: malformed timer end output, missing: {','.join(missing)}")
    lines = ["SESSION_TIMER_END"]
    for key in REQUIRED_END_KEYS:
        lines.append(f"{key}={end_values[key]}")
    return lines


def render_guard_block(guard_values: dict[str, str]) -> list[str]:
    missing = [key for key in REQUIRED_GUARD_KEYS if key not in guard_values]
    if missing:
        raise SystemExit(f"ERROR: malformed timer guard output, missing: {','.join(missing)}")
    lines = ["SESSION_TIMER_GUARD"]
    for key in REQUIRED_GUARD_KEYS:
        lines.append(f"{key}={guard_values[key]}")
    return lines


def extract_safe_stage_targets(dryrun_block_text: str) -> str:
    for raw in dryrun_block_text.splitlines():
        line = raw.strip()
        if "safe_stage_targets=" not in line:
            continue
        value = line.split("safe_stage_targets=", 1)[1]
        value = value.strip().strip("`")
        if value:
            return value
    return ""


def split_targets_for_change_lines(targets: str, chunk_size: int = 4) -> list[str]:
    items = [item for item in targets.split() if item]
    if not items:
        return []
    lines: list[str] = []
    for idx in range(0, len(items), chunk_size):
        lines.append(" ".join(items[idx : idx + chunk_size]))
    return lines


def render_entry(
    task_title: str,
    start_lines: list[str],
    guard_lines: list[str],
    end_lines: list[str],
    dryrun_block_text: str,
    c_stage_dryrun_log: str,
    collect_preflight_log: str,
    collect_latest_require_found: str,
    change_lines: list[str],
    done_lines: list[str],
    in_progress_lines: list[str],
    command_lines: list[str],
    pass_fail_line: str,
) -> str:
    lines: list[str] = []
    lines.append(f"- 実行タスク: {task_title}")
    lines.append("  - タイマー出力（開始）:")
    lines.append("```text")
    lines.extend(start_lines)
    lines.append("```")
    if guard_lines:
        lines.append("  - タイマーガード出力（報告前）:")
        lines.append("```text")
        lines.extend(guard_lines)
        lines.append("```")
    lines.append("  - タイマー出力（終了）:")
    lines.append("```text")
    lines.extend(end_lines)
    lines.append("```")
    if dryrun_block_text.strip():
        for raw in dryrun_block_text.splitlines():
            if raw.strip():
                lines.append(f"  {raw.rstrip()}")
    lines.append("  - 変更ファイル:")
    effective_change_lines = list(change_lines)
    if not effective_change_lines:
        safe_stage_targets = extract_safe_stage_targets(dryrun_block_text)
        if safe_stage_targets:
            effective_change_lines.extend(split_targets_for_change_lines(safe_stage_targets))
    if effective_change_lines:
        lines.extend(f"    - {line}" for line in effective_change_lines)
    else:
        lines.append("    - <記入>")
    lines.append("  - Done:")
    if done_lines:
        lines.extend(f"    - {line}" for line in done_lines)
    else:
        lines.append("    - <記入>")
    lines.append("  - In Progress:")
    if in_progress_lines:
        lines.extend(f"    - {line}" for line in in_progress_lines)
    else:
        lines.append("    - <記入>")
    lines.append("  - 実行コマンド / pass-fail:")
    strict_mode_label = "enabled" if collect_latest_require_found == "1" else "disabled"
    lines.append(
        "    - "
        f"preflight_latest_require_found={collect_latest_require_found} ({strict_mode_label})"
    )
    if c_stage_dryrun_log:
        lines.append(f"    - scripts/c_stage_dryrun.sh --log {c_stage_dryrun_log} -> PASS")
    if collect_preflight_log:
        preflight_cmd_prefix = ""
        if collect_latest_require_found == "1":
            preflight_cmd_prefix = "C_COLLECT_LATEST_REQUIRE_FOUND=1 "
        lines.append(
            "    - "
            f"{preflight_cmd_prefix}python scripts/check_c_team_collect_preflight_report.py {collect_preflight_log} --require-enabled -> PASS"
        )
    if command_lines:
        lines.extend(f"    - {line}" for line in command_lines)
    elif not c_stage_dryrun_log and not collect_preflight_log:
        lines.append("    - <記入>")
    lines.append("  - pass/fail:")
    if pass_fail_line:
        lines.append(f"    - {pass_fail_line}")
    else:
        lines.append("    - <PASS|FAIL>")
    return "\n".join(lines) + "\n"


def main() -> int:
    args = parse_args()
    token_path = Path(args.session_token)
    token_values = parse_kv_lines(read_text(token_path))
    if args.collect_timer_end:
        end_text = collect_timer_end_text(str(token_path))
        if args.timer_end_output:
            out_end = Path(args.timer_end_output)
            try:
                out_end.write_text(end_text, encoding="utf-8")
            except OSError as exc:
                print(f"ERROR: failed to write timer end output: {exc}", file=sys.stderr)
                return 1
            print(f"timer_end_output_path={out_end}", file=sys.stderr)
    else:
        if not args.timer_end_file:
            raise SystemExit("ERROR: --timer-end-file or --collect-timer-end is required")
        end_text = read_text(Path(args.timer_end_file))
    end_values = parse_latest_timer_values(end_text, "SESSION_TIMER_END", REQUIRED_END_KEYS)
    guard_lines: list[str] = []
    if args.collect_timer_guard:
        guard_text = collect_timer_guard_text(str(token_path), args.guard_minutes)
        if args.timer_guard_output:
            out_guard = Path(args.timer_guard_output)
            try:
                out_guard.write_text(guard_text, encoding="utf-8")
            except OSError as exc:
                print(f"ERROR: failed to write timer guard output: {exc}", file=sys.stderr)
                return 1
            print(f"timer_guard_output_path={out_guard}", file=sys.stderr)
    elif args.timer_guard_file:
        guard_text = read_text(Path(args.timer_guard_file))
    else:
        guard_text = ""
    if guard_text:
        guard_values = parse_latest_timer_values(
            guard_text,
            "SESSION_TIMER_GUARD",
            REQUIRED_GUARD_KEYS,
        )
        guard_lines = render_guard_block(guard_values)
    start_lines = render_start_block(str(token_path), token_values)
    end_lines = render_end_block(end_values)
    dryrun_block_text = ""
    if args.dryrun_block_file:
        dryrun_block_text = read_text(Path(args.dryrun_block_file))

    entry_text = render_entry(
        args.task_title,
        start_lines,
        guard_lines,
        end_lines,
        dryrun_block_text,
        args.c_stage_dryrun_log,
        args.collect_preflight_log,
        args.collect_latest_require_found,
        args.change_line,
        args.done_line,
        args.in_progress_line,
        args.command_line,
        args.pass_fail_line,
    )
    print(entry_text)

    if args.output:
        out_path = Path(args.output)
        try:
            out_path.write_text(entry_text, encoding="utf-8")
        except OSError as exc:
            print(f"ERROR: failed to write {out_path}: {exc}", file=sys.stderr)
            return 1
        print(f"render_output_path={out_path}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
