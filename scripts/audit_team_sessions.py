#!/usr/bin/env python3
"""Audit latest A/B/C team session compliance from docs/team_status.md.

Checks (default):
- SESSION_TIMER_START / SESSION_TIMER_END markers exist
- elapsed_min >= threshold (default: 30)
- no obvious artificial wait command (`sleep`) in the entry text
- practical evidence fields exist (changes / commands / pass-fail)
"""
from __future__ import annotations

import argparse
import json
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
START_EPOCH_RE = re.compile(r"start_epoch\s*[:=]\s*`?(\d+)`?")
END_BLOCK_RE = re.compile(
    r"SESSION_TIMER_END(?P<body>.*?)(?=SESSION_TIMER_START|SESSION_TIMER_GUARD|SESSION_TIMER_END|$)",
    re.DOTALL,
)
START_BLOCK_RE = re.compile(
    r"SESSION_TIMER_START(?P<body>.*?)(?=SESSION_TIMER_START|SESSION_TIMER_GUARD|SESSION_TIMER_END|$)",
    re.DOTALL,
)
SLEEP_RE = re.compile(r"(^|[`\s])sleep\s+\d+", re.IGNORECASE)
BACKTICK_BLOCK_RE = re.compile(r"`([^`\n]+)`")
TOP_FIELD_RE = re.compile(r"^\s{0,2}-\s")
RAW_PATH_RE = re.compile(r"(?P<path>(?:FEM4C|docs|scripts|chrono-2d|data|\.github)/[A-Za-z0-9_./-]+)")

# Path prefixes treated as implementation progress (not docs-only maintenance).
IMPL_PATH_PREFIXES = (
    "FEM4C/src/",
    "FEM4C/scripts/",
    "FEM4C/practice/ch09/",
    "chrono-2d/",
    "scripts/",
    ".github/workflows/",
)
IMPL_PATH_EXACT = (
    "FEM4C/Makefile",
    ".github/workflows/ci.yaml",
)


@dataclass
class EntryAudit:
    team: str
    title: str
    elapsed_min: int | None
    has_start: bool
    has_end: bool
    has_sleep: bool
    has_changes: bool
    has_impl_changes: bool
    has_commands: bool
    has_passfail: bool
    changed_paths: list[str]
    start_epoch: int | None

    def failure_reasons(
        self,
        min_elapsed: int,
        require_evidence: bool,
        require_impl_changes: bool,
    ) -> list[str]:
        reasons: list[str] = []
        if not self.has_start:
            reasons.append("missing SESSION_TIMER_START")
        if not self.has_end:
            reasons.append("missing SESSION_TIMER_END")
        if self.elapsed_min is None:
            reasons.append("missing elapsed_min")
        elif self.elapsed_min < min_elapsed:
            reasons.append(f"elapsed_min<{min_elapsed}")
        if self.has_sleep:
            reasons.append("artificial wait command detected")
        if require_evidence:
            if not self.has_changes:
                reasons.append("missing changes evidence")
            if require_impl_changes and not self.has_impl_changes:
                reasons.append("changes evidence does not include implementation files")
            if not self.has_commands:
                reasons.append("missing command evidence")
            if not self.has_passfail:
                reasons.append("missing pass/fail evidence")
        return reasons

    def verdict(self, min_elapsed: int, require_evidence: bool, require_impl_changes: bool) -> str:
        return (
            "PASS"
            if not self.failure_reasons(min_elapsed, require_evidence, require_impl_changes)
            else "FAIL"
        )

    def to_dict(
        self, min_elapsed: int, require_evidence: bool, require_impl_changes: bool
    ) -> dict[str, object]:
        return {
            "team": self.team,
            "entry": self.title,
            "start_epoch": self.start_epoch,
            "elapsed_min": self.elapsed_min,
            "has_timer_start": self.has_start,
            "has_timer_end": self.has_end,
            "has_sleep": self.has_sleep,
            "has_changes": self.has_changes,
            "has_impl_changes": self.has_impl_changes,
            "has_commands": self.has_commands,
            "has_passfail": self.has_passfail,
            "changed_paths": self.changed_paths,
            "verdict": self.verdict(min_elapsed, require_evidence, require_impl_changes),
            "reasons": self.failure_reasons(min_elapsed, require_evidence, require_impl_changes),
        }


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


def extract_elapsed_min(text: str) -> int | None:
    """Return elapsed_min using the latest SESSION_TIMER_END block.

    Some entries include intermediate SESSION_TIMER_GUARD outputs with
    elapsed_min below threshold before the final SESSION_TIMER_END value.
    For compliance, we must evaluate the final session end value.
    """
    end_blocks = list(END_BLOCK_RE.finditer(text))
    for block in reversed(end_blocks):
        match = ELAPSED_RE.search(block.group("body"))
        if match:
            return int(match.group(1))

    matches = list(ELAPSED_RE.finditer(text))
    if not matches:
        return None
    return int(matches[-1].group(1))


def extract_start_epoch(text: str) -> int | None:
    start_blocks = list(START_BLOCK_RE.finditer(text))
    for block in reversed(start_blocks):
        match = START_EPOCH_RE.search(block.group("body"))
        if match:
            return int(match.group(1))

    matches = list(START_EPOCH_RE.finditer(text))
    if not matches:
        return None
    return int(matches[-1].group(1))


def _looks_like_path(text: str) -> bool:
    token = text.strip()
    if " " in token:
        return False
    if "*" in token:
        return False
    if token.startswith("--"):
        return False
    if "/" in token:
        base = token.rsplit("/", 1)[-1]
        if not base:
            return False
        if base == "Makefile":
            return True
        if base.startswith("."):
            return True
        return "." in base
    return token.endswith((".c", ".h", ".py", ".sh", ".md", ".yaml", ".yml", "Makefile"))


def _normalize_path_token(token: str) -> str:
    return token.strip().strip(".,:;)]}")


def extract_changed_paths(lines: list[str]) -> list[str]:
    paths: list[str] = []
    seen: set[str] = set()
    in_change_block = False
    for line in lines:
        stripped = line.strip()

        if TOP_FIELD_RE.match(line):
            if any(marker in stripped for marker in ("変更ファイル", "判定した差分ファイル", "変更対象")):
                in_change_block = True
            elif in_change_block:
                in_change_block = False

        if not in_change_block:
            continue

        for match in BACKTICK_BLOCK_RE.finditer(line):
            token = _normalize_path_token(match.group(1).strip())
            if not _looks_like_path(token):
                continue
            if token in seen:
                continue
            seen.add(token)
            paths.append(token)
        for match in RAW_PATH_RE.finditer(line):
            token = _normalize_path_token(match.group("path"))
            if not _looks_like_path(token):
                continue
            if token in seen:
                continue
            seen.add(token)
            paths.append(token)
    return paths


def path_is_impl(path: str) -> bool:
    if path in IMPL_PATH_EXACT:
        return True
    return any(path.startswith(prefix) for prefix in IMPL_PATH_PREFIXES)


def audit_entry(team: str, lines: list[str]) -> EntryAudit:
    text = "\n".join(lines)
    title = lines[0].strip() if lines else "(no entry)"
    changes_markers = (
        "変更ファイル",
        "判定した差分ファイル",
        "変更対象ファイル",
        "変更対象",
    )
    has_changes = any(marker in text for marker in changes_markers)
    changed_paths = extract_changed_paths(lines)
    has_impl_changes = any(path_is_impl(path) for path in changed_paths)
    return EntryAudit(
        team=team,
        title=title,
        elapsed_min=extract_elapsed_min(text),
        has_start="SESSION_TIMER_START" in text,
        has_end="SESSION_TIMER_END" in text,
        has_sleep=bool(SLEEP_RE.search(text)),
        has_changes=has_changes,
        has_impl_changes=has_impl_changes,
        has_commands=("実行コマンド" in text or "1行再現コマンド" in text),
        has_passfail=("pass/fail" in text.lower() or "PASS" in text or "FAIL" in text),
        changed_paths=changed_paths,
        start_epoch=extract_start_epoch(text),
    )


def choose_latest(audits: list[EntryAudit]) -> EntryAudit:
    with_epoch = [a for a in audits if a.start_epoch is not None]
    if with_epoch:
        return max(with_epoch, key=lambda a: a.start_epoch if a.start_epoch is not None else -1)
    return audits[0]


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
                    has_impl_changes=False,
                    has_commands=False,
                    has_passfail=False,
                    changed_paths=[],
                    start_epoch=None,
                )
            )
            continue
        audited = [audit_entry(team, e) for e in entries]
        audits.append(choose_latest(audited))
    return audits


def print_report(
    audits: list[EntryAudit],
    min_elapsed: int,
    require_evidence: bool,
    require_impl_changes: bool,
) -> int:
    print(f"AUDIT_TARGET: latest entries (A/B/C)  threshold=elapsed_min>={min_elapsed}")
    print("-" * 112)
    print(
        "team  verdict  elapsed  timer  sleep  changes  impl  commands  pass/fail  start_epoch  entry"
    )
    print("-" * 112)
    failed = False
    for a in audits:
        verdict = a.verdict(min_elapsed, require_evidence, require_impl_changes)
        if verdict != "PASS":
            failed = True
        timer = "ok" if (a.has_start and a.has_end) else "missing"
        elapsed = "-" if a.elapsed_min is None else str(a.elapsed_min)
        start_epoch = "-" if a.start_epoch is None else str(a.start_epoch)
        print(
            f"{a.team:>4}  {verdict:<7}  {elapsed:>7}  {timer:<7}  "
            f"{str(a.has_sleep):<5}  {str(a.has_changes):<7}  "
            f"{str(a.has_impl_changes):<4}  {str(a.has_commands):<8}  {str(a.has_passfail):<9}  "
            f"{start_epoch:>11}  {a.title}"
        )
        reasons = a.failure_reasons(min_elapsed, require_evidence, require_impl_changes)
        if reasons:
            print(f"      reasons: {', '.join(reasons)}")
    print("-" * 112)
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
    parser.add_argument(
        "--json",
        action="store_true",
        help="Print machine-readable JSON report",
    )
    parser.add_argument(
        "--require-evidence",
        dest="require_evidence",
        action="store_true",
        default=True,
        help="Require changes/commands/pass-fail evidence (default: on)",
    )
    parser.add_argument(
        "--no-require-evidence",
        dest="require_evidence",
        action="store_false",
        help="Skip changes/commands/pass-fail evidence checks",
    )
    parser.add_argument(
        "--require-impl-changes",
        dest="require_impl_changes",
        action="store_true",
        default=False,
        help="Require at least one implementation file in changed paths (default: off)",
    )
    parser.add_argument(
        "--no-require-impl-changes",
        dest="require_impl_changes",
        action="store_false",
        help="Disable implementation-file requirement",
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
    if args.json:
        payload = {
            "threshold_min_elapsed": args.min_elapsed,
            "require_evidence": args.require_evidence,
            "require_impl_changes": args.require_impl_changes,
            "results": [
                a.to_dict(args.min_elapsed, args.require_evidence, args.require_impl_changes)
                for a in audits
            ],
        }
        payload["summary"] = {
            "all_pass": all(r["verdict"] == "PASS" for r in payload["results"]),
            "failed_teams": [r["team"] for r in payload["results"] if r["verdict"] != "PASS"],
        }
        print(json.dumps(payload, ensure_ascii=False, indent=2))
        return 0 if payload["summary"]["all_pass"] else 1
    return print_report(audits, args.min_elapsed, args.require_evidence, args.require_impl_changes)


if __name__ == "__main__":
    raise SystemExit(main())
