#!/usr/bin/env python3
"""Audit latest A/B/C team session compliance from docs/team_status.md.

Checks (default):
- SESSION_TIMER_START / SESSION_TIMER_END markers exist
- elapsed_min >= threshold (default: 30)
- no obvious artificial wait command (`sleep`) in the entry text
"""
from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

TEAM_HEADINGS = {
    "A": "## Aチーム",
    "B": "## Bチーム",
    "C": "## Cチーム",
}

ENTRY_START_RE = re.compile(r"^- 実行タスク")
ELAPSED_RE = re.compile(r"elapsed_min\s*[:=]\s*`?(\d+)`?")
SLEEP_RE = re.compile(r"(^|[`\\s])sleep\\s+\\d+", re.IGNORECASE)


@dataclass
class EntryAudit:
    team: str
    title: str
    elapsed_min: int | None
    has_start: bool
    has_end: bool
    has_sleep: bool
    has_changes: bool
    has_commands: bool
    has_passfail: bool

    def verdict(self, min_elapsed: int) -> str:
        if not self.has_start or not self.has_end:
            return "FAIL"
        if self.elapsed_min is None:
            return "FAIL"
        if self.elapsed_min < min_elapsed:
            return "FAIL"
        if self.has_sleep:
            return "FAIL"
        return "PASS"


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except OSError as exc:
        raise SystemExit(f"ERROR: failed to read {path}: {exc}") from exc


def section_slice(lines: list[str], heading: str) -> list[str]:
    start = None
    for i, line in enumerate(lines):
        if line.strip() == heading:
            start = i + 1
            break
    if start is None:
        return []

    for j in range(start, len(lines)):
        if lines[j].startswith("## "):
            return lines[start:j]
    return lines[start:]


def split_entries(section_lines: list[str]) -> list[list[str]]:
    entries: list[list[str]] = []
    current: list[str] = []
    for line in section_lines:
        if ENTRY_START_RE.match(line):
            if current:
                entries.append(current)
                current = []
        if current or ENTRY_START_RE.match(line):
            current.append(line)
    if current:
        entries.append(current)
    return entries


def first_elapsed(text: str) -> int | None:
    match = ELAPSED_RE.search(text)
    if not match:
        return None
    return int(match.group(1))


def audit_entry(team: str, lines: list[str]) -> EntryAudit:
    text = "\n".join(lines)
    title = lines[0].strip() if lines else "(no entry)"
    return EntryAudit(
        team=team,
        title=title,
        elapsed_min=first_elapsed(text),
        has_start="SESSION_TIMER_START" in text,
        has_end="SESSION_TIMER_END" in text,
        has_sleep=bool(SLEEP_RE.search(text)),
        has_changes=("変更ファイル" in text),
        has_commands=("実行コマンド" in text or "1行再現コマンド" in text),
        has_passfail=("pass/fail" in text.lower() or "PASS" in text or "FAIL" in text),
    )


def collect_latest_audits(markdown: str, teams: Iterable[str]) -> list[EntryAudit]:
    lines = markdown.splitlines()
    audits: list[EntryAudit] = []
    for team in teams:
        heading = TEAM_HEADINGS[team]
        section = section_slice(lines, heading)
        entries = split_entries(section)
        if not entries:
            audits.append(
                EntryAudit(
                    team=team,
                    title="(entry not found)",
                    elapsed_min=None,
                    has_start=False,
                    has_end=False,
                    has_sleep=False,
                    has_changes=False,
                    has_commands=False,
                    has_passfail=False,
                )
            )
            continue
        audits.append(audit_entry(team, entries[0]))
    return audits


def print_report(audits: list[EntryAudit], min_elapsed: int) -> int:
    print(f"AUDIT_TARGET: latest entries (A/B/C)  threshold=elapsed_min>={min_elapsed}")
    print("-" * 92)
    print("team  verdict  elapsed  timer  sleep  changes  commands  pass/fail  entry")
    print("-" * 92)
    failed = False
    for a in audits:
        verdict = a.verdict(min_elapsed)
        if verdict != "PASS":
            failed = True
        timer = "ok" if (a.has_start and a.has_end) else "missing"
        elapsed = "-" if a.elapsed_min is None else str(a.elapsed_min)
        print(
            f"{a.team:>4}  {verdict:<7}  {elapsed:>7}  {timer:<7}  "
            f"{str(a.has_sleep):<5}  {str(a.has_changes):<7}  "
            f"{str(a.has_commands):<8}  {str(a.has_passfail):<9}  {a.title}"
        )
    print("-" * 92)
    if failed:
        print("RESULT: FAIL (at least one team entry does not satisfy compliance)")
        return 1
    print("RESULT: PASS (all latest team entries satisfy compliance)")
    return 0


def parse_args(argv: list[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Audit A/B/C session compliance from team_status.md")
    parser.add_argument(
        "--team-status",
        default="docs/team_status.md",
        help="Path to team status markdown (default: docs/team_status.md)",
    )
    parser.add_argument(
        "--min-elapsed",
        type=int,
        default=30,
        help="Minimum elapsed_min threshold (default: 30)",
    )
    parser.add_argument(
        "--teams",
        default="A,B,C",
        help="Comma-separated teams to audit (subset of A,B,C). default: A,B,C",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    team_status = Path(args.team_status)
    markdown = read_text(team_status)
    teams = [t.strip().upper() for t in args.teams.split(",") if t.strip()]
    bad = [t for t in teams if t not in TEAM_HEADINGS]
    if bad:
        print(f"ERROR: unknown team(s): {', '.join(bad)}", file=sys.stderr)
        return 2
    audits = collect_latest_audits(markdown, teams)
    return print_report(audits, args.min_elapsed)


if __name__ == "__main__":
    raise SystemExit(main())
