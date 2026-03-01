#!/usr/bin/env python3
"""Apply rendered dry-run block into latest C-team entry in team_status."""
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
        description="Apply dry-run markdown block to latest C-team entry in team_status"
    )
    parser.add_argument(
        "--team-status",
        default="docs/team_status.md",
        help="team_status markdown path",
    )
    parser.add_argument(
        "--block-file",
        required=True,
        help="Rendered block file path (from render_c_stage_team_status_block.py)",
    )
    parser.add_argument(
        "--in-place",
        action="store_true",
        help="Write changes back to --team-status (default: print only)",
    )
    parser.add_argument(
        "--target-start-epoch",
        type=int,
        default=None,
        help="Optional explicit target entry start_epoch",
    )
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


def latest_entry(spans: list[EntrySpan]) -> EntrySpan:
    with_epoch = [s for s in spans if s.start_epoch is not None]
    if with_epoch:
        return max(with_epoch, key=lambda s: s.start_epoch if s.start_epoch is not None else -1)
    return spans[-1]


def select_entry(spans: list[EntrySpan], target_start_epoch: int | None) -> EntrySpan:
    if target_start_epoch is None:
        return latest_entry(spans)
    for span in spans:
        if span.start_epoch == target_start_epoch:
            return span
    raise SystemExit(f"ERROR: target start_epoch not found: {target_start_epoch}")


def indent_block(block_text: str) -> list[str]:
    block_lines = [line.rstrip() for line in block_text.splitlines() if line.strip()]
    if not block_lines:
        raise SystemExit("ERROR: block file is empty")
    return [f"  {line}" for line in block_lines]


def block_replace_range(entry_lines: list[str]) -> tuple[int | None, int | None]:
    start = None
    for i, line in enumerate(entry_lines):
        if line.strip().startswith("- dry-run 生出力（strict-safe 記録）:"):
            start = i
            break
    if start is None:
        return None, None
    end = len(entry_lines)
    for j in range(start + 1, len(entry_lines)):
        if entry_lines[j].startswith("  - "):
            end = j
            break
    return start, end


def insert_anchor_index(entry_lines: list[str]) -> int:
    anchors = ("  - 追加実行コマンド", "  - pass/fail:")
    for anchor in anchors:
        for i, line in enumerate(entry_lines):
            if line.startswith(anchor):
                return i
    return len(entry_lines)


def apply_block_to_entry(entry_lines: list[str], indented_block: list[str]) -> list[str]:
    replace_start, replace_end = block_replace_range(entry_lines)
    if replace_start is not None and replace_end is not None:
        return entry_lines[:replace_start] + indented_block + entry_lines[replace_end:]
    insert_at = insert_anchor_index(entry_lines)
    return entry_lines[:insert_at] + indented_block + entry_lines[insert_at:]


def main() -> int:
    args = parse_args()
    team_status_path = Path(args.team_status)
    block_path = Path(args.block_file)
    original = read_text(team_status_path)
    lines = original.splitlines()
    section_start, section_end = section_bounds(lines, C_SECTION_HEADING)
    spans = collect_entries(lines, section_start, section_end)
    target = select_entry(spans, args.target_start_epoch)
    block_text = read_text(block_path)
    indented_block = indent_block(block_text)

    entry_lines = lines[target.start:target.end]
    replaced_entry = apply_block_to_entry(entry_lines, indented_block)
    new_lines = lines[:target.start] + replaced_entry + lines[target.end:]
    new_text = "\n".join(new_lines) + "\n"

    print("APPLY_C_STAGE_BLOCK_TO_TEAM_STATUS")
    print(f"team_status={team_status_path}")
    print(f"block_file={block_path}")
    print(f"entry_start_line={target.start + 1}")
    print(f"entry_end_line={target.end}")
    print(f"entry_start_epoch={target.start_epoch if target.start_epoch is not None else '-'}")
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
