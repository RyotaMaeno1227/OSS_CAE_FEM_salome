#!/usr/bin/env python3
"""Append a rendered C-team session entry into docs/team_status.md."""
from __future__ import annotations

import argparse
from pathlib import Path
import sys


C_SECTION_HEADING = "## Cチーム"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Append C-team session entry markdown to team_status"
    )
    parser.add_argument(
        "--team-status",
        default="docs/team_status.md",
        help="team_status markdown path",
    )
    parser.add_argument(
        "--entry-file",
        required=True,
        help="Rendered session entry markdown file",
    )
    parser.add_argument(
        "--in-place",
        action="store_true",
        help="Write result back to --team-status (default: print only)",
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


def normalize_entry(entry_text: str) -> list[str]:
    raw = entry_text.strip("\n")
    if not raw.strip():
        raise SystemExit("ERROR: entry file is empty")
    lines = raw.splitlines()
    first = lines[0].strip()
    if not first.startswith("- 実行タスク:"):
        raise SystemExit("ERROR: entry must start with '- 実行タスク:'")
    return [line.rstrip() for line in lines]


def append_entry(lines: list[str], section_end: int, entry_lines: list[str]) -> list[str]:
    before = lines[:section_end]
    after = lines[section_end:]
    if before and before[-1].strip():
        before.append("")
    before.extend(entry_lines)
    before.append("")
    return before + after


def main() -> int:
    args = parse_args()
    team_status_path = Path(args.team_status)
    entry_path = Path(args.entry_file)

    original = read_text(team_status_path)
    lines = original.splitlines()
    _, section_end = section_bounds(lines, C_SECTION_HEADING)
    entry_lines = normalize_entry(read_text(entry_path))
    new_lines = append_entry(lines, section_end, entry_lines)
    new_text = "\n".join(new_lines) + "\n"

    print("APPEND_C_TEAM_ENTRY")
    print(f"team_status={team_status_path}")
    print(f"entry_file={entry_path}")
    print(f"insert_before_line={section_end + 1}")

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
