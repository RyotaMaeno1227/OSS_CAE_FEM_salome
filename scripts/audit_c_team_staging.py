#!/usr/bin/env python3
"""Audit latest C-team report for staging dry-run compliance."""
from __future__ import annotations

import argparse
import fnmatch
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path

SECTION_HEADING = "## Cチーム"
ENTRY_START_RE = re.compile(r"^- 実行タスク")
START_EPOCH_RE = re.compile(r"start_epoch\s*[:=]\s*`?(\d+)`?")
END_EPOCH_RE = re.compile(r"end_epoch\s*[:=]\s*`?(\d+)`?")
ELAPSED_MIN_RE = re.compile(r"elapsed_min\s*[:=]\s*`?(\d+)`?")
END_BLOCK_RE = re.compile(
    r"SESSION_TIMER_END(?P<body>.*?)(?=SESSION_TIMER_START|SESSION_TIMER_GUARD|SESSION_TIMER_END|$)",
    re.DOTALL,
)
DRYRUN_RESULT_RE = re.compile(r"dryrun_result\s*[:=]\s*([A-Za-z_\-]+)", re.IGNORECASE)
PATH_RE = re.compile(r"(?:FEM4C|docs|scripts|chrono-2d|oldFile)/[A-Za-z0-9_.\-/]+")
SAFE_STAGE_GIT_ADD_RE = re.compile(r"^git\s+add(?:\s+--)?\s+\S+")
TEMPLATE_PLACEHOLDER_RE = re.compile(r"<[^>\n]{1,120}>")

DEFAULT_COUPLED_FREEZE_FILE = Path(__file__).with_name(
    "c_coupled_freeze_forbidden_paths.txt"
)

FALLBACK_COUPLED_FREEZE_PATTERNS = (
    "FEM4C/src/analysis/runner.c",
    "FEM4C/src/analysis/runner.h",
    "FEM4C/scripts/check_coupled_stub_contract.sh",
    "FEM4C/src/fem4c.c",
)


@dataclass
class CEntryAudit:
    title: str
    start_epoch: int | None
    end_epoch: int | None
    elapsed_min: int | None
    has_dryrun_result: bool
    dryrun_result_values: list[str]
    has_dryrun_command: bool
    has_safe_stage_command: bool
    has_safe_stage_git_add: bool
    safe_stage_command: str | None
    has_pending_placeholder: bool
    has_token_missing_marker: bool
    has_template_placeholder: bool
    template_placeholders: list[str]
    detected_paths: list[str]
    source: str = "c_section"

    def reasons(
        self,
        require_pass: bool = False,
        require_both: bool = False,
        require_c_section: bool = False,
        require_coupled_freeze: bool = False,
        require_complete_timer: bool = False,
        require_safe_stage_command: bool = False,
        require_no_template_placeholder: bool = False,
        forbid_patterns: list[str] | None = None,
        coupled_freeze_patterns: list[str] | None = None,
    ) -> list[str]:
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
        if require_safe_stage_command and not self.has_safe_stage_command:
            issues.append("missing safe_stage_command")
        if require_safe_stage_command and self.has_safe_stage_command and not self.has_safe_stage_git_add:
            issues.append("safe_stage_command_not_git_add")
        if require_c_section and self.source != "c_section":
            issues.append("latest C entry is outside ## Cチーム section")
        if require_complete_timer:
            if self.end_epoch is None:
                issues.append("missing end_epoch")
            if self.elapsed_min is None:
                issues.append("missing elapsed_min")
            if self.has_pending_placeholder:
                issues.append("pending_placeholder_detected")
            if self.has_token_missing_marker:
                issues.append("token_missing_marker_detected")
        if require_no_template_placeholder and self.has_template_placeholder:
            issues.append("template_placeholder_detected")
        patterns = list(forbid_patterns or [])
        if require_coupled_freeze:
            patterns.extend(coupled_freeze_patterns or FALLBACK_COUPLED_FREEZE_PATTERNS)
        forbidden_hits = find_forbidden_paths(self.detected_paths, patterns)
        if forbidden_hits:
            issues.append(f"forbidden_paths_detected: {','.join(forbidden_hits)}")
        return issues

    def verdict(
        self,
        require_pass: bool = False,
        require_both: bool = False,
        require_c_section: bool = False,
        require_coupled_freeze: bool = False,
        require_complete_timer: bool = False,
        require_safe_stage_command: bool = False,
        require_no_template_placeholder: bool = False,
        forbid_patterns: list[str] | None = None,
        coupled_freeze_patterns: list[str] | None = None,
    ) -> str:
        return (
            "PASS"
            if not self.reasons(
                require_pass=require_pass,
                require_both=require_both,
                require_c_section=require_c_section,
                require_coupled_freeze=require_coupled_freeze,
                require_complete_timer=require_complete_timer,
                require_safe_stage_command=require_safe_stage_command,
                require_no_template_placeholder=require_no_template_placeholder,
                forbid_patterns=forbid_patterns,
                coupled_freeze_patterns=coupled_freeze_patterns,
            )
            else "FAIL"
        )

    def to_dict(self) -> dict[str, object]:
        return {
            "entry": self.title,
            "start_epoch": self.start_epoch,
            "end_epoch": self.end_epoch,
            "elapsed_min": self.elapsed_min,
            "has_dryrun_result": self.has_dryrun_result,
            "dryrun_results": self.dryrun_result_values,
            "has_c_stage_dryrun_command": self.has_dryrun_command,
            "has_safe_stage_command": self.has_safe_stage_command,
            "has_safe_stage_git_add": self.has_safe_stage_git_add,
            "safe_stage_command": self.safe_stage_command,
            "has_pending_placeholder": self.has_pending_placeholder,
            "has_token_missing_marker": self.has_token_missing_marker,
            "has_template_placeholder": self.has_template_placeholder,
            "template_placeholders": self.template_placeholders,
            "detected_paths": self.detected_paths,
            "source": self.source,
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
    parser.add_argument(
        "--global-fallback",
        action="store_true",
        default=True,
        help="Also scan full markdown for C-task entries (default: true)",
    )
    parser.add_argument(
        "--no-global-fallback",
        dest="global_fallback",
        action="store_false",
        help="Only use entries under ## Cチーム section",
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
    parser.add_argument(
        "--require-c-section",
        action="store_true",
        help="Fail if latest C entry is not under ## Cチーム section",
    )
    parser.add_argument(
        "--forbid-path",
        action="append",
        default=[],
        help="Forbidden path pattern (supports shell-style wildcards)",
    )
    parser.add_argument(
        "--require-coupled-freeze",
        action="store_true",
        help="Require no touched paths in coupled-freeze default patterns",
    )
    parser.add_argument(
        "--require-complete-timer",
        action="store_true",
        help="Require end_epoch and elapsed_min in the latest C entry",
    )
    parser.add_argument(
        "--require-safe-stage-command",
        action="store_true",
        help="Require safe_stage_command evidence in the latest C entry",
    )
    parser.add_argument(
        "--require-no-template-placeholder",
        action="store_true",
        help="Fail when template placeholder tokens (e.g. <記入>) remain in the latest C entry",
    )
    parser.add_argument(
        "--coupled-freeze-file",
        default=str(DEFAULT_COUPLED_FREEZE_FILE),
        help="Path list for coupled-freeze forbidden patterns (one pattern per line)",
    )
    parser.add_argument(
        "--print-coupled-freeze-patterns",
        action="store_true",
        help="Print coupled-freeze patterns loaded from file and exit",
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


def split_entries_global(lines: list[str]) -> list[list[str]]:
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
            if line.startswith("## "):
                entries.append(current)
                current = []
                continue
            if ENTRY_START_RE.match(line):
                entries.append(current)
                current = [line]
                continue
            current.append(line)
    if current:
        entries.append(current)
    return entries


def parse_start_epoch(text: str) -> int | None:
    match = START_EPOCH_RE.search(text)
    if not match:
        return None
    return int(match.group(1))


def _latest_complete_end_block_match(text: str) -> tuple[re.Match[str], re.Match[str]] | None:
    end_blocks = list(END_BLOCK_RE.finditer(text))
    for block in reversed(end_blocks):
        body = block.group("body")
        end_match = END_EPOCH_RE.search(body)
        elapsed_match = ELAPSED_MIN_RE.search(body)
        if end_match and elapsed_match:
            return end_match, elapsed_match
    return None


def parse_end_epoch(text: str) -> int | None:
    complete = _latest_complete_end_block_match(text)
    if complete is not None:
        end_match, _ = complete
        return int(end_match.group(1))
    matches = END_EPOCH_RE.findall(text)
    if not matches:
        return None
    # Prefer the latest timer completion evidence when multiple values exist.
    return int(matches[-1])


def parse_elapsed_min(text: str) -> int | None:
    complete = _latest_complete_end_block_match(text)
    if complete is not None:
        _, elapsed_match = complete
        return int(elapsed_match.group(1))
    matches = ELAPSED_MIN_RE.findall(text)
    if not matches:
        return None
    # Prefer the latest elapsed value (e.g. final guard/end block).
    return int(matches[-1])


def parse_dryrun_results(text: str) -> list[str]:
    values: list[str] = []
    for match in DRYRUN_RESULT_RE.finditer(text):
        value = match.group(1).lower()
        if value not in values:
            values.append(value)
    return values


def parse_safe_stage_command(text: str) -> str | None:
    pattern = re.compile(r"^\s*(?:[-*]\s*)?`?safe_stage_command\s*=\s*(.+?)`?\s*$")
    for line in text.splitlines():
        match = pattern.match(line)
        if match:
            return match.group(1).strip()
    return None


def parse_template_placeholders(text: str) -> list[str]:
    placeholders: list[str] = []
    for match in TEMPLATE_PLACEHOLDER_RE.finditer(text):
        token = match.group(0)
        if token not in placeholders:
            placeholders.append(token)
    return placeholders


def parse_paths(text: str) -> list[str]:
    found: list[str] = []
    lines = text.splitlines()
    in_cached_list = False

    for raw_line in lines:
        line = raw_line.strip()

        if "dryrun_cached_list<<EOF" in line:
            in_cached_list = True
            continue
        if in_cached_list:
            if line == "EOF":
                in_cached_list = False
                continue
            cached_line = line
            if "\t" in cached_line:
                _, cached_path = cached_line.split("\t", 1)
                cached_path = cached_path.strip()
                if PATH_RE.fullmatch(cached_path) and cached_path not in found:
                    found.append(cached_path)
            continue

        if "dryrun_targets=" in line or "dryrun_changed_targets=" in line:
            _, targets_str = line.split("=", 1)
            for token in targets_str.split():
                path = token.strip()
                if PATH_RE.fullmatch(path) and path not in found:
                    found.append(path)
            continue

        if "変更ファイル" in line or "判定した差分ファイル" in line:
            for match in PATH_RE.finditer(line):
                path = match.group(0)
                if path not in found:
                    found.append(path)

    if found:
        return found

    # Backward-compatible fallback for legacy reports without dry-run blocks.
    for match in PATH_RE.finditer(text):
        path = match.group(0)
        if path not in found:
            found.append(path)
    return found


def find_forbidden_paths(paths: list[str], patterns: list[str]) -> list[str]:
    if not patterns:
        return []
    hits: list[str] = []
    for path in paths:
        for pattern in patterns:
            if fnmatch.fnmatch(path, pattern):
                if path not in hits:
                    hits.append(path)
                break
    return hits


def load_path_patterns(path: str | Path) -> list[str]:
    pattern_path = Path(path)
    try:
        lines = pattern_path.read_text(encoding="utf-8").splitlines()
    except OSError:
        return []
    patterns: list[str] = []
    for raw in lines:
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if line not in patterns:
            patterns.append(line)
    return patterns


def audit_entry(lines: list[str], source: str = "c_section") -> CEntryAudit:
    text = "\n".join(lines)
    dryrun_results = parse_dryrun_results(text)
    safe_stage_command = parse_safe_stage_command(text)
    paths = parse_paths(text)
    template_placeholders = parse_template_placeholders(text)
    return CEntryAudit(
        title=lines[0].strip() if lines else "(no-entry)",
        start_epoch=parse_start_epoch(text),
        end_epoch=parse_end_epoch(text),
        elapsed_min=parse_elapsed_min(text),
        has_dryrun_result=bool(dryrun_results),
        dryrun_result_values=dryrun_results,
        has_dryrun_command=("scripts/c_stage_dryrun.sh" in text),
        has_safe_stage_command=(safe_stage_command is not None),
        has_safe_stage_git_add=bool(
            safe_stage_command and SAFE_STAGE_GIT_ADD_RE.match(safe_stage_command)
        ),
        safe_stage_command=safe_stage_command,
        has_pending_placeholder=("<pending>" in text),
        has_token_missing_marker=bool(
            re.search(r"ERROR:\s*token file not found", text, re.IGNORECASE)
        ),
        has_template_placeholder=bool(template_placeholders),
        template_placeholders=template_placeholders,
        detected_paths=paths,
        source=source,
    )


def choose_latest(entries: list[CEntryAudit]) -> CEntryAudit:
    with_epoch = [e for e in entries if e.start_epoch is not None]
    if with_epoch:
        return max(with_epoch, key=lambda e: e.start_epoch if e.start_epoch is not None else -1)
    return entries[0]


def title_is_c_task(title: str) -> bool:
    return "C-" in title and "PM-" not in title


def collect_latest(markdown: str, team_prefix_only: bool, global_fallback: bool = True) -> CEntryAudit:
    lines = markdown.splitlines()
    section = section_slice(lines, SECTION_HEADING)
    section_entries = split_entries(section)
    audited = [audit_entry(e, source="c_section") for e in section_entries]
    if global_fallback:
        global_entries = split_entries_global(lines)
        audited.extend(audit_entry(e, source="global_fallback") for e in global_entries)
    if not audited:
        return CEntryAudit(
            title="(entry not found)",
            start_epoch=None,
            end_epoch=None,
            elapsed_min=None,
            has_dryrun_result=False,
            dryrun_result_values=[],
            has_dryrun_command=False,
            has_safe_stage_command=False,
            has_safe_stage_git_add=False,
            safe_stage_command=None,
            has_pending_placeholder=False,
            has_token_missing_marker=False,
            has_template_placeholder=False,
            template_placeholders=[],
            detected_paths=[],
            source="c_section",
        )
    if team_prefix_only:
        filtered = [e for e in audited if title_is_c_task(e.title)]
        if filtered:
            audited = filtered
    return choose_latest(audited)


def print_report(
    audit: CEntryAudit,
    require_pass: bool,
    require_both: bool,
    require_c_section: bool,
    require_coupled_freeze: bool,
    require_complete_timer: bool,
    require_safe_stage_command: bool,
    require_no_template_placeholder: bool,
    forbid_patterns: list[str],
    coupled_freeze_patterns: list[str],
    coupled_freeze_file: str,
) -> int:
    print("AUDIT_TARGET: latest C-team entry for staging dry-run compliance")
    print("-" * 96)
    dryrun_value = ",".join(audit.dryrun_result_values) if audit.dryrun_result_values else "-"
    start_epoch = audit.start_epoch if audit.start_epoch is not None else "-"
    end_epoch = audit.end_epoch if audit.end_epoch is not None else "-"
    elapsed_min = audit.elapsed_min if audit.elapsed_min is not None else "-"
    print(f"entry: {audit.title}")
    print(f"start_epoch: {start_epoch}")
    print(f"end_epoch: {end_epoch}")
    print(f"elapsed_min: {elapsed_min}")
    print(f"dryrun_results: {dryrun_value}")
    print(f"has_c_stage_dryrun_command: {audit.has_dryrun_command}")
    print(f"has_safe_stage_command: {audit.has_safe_stage_command}")
    print(f"has_safe_stage_git_add: {audit.has_safe_stage_git_add}")
    print(f"safe_stage_command: {audit.safe_stage_command or '-'}")
    print(f"has_pending_placeholder: {audit.has_pending_placeholder}")
    print(f"has_token_missing_marker: {audit.has_token_missing_marker}")
    print(f"has_template_placeholder: {audit.has_template_placeholder}")
    if audit.template_placeholders:
        print(f"template_placeholders: {','.join(audit.template_placeholders)}")
    print(f"source: {audit.source}")
    print(f"detected_paths_count: {len(audit.detected_paths)}")
    print(f"require_pass: {require_pass}")
    print(f"require_both: {require_both}")
    print(f"require_c_section: {require_c_section}")
    print(f"require_coupled_freeze: {require_coupled_freeze}")
    print(f"require_complete_timer: {require_complete_timer}")
    print(f"require_safe_stage_command: {require_safe_stage_command}")
    print(f"require_no_template_placeholder: {require_no_template_placeholder}")
    if require_coupled_freeze:
        print(f"coupled_freeze_file: {coupled_freeze_file}")
        print(f"coupled_freeze_patterns_count: {len(coupled_freeze_patterns)}")
    if forbid_patterns:
        print(f"forbid_patterns: {','.join(forbid_patterns)}")
    verdict = audit.verdict(
        require_pass=require_pass,
        require_both=require_both,
        require_c_section=require_c_section,
        require_coupled_freeze=require_coupled_freeze,
        require_complete_timer=require_complete_timer,
        require_safe_stage_command=require_safe_stage_command,
        require_no_template_placeholder=require_no_template_placeholder,
        forbid_patterns=forbid_patterns,
        coupled_freeze_patterns=coupled_freeze_patterns,
    )
    print(f"verdict: {verdict}")
    reasons = audit.reasons(
        require_pass=require_pass,
        require_both=require_both,
        require_c_section=require_c_section,
        require_coupled_freeze=require_coupled_freeze,
        require_complete_timer=require_complete_timer,
        require_safe_stage_command=require_safe_stage_command,
        require_no_template_placeholder=require_no_template_placeholder,
        forbid_patterns=forbid_patterns,
        coupled_freeze_patterns=coupled_freeze_patterns,
    )
    if reasons:
        print(f"reasons: {', '.join(reasons)}")
        print("RESULT: FAIL")
        return 1
    print("RESULT: PASS")
    return 0


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    markdown = read_text(Path(args.team_status))
    audit = collect_latest(markdown, args.team_prefix_only, args.global_fallback)
    coupled_patterns = load_path_patterns(args.coupled_freeze_file)
    if args.require_coupled_freeze and not coupled_patterns:
        coupled_patterns = list(FALLBACK_COUPLED_FREEZE_PATTERNS)
    if args.print_coupled_freeze_patterns:
        for pattern in coupled_patterns:
            print(pattern)
        return 0
    if args.json:
        payload = audit.to_dict()
        payload["team"] = "C"
        payload["require_pass"] = args.require_pass
        payload["require_both"] = args.require_both
        payload["require_coupled_freeze"] = args.require_coupled_freeze
        payload["require_complete_timer"] = args.require_complete_timer
        payload["require_safe_stage_command"] = args.require_safe_stage_command
        payload["require_no_template_placeholder"] = args.require_no_template_placeholder
        payload["coupled_freeze_file"] = args.coupled_freeze_file
        payload["coupled_freeze_patterns"] = coupled_patterns
        payload["verdict"] = audit.verdict(
            require_pass=args.require_pass,
            require_both=args.require_both,
            require_c_section=args.require_c_section,
            require_coupled_freeze=args.require_coupled_freeze,
            require_complete_timer=args.require_complete_timer,
            require_safe_stage_command=args.require_safe_stage_command,
            require_no_template_placeholder=args.require_no_template_placeholder,
            forbid_patterns=args.forbid_path,
            coupled_freeze_patterns=coupled_patterns,
        )
        payload["reasons"] = audit.reasons(
            require_pass=args.require_pass,
            require_both=args.require_both,
            require_c_section=args.require_c_section,
            require_coupled_freeze=args.require_coupled_freeze,
            require_complete_timer=args.require_complete_timer,
            require_safe_stage_command=args.require_safe_stage_command,
            require_no_template_placeholder=args.require_no_template_placeholder,
            forbid_patterns=args.forbid_path,
            coupled_freeze_patterns=coupled_patterns,
        )
        print(json.dumps(payload, ensure_ascii=False, indent=2))
        return 0 if payload["verdict"] == "PASS" else 1
    return print_report(
        audit,
        require_pass=args.require_pass,
        require_both=args.require_both,
        require_c_section=args.require_c_section,
        require_coupled_freeze=args.require_coupled_freeze,
        require_complete_timer=args.require_complete_timer,
        require_safe_stage_command=args.require_safe_stage_command,
        require_no_template_placeholder=args.require_no_template_placeholder,
        forbid_patterns=args.forbid_path,
        coupled_freeze_patterns=coupled_patterns,
        coupled_freeze_file=args.coupled_freeze_file,
    )


if __name__ == "__main__":
    raise SystemExit(main())
