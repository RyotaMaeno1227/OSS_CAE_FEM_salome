#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

SECTION_HEADING = "## Cチーム"
ENTRY_START_RE = re.compile(r"^- 実行タスク")
START_EPOCH_RE = re.compile(r"start_epoch\s*[:=]\s*`?(\d+)`?")
AUDIT_RETRY_RE = re.compile(
    r"scripts/run_c_team_fail_trace_audit\.sh\s+(?P<team_status>\S+)\s+(?P<minutes>\d+)\s+\|\s+tee\s+(?P<log>\S+)"
)
FINALIZE_LOG_RE = re.compile(r"--fail-trace-audit-log\s+(?P<log>\S+)")
FINALIZE_TEAM_STATUS_RE = re.compile(r"--team-status\s+(?P<team_status>\S+)")
FINALIZE_MINUTES_RE = re.compile(r"--check-submission-readiness-minutes\s+(?P<minutes>\d+)")


def parse_args(argv: list[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Check consistency between fail-trace audit retry/finalize retry commands"
    )
    parser.add_argument("--team-status", default="docs/team_status.md")
    parser.add_argument(
        "--require-finalize-retry-when-audit-retry",
        action="store_true",
        default=True,
        help="Require fail_trace_finalize_retry_command when fail_trace_audit_retry_command exists (default: true)",
    )
    parser.add_argument(
        "--no-require-finalize-retry-when-audit-retry",
        dest="require_finalize_retry_when_audit_retry",
        action="store_false",
        help="Disable finalize retry requirement",
    )
    parser.add_argument(
        "--require-retry-consistency-check-key",
        action="store_true",
        default=False,
        help="Fail if fail_trace_retry_consistency_check key is missing",
    )
    parser.add_argument(
        "--no-require-retry-consistency-check-key",
        dest="require_retry_consistency_check_key",
        action="store_false",
        help="Allow missing fail_trace_retry_consistency_check key",
    )
    parser.add_argument(
        "--require-strict-env-prefix-match",
        action="store_true",
        default=False,
        help=(
            "Require C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY* env prefixes in retry commands "
            "to match each other and entry keys"
        ),
    )
    return parser.parse_args(argv)


def section_slice(lines: list[str], heading: str) -> list[str]:
    start = None
    for idx, line in enumerate(lines):
        if line.strip() == heading:
            start = idx + 1
            break
    if start is None:
        return []
    for idx in range(start, len(lines)):
        if lines[idx].startswith("## "):
            return lines[start:idx]
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
    candidates: list[tuple[str, str, int]] = []
    for entry in entries:
        if not entry:
            continue
        title = entry[0].strip()
        if "C-" not in title:
            continue
        text = "\n".join(entry)
        candidates.append((title, text, parse_start_epoch(text)))
    if not candidates:
        return ("", "", -1)
    return max(candidates, key=lambda item: item[2])


def extract_entry_value(entry_text: str, key: str) -> tuple[str, int]:
    prefix = f"{key}="
    for idx, line in enumerate(entry_text.splitlines()):
        stripped = line.strip().strip("`")
        marker_idx = stripped.find(prefix)
        if marker_idx >= 0:
            value = stripped[marker_idx + len(prefix) :]
            value = value.strip().strip("`")
            return (value, idx)
    return ("", -1)


def extract_env_flag(command: str, key: str) -> str:
    if not command:
        return ""
    pattern = re.compile(rf"(?:^|\s){re.escape(key)}=(0|1)(?:\s|$)")
    match = pattern.search(command)
    if not match:
        return ""
    return match.group(1)


def reason_to_code(reason: str) -> str:
    normalized = re.sub(r"[^a-z0-9]+", "_", reason.lower()).strip("_")
    if not normalized:
        return "unknown_reason"
    return normalized


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

    audit_retry, audit_idx = extract_entry_value(entry_text, "fail_trace_audit_retry_command")
    finalize_retry, finalize_idx = extract_entry_value(entry_text, "fail_trace_finalize_retry_command")
    retry_consistency_check, _ = extract_entry_value(entry_text, "fail_trace_retry_consistency_check")
    retry_consistency_required, _ = extract_entry_value(entry_text, "fail_trace_retry_consistency_required")
    retry_consistency_require_key, _ = extract_entry_value(entry_text, "fail_trace_retry_consistency_require_key")
    retry_consistency_require_strict_env, _ = extract_entry_value(
        entry_text,
        "fail_trace_retry_consistency_require_strict_env",
    )
    audit_log, _ = extract_entry_value(entry_text, "fail_trace_audit_log")
    audit_result, _ = extract_entry_value(entry_text, "fail_trace_audit_result")
    audit_retry_reason, _ = extract_entry_value(entry_text, "fail_trace_audit_retry_reason")
    audit_missing_keys, _ = extract_entry_value(entry_text, "fail_trace_audit_missing_keys")
    has_audit_issue = bool(audit_retry_reason or audit_missing_keys or (audit_result and audit_result != "PASS"))
    audit_env_required = extract_env_flag(audit_retry, "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY")
    audit_env_require_key = extract_env_flag(audit_retry, "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY")
    audit_env_require_strict_env = extract_env_flag(
        audit_retry,
        "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV",
    )
    finalize_env_required = extract_env_flag(finalize_retry, "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY")
    finalize_env_require_key = extract_env_flag(finalize_retry, "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY")
    finalize_env_require_strict_env = extract_env_flag(
        finalize_retry,
        "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV",
    )

    if args.require_retry_consistency_check_key and not retry_consistency_check:
        reasons.append("missing fail_trace_retry_consistency_check")
    if retry_consistency_check and retry_consistency_check not in {"pass", "skipped", "fail", "unknown"}:
        reasons.append("invalid fail_trace_retry_consistency_check value")
    if retry_consistency_check == "fail":
        reasons.append("fail_trace_retry_consistency_check=fail")

    if (
        args.require_finalize_retry_when_audit_retry
        and audit_retry
        and has_audit_issue
        and not finalize_retry
    ):
        reasons.append("missing fail_trace_finalize_retry_command")

    audit_team_status = ""
    audit_minutes = ""
    audit_retry_log = ""
    if audit_retry:
        match = AUDIT_RETRY_RE.search(audit_retry)
        if not match:
            reasons.append("invalid fail_trace_audit_retry_command format")
        else:
            audit_team_status = match.group("team_status")
            audit_minutes = match.group("minutes")
            audit_retry_log = match.group("log")

    finalize_team_status = ""
    finalize_minutes = ""
    finalize_log = ""
    if finalize_retry:
        log_match = FINALIZE_LOG_RE.search(finalize_retry)
        status_match = FINALIZE_TEAM_STATUS_RE.search(finalize_retry)
        minutes_match = FINALIZE_MINUTES_RE.search(finalize_retry)
        if not log_match:
            reasons.append("missing --fail-trace-audit-log in fail_trace_finalize_retry_command")
        else:
            finalize_log = log_match.group("log")
        if not status_match:
            reasons.append("missing --team-status in fail_trace_finalize_retry_command")
        else:
            finalize_team_status = status_match.group("team_status")
        if minutes_match:
            finalize_minutes = minutes_match.group("minutes")

    if audit_retry and finalize_retry:
        if audit_idx >= 0 and finalize_idx >= 0 and audit_idx > finalize_idx:
            reasons.append("fail_trace_finalize_retry_command appears before fail_trace_audit_retry_command")
        if audit_team_status and finalize_team_status and audit_team_status != finalize_team_status:
            reasons.append("team_status mismatch between audit retry and finalize retry")
        if audit_minutes and finalize_minutes and audit_minutes != finalize_minutes:
            reasons.append("minutes mismatch between audit retry and finalize retry")
        if audit_retry_log and finalize_log and audit_retry_log != finalize_log:
            reasons.append("fail-trace log mismatch between audit retry and finalize retry")
        if audit_log and audit_retry_log and audit_log != audit_retry_log:
            reasons.append("fail_trace_audit_log and fail_trace_audit_retry_command log mismatch")
        if audit_log and finalize_log and audit_log != finalize_log:
            reasons.append("fail_trace_audit_log and fail_trace_finalize_retry_command log mismatch")

    if args.require_strict_env_prefix_match:
        if retry_consistency_required in {"0", "1"}:
            if audit_retry:
                if not audit_env_required:
                    reasons.append(
                        "missing C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY in fail_trace_audit_retry_command"
                    )
                elif audit_env_required != retry_consistency_required:
                    reasons.append(
                        "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY mismatch with fail_trace_retry_consistency_required"
                    )
            if finalize_retry:
                if not finalize_env_required:
                    reasons.append(
                        "missing C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY in fail_trace_finalize_retry_command"
                    )
                elif finalize_env_required != retry_consistency_required:
                    reasons.append(
                        "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY mismatch in fail_trace_finalize_retry_command"
                    )
        if retry_consistency_require_key in {"0", "1"}:
            if audit_retry:
                if not audit_env_require_key:
                    reasons.append(
                        "missing C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY in fail_trace_audit_retry_command"
                    )
                elif audit_env_require_key != retry_consistency_require_key:
                    reasons.append(
                        "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY mismatch with fail_trace_retry_consistency_require_key"
                    )
            if finalize_retry:
                if not finalize_env_require_key:
                    reasons.append(
                        "missing C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY in fail_trace_finalize_retry_command"
                    )
                elif finalize_env_require_key != retry_consistency_require_key:
                    reasons.append(
                        "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_KEY mismatch in fail_trace_finalize_retry_command"
                    )
        if retry_consistency_require_strict_env in {"0", "1"}:
            if audit_retry:
                if not audit_env_require_strict_env:
                    reasons.append(
                        "missing C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV in fail_trace_audit_retry_command"
                    )
                elif audit_env_require_strict_env != retry_consistency_require_strict_env:
                    reasons.append(
                        "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV mismatch with fail_trace_retry_consistency_require_strict_env"
                    )
            if finalize_retry:
                if not finalize_env_require_strict_env:
                    reasons.append(
                        "missing C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV in fail_trace_finalize_retry_command"
                    )
                elif finalize_env_require_strict_env != retry_consistency_require_strict_env:
                    reasons.append(
                        "C_FAIL_TRACE_REQUIRE_RETRY_CONSISTENCY_STRICT_ENV mismatch in fail_trace_finalize_retry_command"
                    )

        if audit_retry and finalize_retry:
            if audit_env_required and finalize_env_required and audit_env_required != finalize_env_required:
                reasons.append("strict env mismatch between audit retry and finalize retry (require_retry_consistency)")
            if (
                audit_env_require_key
                and finalize_env_require_key
                and audit_env_require_key != finalize_env_require_key
            ):
                reasons.append(
                    "strict env mismatch between audit retry and finalize retry (require_retry_consistency_key)"
                )
            if (
                audit_env_require_strict_env
                and finalize_env_require_strict_env
                and audit_env_require_strict_env != finalize_env_require_strict_env
            ):
                reasons.append(
                    "strict env mismatch between audit retry and finalize retry (require_retry_consistency_strict_env)"
                )

    verdict = "PASS" if not reasons else "FAIL"
    reason_codes: list[str] = []
    seen_reason_codes: set[str] = set()
    for reason in reasons:
        code = reason_to_code(reason)
        if code in seen_reason_codes:
            continue
        seen_reason_codes.add(code)
        reason_codes.append(code)

    print("C_TEAM_FAIL_TRACE_RETRY_CONSISTENCY")
    print(f"entry={title}")
    print(f"start_epoch={start_epoch if start_epoch >= 0 else ''}")
    print(f"has_audit_retry_command={'yes' if bool(audit_retry) else 'no'}")
    print(f"has_finalize_retry_command={'yes' if bool(finalize_retry) else 'no'}")
    print(f"has_audit_issue={'yes' if has_audit_issue else 'no'}")
    print(f"retry_consistency_check={retry_consistency_check}")
    print(f"retry_consistency_required={retry_consistency_required}")
    print(f"retry_consistency_require_key={retry_consistency_require_key}")
    print(f"retry_consistency_require_strict_env={retry_consistency_require_strict_env}")
    print(f"audit_result={audit_result}")
    print(f"audit_retry_reason={audit_retry_reason}")
    print(f"audit_missing_keys={audit_missing_keys}")
    print(f"audit_retry_team_status={audit_team_status}")
    print(f"audit_retry_minutes={audit_minutes}")
    print(f"audit_retry_log={audit_retry_log}")
    print(f"audit_retry_env_require_retry_consistency={audit_env_required}")
    print(f"audit_retry_env_require_retry_consistency_key={audit_env_require_key}")
    print(f"audit_retry_env_require_retry_consistency_strict_env={audit_env_require_strict_env}")
    print(f"finalize_retry_team_status={finalize_team_status}")
    print(f"finalize_retry_minutes={finalize_minutes}")
    print(f"finalize_retry_log={finalize_log}")
    print(f"finalize_retry_env_require_retry_consistency={finalize_env_required}")
    print(f"finalize_retry_env_require_retry_consistency_key={finalize_env_require_key}")
    print(f"finalize_retry_env_require_retry_consistency_strict_env={finalize_env_require_strict_env}")
    print(f"fail_trace_audit_log={audit_log}")
    print(
        "require_finalize_retry_when_audit_retry="
        + ("yes" if args.require_finalize_retry_when_audit_retry else "no")
    )
    print(
        "require_retry_consistency_check_key="
        + ("yes" if args.require_retry_consistency_check_key else "no")
    )
    print("reason_codes=" + (" ".join(reason_codes) if reason_codes else "-"))
    print(f"verdict={verdict}")
    if reasons:
        print("reasons=" + "; ".join(reasons))
    return 0 if verdict == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
