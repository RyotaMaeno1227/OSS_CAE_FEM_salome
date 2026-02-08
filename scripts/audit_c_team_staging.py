#!/usr/bin/env python3
"""Audit latest C-team report for staging dry-run compliance."""
from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path

SECTION_HEADING = "## Cチーム"
ENTRY_START_RE = re.compile(r"^- 実行タスク")
START_EPOCH_RE = re.compile(r"start_epoch\s*[:=]\s*`?(\d+)`?")
DRYRUN_RESULT_RE = re.compile(r"dryrun_result\s*[:=]\s*([A-Za-z_\-]+)", re.IGNORECASE)


@dataclass
class CEntryAudit:
    title: str
    start_epoch: int | None
    has_dryrun_result: bool
    dryrun_result_values: list[str]
    has_dryrun_command: bool

    def reasons(self, require_pass: bool = False, require_both: bool = False) -> list[str]:
        issues: list[str] = []
        if not self.has_dryrun_result:
            issues.append("missing dryrun_result")
        if require_pass and "pass" not in self.dryrun_result_values:
            issues.append("missing dryrun_result=pass")
        if require_both:
            if "pass" not in self.dryrun_result_values:
                issues.append("missing dryrun_result=pass")
            if "fail" not in self.dryrun_result_values:
                issues.append("missing dryrun_result=fail")
        if not self.has_dryrun_command:
            issues.append("missing c_stage_dryrun command evidence")
        return issues

    def verdict(self, require_pass: bool = False, require_both: bool = False) -> str:
        return "PASS" if not self.reasons(require_pass=require_pass, require_both=require_both) else "FAIL"

    def to_dict(self) -> dict[str, object]:
        return {
            "entry": self.title,
            "start_epoch": self.start_epoch,
            "has_dryrun_result": self.has_dryrun_result,
            "dryrun_results": self.dryrun_result_values,
            "has_c_stage_dryrun_command": self.has_dryrun_command,
            "verdict": self.verdict(),
            "reasons": self.reasons(),
        }


def parse_args(argv: list[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Audit latest C-team report for staging dry-run compliance"
    )
    parser.add_argument("--team-status", default="docs/team_status.md")
    parser.add_argument(
        "--team-prefix-only",
        action="store_true",
        default=True,
        help="Only consider entries whose title contains C- (default: true)",
    )
    parser.add_argument(
        "--allow-non-prefix",
        dest="team_prefix_only",
        action="store_false",
        help="Allow non C- entries in C section when choosing latest",
    )
    parser.add_argument("--json", action="store_true", help="Emit JSON")
    parser.add_argument(
        "--require-pass",
        action="store_true",
        help="Require dryrun_result=pass in the latest C-team entry",
    )
    parser.add_argument(
        "--require-both",
        action="store_true",
        help="Require both dryrun_result=pass and dryrun_result=fail evidence",
    )
    return parser.parse_args(argv)


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


def parse_start_epoch(text: str) -> int | None:
    match = START_EPOCH_RE.search(text)
    if not match:
        return None
    return int(match.group(1))


def parse_dryrun_results(text: str) -> list[str]:
    values: list[str] = []
    for match in DRYRUN_RESULT_RE.finditer(text):
        value = match.group(1).lower()
        if value not in values:
            values.append(value)
    return values


def audit_entry(lines: list[str]) -> CEntryAudit:
    text = "\n".join(lines)
    dryrun_results = parse_dryrun_results(text)
    return CEntryAudit(
        title=lines[0].strip() if lines else "(no-entry)",
        start_epoch=parse_start_epoch(text),
        has_dryrun_result=bool(dryrun_results),
        dryrun_result_values=dryrun_results,
        has_dryrun_command=("scripts/c_stage_dryrun.sh" in text),
    )


def choose_latest(entries: list[CEntryAudit]) -> CEntryAudit:
    with_epoch = [e for e in entries if e.start_epoch is not None]
    if with_epoch:
        return max(with_epoch, key=lambda e: e.start_epoch if e.start_epoch is not None else -1)
    return entries[0]


def title_is_c_task(title: str) -> bool:
    return "C-" in title and "PM-" not in title


def collect_latest(markdown: str, team_prefix_only: bool) -> CEntryAudit:
    lines = markdown.splitlines()
    section = section_slice(lines, SECTION_HEADING)
    raw_entries = split_entries(section)
    if not raw_entries:
        return CEntryAudit(
            title="(entry not found)",
            start_epoch=None,
            has_dryrun_result=False,
            dryrun_result_values=[],
            has_dryrun_command=False,
        )
    audited = [audit_entry(e) for e in raw_entries]
    if team_prefix_only:
        filtered = [e for e in audited if title_is_c_task(e.title)]
        if filtered:
            audited = filtered
    return choose_latest(audited)


def print_report(audit: CEntryAudit, require_pass: bool, require_both: bool) -> int:
    print("AUDIT_TARGET: latest C-team entry for staging dry-run compliance")
    print("-" * 96)
    dryrun_value = ",".join(audit.dryrun_result_values) if audit.dryrun_result_values else "-"
    start_epoch = audit.start_epoch if audit.start_epoch is not None else "-"
    print(f"entry: {audit.title}")
    print(f"start_epoch: {start_epoch}")
    print(f"dryrun_results: {dryrun_value}")
    print(f"has_c_stage_dryrun_command: {audit.has_dryrun_command}")
    print(f"require_pass: {require_pass}")
    print(f"require_both: {require_both}")
    verdict = audit.verdict(require_pass=require_pass, require_both=require_both)
    print(f"verdict: {verdict}")
    reasons = audit.reasons(require_pass=require_pass, require_both=require_both)
    if reasons:
        print(f"reasons: {', '.join(reasons)}")
        print("RESULT: FAIL")
        return 1
    print("RESULT: PASS")
    return 0


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    markdown = read_text(Path(args.team_status))
    audit = collect_latest(markdown, args.team_prefix_only)
    if args.json:
        payload = audit.to_dict()
        payload["team"] = "C"
        payload["require_pass"] = args.require_pass
        payload["require_both"] = args.require_both
        payload["verdict"] = audit.verdict(
            require_pass=args.require_pass,
            require_both=args.require_both,
        )
        payload["reasons"] = audit.reasons(
            require_pass=args.require_pass,
            require_both=args.require_both,
        )
        print(json.dumps(payload, ensure_ascii=False, indent=2))
        return 0 if payload["verdict"] == "PASS" else 1
    return print_report(audit, require_pass=args.require_pass, require_both=args.require_both)


if __name__ == "__main__":
    raise SystemExit(main())
