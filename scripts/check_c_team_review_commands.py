#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

SECTION_HEADING = "## Cチーム"
ENTRY_START_RE = re.compile(r"^- 実行タスク")
START_EPOCH_RE = re.compile(r"start_epoch\s*[:=]\s*`?(\d+)`?")


def reason_to_code(reason: str) -> str:
    return re.sub(r"(^_+|_+$)", "", re.sub(r"[^a-z0-9]+", "_", reason.lower()))


def parse_args(argv: list[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Check latest C-team entry has missing-log review commands"
    )
    parser.add_argument("--team-status", default="docs/team_status.md")
    parser.add_argument(
        "--require-collect-report-if-check-command",
        action="store_true",
        default=True,
        help="Require collect_report_review_command when check_c_team_collect_preflight_report.py appears (default: true)",
    )
    parser.add_argument(
        "--no-require-collect-report-if-check-command",
        dest="require_collect_report_if_check_command",
        action="store_false",
        help="Disable collect_report_review_command requirement",
    )
    parser.add_argument("--json", action="store_true")
    return parser.parse_args(argv)


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


def parse_start_epoch(text: str) -> int:
    match = START_EPOCH_RE.search(text)
    if not match:
        return -1
    return int(match.group(1))


def select_latest_entry(entries: list[list[str]]) -> tuple[str, str, int]:
    c_prefixed: list[tuple[str, str, int]] = []
    for entry in entries:
        text = "\n".join(entry)
        title = entry[0].strip() if entry else ""
        if "C-" not in title:
            continue
        c_prefixed.append((title, text, parse_start_epoch(text)))
    candidates = c_prefixed if c_prefixed else [
        (entry[0].strip(), "\n".join(entry), parse_start_epoch("\n".join(entry)))
        for entry in entries
        if entry
    ]
    if not candidates:
        return ("", "", -1)
    return max(candidates, key=lambda item: item[2])


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    text = Path(args.team_status).read_text(encoding="utf-8")
    lines = text.splitlines()
    section = section_slice(lines, SECTION_HEADING)
    entries = split_entries(section)
    title, entry_text, start_epoch = select_latest_entry(entries)
    reasons: list[str] = []

    if not entry_text:
        reasons.append("no_c_entry_found")
    has_missing_log_review_command = "missing_log_review_command=" in entry_text
    has_collect_report_review_command = "collect_report_review_command=" in entry_text
    has_collect_check_command = (
        "python scripts/check_c_team_collect_preflight_report.py " in entry_text
    )

    if entry_text and not has_missing_log_review_command:
        reasons.append("missing missing_log_review_command")
    if (
        entry_text
        and args.require_collect_report_if_check_command
        and has_collect_check_command
        and not has_collect_report_review_command
    ):
        reasons.append("missing collect_report_review_command")

    reason_codes = [reason_to_code(reason) for reason in reasons]
    verdict = "PASS" if not reasons else "FAIL"
    report = {
        "entry": title,
        "start_epoch": start_epoch if start_epoch >= 0 else None,
        "has_missing_log_review_command": has_missing_log_review_command,
        "has_collect_report_review_command": has_collect_report_review_command,
        "has_collect_check_command": has_collect_check_command,
        "require_collect_report_if_check_command": args.require_collect_report_if_check_command,
        "verdict": verdict,
        "reasons": reasons,
        "reason_codes": reason_codes,
    }

    if args.json:
        json.dump(report, sys.stdout, ensure_ascii=False, indent=2)
        sys.stdout.write("\n")
    else:
        print("C_TEAM_REVIEW_COMMAND_AUDIT")
        print(f"entry={report['entry']}")
        print(f"start_epoch={report['start_epoch']}")
        print(
            f"has_missing_log_review_command={'yes' if has_missing_log_review_command else 'no'}"
        )
        print(
            f"has_collect_report_review_command={'yes' if has_collect_report_review_command else 'no'}"
        )
        print(f"has_collect_check_command={'yes' if has_collect_check_command else 'no'}")
        print(
            "require_collect_report_if_check_command="
            + ("yes" if args.require_collect_report_if_check_command else "no")
        )
        print(f"verdict={verdict}")
        if reasons:
            print("reasons=" + "; ".join(reasons))
            print("reason_codes=" + "; ".join(reason_codes))
        else:
            print("reasons=-")
            print("reason_codes=-")
    return 0 if verdict == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
