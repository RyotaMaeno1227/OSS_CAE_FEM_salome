#!/usr/bin/env python3
"""Summarize A/B/C session compliance history from docs/team_status.md."""
from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass
from pathlib import Path

TEAM_HEADINGS = {
    "A": "## Aチーム",
    "B": "## Bチーム",
    "C": "## Cチーム",
}

ENTRY_START_RE = re.compile(r"^- 実行タスク")
ELAPSED_RE = re.compile(r"elapsed_min\s*[:=]\s*`?(\d+)`?")
START_EPOCH_RE = re.compile(r"start_epoch\s*[:=]\s*`?(\d+)`?")
SLEEP_RE = re.compile(r"(^|[`\s])sleep\s+\d+", re.IGNORECASE)


@dataclass
class Entry:
    team: str
    title: str
    elapsed_min: int | None
    start_epoch: int | None
    has_timer: bool
    has_sleep: bool
    has_changes: bool
    has_commands: bool
    has_passfail: bool
    line_hint: int

    def compliance(self, min_elapsed: int, require_evidence: bool) -> tuple[bool, list[str]]:
        reasons: list[str] = []
        if not self.has_timer:
            reasons.append("missing_timer")
        if self.elapsed_min is None:
            reasons.append("missing_elapsed")
        elif self.elapsed_min < min_elapsed:
            reasons.append("short_elapsed")
        if self.has_sleep:
            reasons.append("artificial_wait")
        if require_evidence:
            if not self.has_changes:
                reasons.append("missing_changes")
            if not self.has_commands:
                reasons.append("missing_commands")
            if not self.has_passfail:
                reasons.append("missing_passfail")
        return (not reasons, reasons)


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def section_slice(lines: list[str], heading: str) -> tuple[int, list[str]]:
    start = None
    for i, line in enumerate(lines):
        if line.strip() == heading:
            start = i + 1
            break
    if start is None:
        return (-1, [])
    for j in range(start, len(lines)):
        if lines[j].startswith("## "):
            return (start, lines[start:j])
    return (start, lines[start:])


def split_entries_with_line_no(section_start: int, section_lines: list[str]) -> list[tuple[int, list[str]]]:
    entries: list[tuple[int, list[str]]] = []
    current: list[str] = []
    current_line = -1
    for offset, line in enumerate(section_lines):
        if ENTRY_START_RE.match(line):
            if current:
                entries.append((current_line, current))
                current = []
            current_line = section_start + offset + 1
        if current or ENTRY_START_RE.match(line):
            current.append(line)
    if current:
        entries.append((current_line, current))
    return entries


def first_int(pattern: re.Pattern[str], text: str) -> int | None:
    m = pattern.search(text)
    if not m:
        return None
    return int(m.group(1))


def parse_entry(team: str, line_no: int, lines: list[str]) -> Entry:
    text = "\n".join(lines)
    return Entry(
        team=team,
        title=(lines[0].strip() if lines else "(no-title)"),
        elapsed_min=first_int(ELAPSED_RE, text),
        start_epoch=first_int(START_EPOCH_RE, text),
        has_timer=("SESSION_TIMER_START" in text and "SESSION_TIMER_END" in text),
        has_sleep=bool(SLEEP_RE.search(text)),
        has_changes=any(x in text for x in ("変更ファイル", "判定した差分ファイル", "変更対象ファイル", "変更対象")),
        has_commands=("実行コマンド" in text or "1行再現コマンド" in text),
        has_passfail=("pass/fail" in text.lower() or "PASS" in text or "FAIL" in text),
        line_hint=line_no,
    )


def title_matches_team(team: str, title: str) -> bool:
    markers = {
        "A": ("A-", "Aチーム"),
        "B": ("B-", "Bチーム"),
        "C": ("C-", "Cチーム"),
    }
    if "PM-" in title:
        return False
    return any(m in title for m in markers.get(team, ()))


def collect_entries(markdown: str, teams: list[str], team_prefix_only: bool) -> list[Entry]:
    lines = markdown.splitlines()
    out: list[Entry] = []
    for team in teams:
        section_start, section = section_slice(lines, TEAM_HEADINGS[team])
        if section_start < 0:
            continue
        for line_no, entry_lines in split_entries_with_line_no(section_start, section):
            entry = parse_entry(team, line_no, entry_lines)
            if team_prefix_only and not title_matches_team(team, entry.title):
                continue
            out.append(entry)
    return out


def summarize(entries: list[Entry], min_elapsed: int, require_evidence: bool) -> dict[str, object]:
    by_team: dict[str, list[Entry]] = {k: [] for k in TEAM_HEADINGS}
    for e in entries:
        by_team[e.team].append(e)

    report: dict[str, object] = {
        "threshold_min_elapsed": min_elapsed,
        "require_evidence": require_evidence,
        "teams": {},
    }

    for team, team_entries in by_team.items():
        total = len(team_entries)
        pass_count = 0
        fail_count = 0
        short_count = 0
        timer_missing = 0
        sleep_count = 0
        elapsed_values: list[int] = []
        recent_failures: list[dict[str, object]] = []

        # newest first (line_hint desc)
        ordered = sorted(team_entries, key=lambda x: x.line_hint, reverse=True)
        for e in ordered:
            ok, reasons = e.compliance(min_elapsed, require_evidence)
            if e.elapsed_min is not None:
                elapsed_values.append(e.elapsed_min)
            if ok:
                pass_count += 1
            else:
                fail_count += 1
                if len(recent_failures) < 5:
                    recent_failures.append(
                        {
                            "line": e.line_hint,
                            "title": e.title,
                            "elapsed_min": e.elapsed_min,
                            "reasons": reasons,
                        }
                    )
            if "short_elapsed" in reasons:
                short_count += 1
            if "missing_timer" in reasons:
                timer_missing += 1
            if "artificial_wait" in reasons:
                sleep_count += 1

        avg_elapsed = (sum(elapsed_values) / len(elapsed_values)) if elapsed_values else None
        pass_rate = (pass_count / total) if total > 0 else None
        report["teams"][team] = {
            "total_entries": total,
            "pass_count": pass_count,
            "fail_count": fail_count,
            "pass_rate": pass_rate,
            "short_elapsed_count": short_count,
            "missing_timer_count": timer_missing,
            "artificial_wait_count": sleep_count,
            "avg_elapsed_min": avg_elapsed,
            "recent_failures": recent_failures,
        }

    return report


def print_text(report: dict[str, object]) -> int:
    min_elapsed = report["threshold_min_elapsed"]
    req = report["require_evidence"]
    teams = report["teams"]
    print(f"HISTORY_AUDIT threshold=elapsed_min>={min_elapsed} require_evidence={req}")
    print("-" * 108)
    print("team  total  pass  fail  pass_rate  short(<th)  missing_timer  sleep  avg_elapsed")
    print("-" * 108)
    for team in ("A", "B", "C"):
        item = teams.get(team, {})
        if not isinstance(item, dict):
            continue
        total = item.get("total_entries", 0)
        p = item.get("pass_count", 0)
        f = item.get("fail_count", 0)
        r = item.get("pass_rate")
        sr = "-" if r is None else f"{float(r)*100:.1f}%"
        sh = item.get("short_elapsed_count", 0)
        mt = item.get("missing_timer_count", 0)
        sl = item.get("artificial_wait_count", 0)
        avg = item.get("avg_elapsed_min")
        avg_text = "-" if avg is None else f"{float(avg):.1f}"
        print(f"{team:>4}  {int(total):>5}  {int(p):>4}  {int(f):>4}  {sr:>8}  {int(sh):>10}  {int(mt):>13}  {int(sl):>5}  {avg_text:>10}")
    print("-" * 108)
    print("recent_failures (up to 5 per team):")
    for team in ("A", "B", "C"):
        item = teams.get(team, {})
        if not isinstance(item, dict):
            continue
        failures = item.get("recent_failures", [])
        if not isinstance(failures, list):
            continue
        print(f"[{team}]")
        if not failures:
            print("  - none")
            continue
        for entry in failures:
            if not isinstance(entry, dict):
                continue
            line = entry.get("line", "-")
            title = entry.get("title", "-")
            elapsed = entry.get("elapsed_min", "-")
            reasons = entry.get("reasons", [])
            reasons_text = ",".join(str(x) for x in reasons) if isinstance(reasons, list) else str(reasons)
            print(f"  - line={line} elapsed={elapsed} reasons={reasons_text} title={title}")
    return 0


def parse_args(argv: list[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Audit historical team session compliance from team_status.md")
    parser.add_argument("--team-status", default="docs/team_status.md")
    parser.add_argument("--min-elapsed", type=int, default=30)
    parser.add_argument("--teams", default="A,B,C")
    parser.add_argument("--json", action="store_true", help="Emit JSON")
    parser.add_argument("--require-evidence", dest="require_evidence", action="store_true", default=True)
    parser.add_argument("--no-require-evidence", dest="require_evidence", action="store_false")
    parser.add_argument(
        "--team-prefix-only",
        dest="team_prefix_only",
        action="store_true",
        default=True,
        help="Count only entries whose title matches team marker (default: on)",
    )
    parser.add_argument(
        "--no-team-prefix-only",
        dest="team_prefix_only",
        action="store_false",
        help="Count all entries under each team section",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    teams = [t.strip().upper() for t in args.teams.split(",") if t.strip()]
    markdown = read_text(Path(args.team_status))
    entries = collect_entries(markdown, teams, args.team_prefix_only)
    report = summarize(entries, args.min_elapsed, args.require_evidence)
    if args.json:
        print(json.dumps(report, ensure_ascii=False, indent=2))
        return 0
    return print_text(report)


if __name__ == "__main__":
    raise SystemExit(main())
