#!/usr/bin/env python3
"""Mark a C-team entry as token-missing and invalidate pending placeholders."""
from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re
import sys


C_SECTION_HEADING = "## Cチーム"
ENTRY_START_RE = re.compile(r"^- 実行タスク")
START_EPOCH_RE = re.compile(r"start_epoch\s*[:=]\s*`?(\d+)`?")


@dataclass
class EntrySpan:
    start: int
    end: int
    start_epoch: int | None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Mark target C entry as token-missing in team_status"
    )
    parser.add_argument("--team-status", default="docs/team_status.md")
    parser.add_argument("--target-start-epoch", type=int, required=True)
    parser.add_argument("--token-path", required=True)
    parser.add_argument("--end-rc", type=int, default=2)
    parser.add_argument("--guard-rc", type=int, default=2)
    parser.add_argument("--in-place", action="store_true")
    return parser.parse_args()


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except OSError as exc:
        raise SystemExit(f"ERROR: failed to read {path}: {exc}") from exc


def section_bounds(lines: list[str], heading: str) -> tuple[int, int]:
    start = -1
    for i, line in enumerate(lines):
        if line.strip() == heading:
            start = i + 1
            break
    if start < 0:
        raise SystemExit(f"ERROR: section not found: {heading}")
    end = len(lines)
    for j in range(start, len(lines)):
        if lines[j].startswith("## "):
            end = j
            break
    return start, end


def parse_start_epoch(entry_lines: list[str]) -> int | None:
    text = "\n".join(entry_lines)
    match = START_EPOCH_RE.search(text)
    return int(match.group(1)) if match else None


def collect_entries(lines: list[str], section_start: int, section_end: int) -> list[EntrySpan]:
    spans: list[EntrySpan] = []
    current_start: int | None = None
    for i in range(section_start, section_end):
        if ENTRY_START_RE.match(lines[i]):
            if current_start is not None:
                entry_lines = lines[current_start:i]
                spans.append(
                    EntrySpan(
                        start=current_start,
                        end=i,
                        start_epoch=parse_start_epoch(entry_lines),
                    )
                )
            current_start = i
    if current_start is not None:
        entry_lines = lines[current_start:section_end]
        spans.append(
            EntrySpan(
                start=current_start,
                end=section_end,
                start_epoch=parse_start_epoch(entry_lines),
            )
        )
    if not spans:
        raise SystemExit("ERROR: no C-team entry found in section")
    return spans


def select_entry(spans: list[EntrySpan], target_start_epoch: int) -> EntrySpan:
    for span in spans:
        if span.start_epoch == target_start_epoch:
            return span
    raise SystemExit(f"ERROR: target start_epoch not found: {target_start_epoch}")


def replace_timer_end_block(
    entry_lines: list[str],
    token_path: str,
    end_rc: int,
    guard_rc: int,
) -> list[str]:
    marker = "  - タイマー出力（終了）:"
    marker_idx = -1
    for i, line in enumerate(entry_lines):
        if line.startswith(marker):
            marker_idx = i
            break
    if marker_idx < 0:
        return entry_lines
    open_idx = marker_idx + 1
    if open_idx >= len(entry_lines) or not entry_lines[open_idx].strip().startswith("```"):
        return entry_lines
    close_idx = -1
    for j in range(open_idx + 1, len(entry_lines)):
        if entry_lines[j].strip().startswith("```"):
            close_idx = j
            break
    if close_idx < 0:
        return entry_lines

    replacement = [
        "```text",
        "SESSION_TIMER_END",
        f"session_token={token_path}",
        f"ERROR: token file not found: {token_path}",
        f"recovery_end_rc={end_rc}",
        f"recovery_guard_rc={guard_rc}",
        "```",
    ]
    return entry_lines[:open_idx] + replacement + entry_lines[close_idx + 1 :]


def replace_pass_fail(entry_lines: list[str]) -> list[str]:
    pass_idx = -1
    for i, line in enumerate(entry_lines):
        if line.startswith("  - pass/fail:"):
            pass_idx = i
            break
    if pass_idx < 0:
        return entry_lines
    bullet_idx = -1
    for j in range(pass_idx + 1, len(entry_lines)):
        if entry_lines[j].startswith("  - "):
            break
        if entry_lines[j].startswith("    - "):
            bullet_idx = j
            break
    fail_line = "    - FAIL（`token missing` により当該エントリは無効化）"
    if bullet_idx >= 0:
        entry_lines[bullet_idx] = fail_line
        return entry_lines
    entry_lines.insert(pass_idx + 1, fail_line)
    return entry_lines


def replace_pending(entry_lines: list[str]) -> list[str]:
    return [line.replace("<pending>", "token_missing") for line in entry_lines]


def main() -> int:
    args = parse_args()
    team_status_path = Path(args.team_status)
    original = read_text(team_status_path)
    lines = original.splitlines()
    section_start, section_end = section_bounds(lines, C_SECTION_HEADING)
    spans = collect_entries(lines, section_start, section_end)
    target = select_entry(spans, args.target_start_epoch)
    entry_lines = lines[target.start : target.end]
    entry_lines = replace_timer_end_block(
        entry_lines,
        token_path=args.token_path,
        end_rc=args.end_rc,
        guard_rc=args.guard_rc,
    )
    entry_lines = replace_pending(entry_lines)
    entry_lines = replace_pass_fail(entry_lines)

    new_lines = lines[:target.start] + entry_lines + lines[target.end :]
    new_text = "\n".join(new_lines) + "\n"

    print("MARK_C_TEAM_ENTRY_TOKEN_MISSING")
    print(f"team_status={team_status_path}")
    print(f"target_start_epoch={args.target_start_epoch}")
    print(f"token_path={args.token_path}")
    if args.in_place:
        try:
            team_status_path.write_text(new_text, encoding="utf-8")
        except OSError as exc:
            print(f"ERROR: failed to write {team_status_path}: {exc}", file=sys.stderr)
            return 1
        print("result=UPDATED")
        return 0

    print("result=DRY_RUN")
    print(new_text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
