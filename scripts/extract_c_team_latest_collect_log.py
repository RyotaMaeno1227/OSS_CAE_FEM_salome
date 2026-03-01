#!/usr/bin/env python3
"""Extract latest collect preflight log path from latest C-team entry."""
from __future__ import annotations

import argparse
import re
from pathlib import Path


ENTRY_START_RE = re.compile(r"^- 実行タスク")
LOG_PATTERNS = (
    re.compile(r"check_c_team_collect_preflight_report\.py\s+([^\s`]+)"),
    re.compile(r"C_COLLECT_PREFLIGHT_LOG=([^\s`]+)"),
    re.compile(r"--collect-log-out\s+([^\s`]+)"),
    re.compile(r"collect_log_out=([^\s`]+)"),
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Extract collect preflight log path from latest C-team entry"
    )
    parser.add_argument("--team-status", default="docs/team_status.md")
    parser.add_argument(
        "--print-path-only",
        action="store_true",
        help="Print only resolved log path on success",
    )
    parser.add_argument(
        "--require-existing",
        action="store_true",
        help="Fail when resolved log path does not exist as a file",
    )
    return parser.parse_args()


def load_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except OSError as exc:
        raise SystemExit(f"ERROR: failed to read {path}: {exc}") from exc


def find_c_section(lines: list[str]) -> list[str]:
    start = None
    for idx, line in enumerate(lines):
        if line.strip() == "## Cチーム":
            start = idx + 1
            break
    if start is None:
        return []
    for idx in range(start, len(lines)):
        if lines[idx].startswith("## "):
            return lines[start:idx]
    return lines[start:]


def split_entries(lines: list[str]) -> list[list[str]]:
    entries: list[list[str]] = []
    current: list[str] = []
    for line in lines:
        if ENTRY_START_RE.match(line):
            if current:
                entries.append(current)
                current = []
            current.append(line)
            continue
        if current:
            current.append(line)
    if current:
        entries.append(current)
    return entries


def normalize_candidate(text: str) -> str:
    value = text.strip().strip("`").strip('"').strip("'")
    while value.endswith((",", ";")):
        value = value[:-1]
    return value


def extract_log(entry_lines: list[str]) -> tuple[str | None, str | None]:
    matches: list[tuple[int, int, str, str]] = []
    for line_idx, raw in enumerate(entry_lines):
        line = raw.strip()
        for pattern in LOG_PATTERNS:
            match = pattern.search(line)
            if not match:
                continue
            candidate = normalize_candidate(match.group(1))
            if candidate.endswith(".log"):
                priority = 0
                if "check_c_team_collect_preflight_report" in pattern.pattern:
                    priority = 3
                elif "C_COLLECT_PREFLIGHT_LOG" in pattern.pattern:
                    priority = 2
                elif "--collect-log-out" in pattern.pattern or "collect_log_out=" in pattern.pattern:
                    priority = 1
                matches.append((priority, line_idx, candidate, pattern.pattern))
    if not matches:
        return None, None
    matches.sort(key=lambda item: (item[0], item[1]))
    best = matches[-1]
    return best[2], best[3]


def main() -> int:
    args = parse_args()
    path = Path(args.team_status)
    lines = load_text(path).splitlines()
    c_section = find_c_section(lines)
    if not c_section:
        print("C_TEAM_LATEST_COLLECT_LOG")
        print(f"team_status={path}")
        print("reason=missing_c_section")
        print("verdict=FAIL")
        return 1

    entries = split_entries(c_section)
    if not entries:
        print("C_TEAM_LATEST_COLLECT_LOG")
        print(f"team_status={path}")
        print("reason=missing_c_entry")
        print("verdict=FAIL")
        return 1

    selected_title = ""
    collect_log: str | None = None
    source: str | None = None
    fallback_from_latest = False
    for idx in range(len(entries) - 1, -1, -1):
        title = entries[idx][0].strip()
        found_log, found_source = extract_log(entries[idx])
        if not found_log:
            continue
        selected_title = title
        collect_log = found_log
        source = found_source
        fallback_from_latest = idx != len(entries) - 1
        break

    if not collect_log:
        latest_title = entries[-1][0].strip()
        print("C_TEAM_LATEST_COLLECT_LOG")
        print(f"team_status={path}")
        print(f"entry={latest_title}")
        print("reason=collect_log_not_found")
        print("verdict=FAIL")
        return 1

    collect_log_exists = Path(collect_log).is_file()

    if args.require_existing and not collect_log_exists:
        if args.print_path_only:
            print("C_TEAM_LATEST_COLLECT_LOG")
            print(f"team_status={path}")
            print(f"entry={selected_title}")
            print(f"fallback_from_latest={'1' if fallback_from_latest else '0'}")
            print(f"collect_log={collect_log}")
            print(f"source_pattern={source}")
            print("reason=collect_log_missing")
            print("verdict=FAIL")
            return 1
        print("C_TEAM_LATEST_COLLECT_LOG")
        print(f"team_status={path}")
        print(f"entry={selected_title}")
        print(f"fallback_from_latest={'1' if fallback_from_latest else '0'}")
        print(f"collect_log={collect_log}")
        print(f"collect_log_exists=0")
        print(f"source_pattern={source}")
        print("reason=collect_log_missing")
        print("verdict=FAIL")
        return 1

    if args.print_path_only:
        print(collect_log)
        return 0

    print("C_TEAM_LATEST_COLLECT_LOG")
    print(f"team_status={path}")
    print(f"entry={selected_title}")
    print(f"fallback_from_latest={'1' if fallback_from_latest else '0'}")
    print(f"collect_log={collect_log}")
    print(f"collect_log_exists={'1' if collect_log_exists else '0'}")
    print(f"source_pattern={source}")
    print("verdict=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
