#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path

REVIEW_TRACE_RE = re.compile(
    r"^(C_TEAM_REVIEW_COMMAND_AUDIT|"
    r"has_missing_log_review_command=|"
    r"has_collect_report_review_command=|"
    r"has_collect_check_command=|"
    r"require_collect_report_if_check_command=|"
    r"review_command_check=)"
)


def find_first(lines: list[str], pattern: re.Pattern[str]) -> tuple[int, str] | None:
    for idx, line in enumerate(lines):
        if pattern.search(line):
            return idx, line
    return None


def find_all(lines: list[str], pattern: re.Pattern[str]) -> list[tuple[int, str]]:
    out: list[tuple[int, str]] = []
    for idx, line in enumerate(lines):
        if pattern.search(line):
            out.append((idx, line))
    return out


def check_review_trace_boundary(
    lines: list[str], trace_start: int, trace_end: int
) -> tuple[list[str], dict[str, int]]:
    issues: list[str] = []
    meta: dict[str, int] = {}
    review_lines = find_all(lines, REVIEW_TRACE_RE)
    if not review_lines:
        return issues, meta

    meta["idx_review_trace_first"] = review_lines[0][0]
    meta["idx_review_trace_last"] = review_lines[-1][0]
    for idx, _ in review_lines:
        if trace_start < idx < trace_end:
            issues.append(
                "invalid_review_trace_boundary: review output interleaved with fail-trace block"
            )
            meta["idx_review_trace_interleaved"] = idx
            break
    return issues, meta


def verify_strict(lines: list[str]) -> tuple[bool, list[str], dict[str, int | str]]:
    issues: list[str] = []
    meta: dict[str, int | str] = {}

    reason = find_first(lines, re.compile(r"^collect_preflight_check_reason=.*_strict$"))
    check = find_first(lines, re.compile(r"^collect_preflight_check=fail$"))
    summary_check = find_first(lines, re.compile(r"^submission_readiness_collect_preflight_check=fail$"))
    summary_reason = find_first(
        lines,
        re.compile(r"^submission_readiness_collect_preflight_reason=.*_strict$"),
    )
    retry = find_first(lines, re.compile(r"^submission_readiness_retry_command=.*"))
    fail_step = find_first(lines, re.compile(r"^submission_readiness_fail_step=collect_preflight$"))

    required = {
        "reason": reason,
        "collect_preflight_check": check,
        "summary_check": summary_check,
        "summary_reason": summary_reason,
        "retry": retry,
        "fail_step": fail_step,
    }
    for key, value in required.items():
        if value is None:
            issues.append(f"missing {key}")
        else:
            meta[f"idx_{key}"] = value[0]

    if reason and summary_check and summary_reason and retry and fail_step:
        if not (reason[0] < summary_check[0] < summary_reason[0] < retry[0] < fail_step[0]):
            issues.append(
                "invalid_order(strict): expected reason < summary_check < summary_reason < retry < fail_step"
            )
        review_issues, review_meta = check_review_trace_boundary(lines, reason[0], fail_step[0])
        issues.extend(review_issues)
        meta.update(review_meta)

    return (len(issues) == 0, issues, meta)


def verify_default(lines: list[str]) -> tuple[bool, list[str], dict[str, int | str]]:
    issues: list[str] = []
    meta: dict[str, int | str] = {}

    reason = find_first(lines, re.compile(r"^collect_preflight_check_reason=.*_default_skip$"))
    check = find_first(lines, re.compile(r"^collect_preflight_check=skipped$"))
    summary_check = find_first(lines, re.compile(r"^submission_readiness_collect_preflight_check=skipped$"))
    summary_reason = find_first(
        lines,
        re.compile(r"^submission_readiness_collect_preflight_reason=.*_default_skip$"),
    )
    retry = find_all(lines, re.compile(r"^submission_readiness_retry_command=.*"))
    fail_step = find_all(lines, re.compile(r"^submission_readiness_fail_step=collect_preflight$"))

    required = {
        "reason": reason,
        "collect_preflight_check": check,
        "summary_check": summary_check,
        "summary_reason": summary_reason,
    }
    for key, value in required.items():
        if value is None:
            issues.append(f"missing {key}")
        else:
            meta[f"idx_{key}"] = value[0]

    if reason and summary_check and summary_reason:
        if not (reason[0] < summary_check[0] < summary_reason[0]):
            issues.append("invalid_order(default): expected reason < summary_check < summary_reason")
        review_issues, review_meta = check_review_trace_boundary(
            lines, reason[0], summary_reason[0]
        )
        issues.extend(review_issues)
        meta.update(review_meta)

    if retry:
        issues.append("unexpected retry in default skip")
        meta["idx_retry"] = retry[0][0]
    if fail_step:
        issues.append("unexpected fail_step in default skip")
        meta["idx_fail_step"] = fail_step[0][0]

    return (len(issues) == 0, issues, meta)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate C-team strict/default fail-trace ordering from command logs"
    )
    parser.add_argument("log_path", help="Path to captured command log")
    parser.add_argument(
        "--mode",
        choices=["strict", "default"],
        required=True,
        help="Validation mode",
    )
    args = parser.parse_args()

    path = Path(args.log_path)
    text = path.read_text(encoding="utf-8")
    lines = [line.rstrip("\n") for line in text.splitlines()]

    if args.mode == "strict":
        ok, issues, meta = verify_strict(lines)
    else:
        ok, issues, meta = verify_default(lines)

    print("C_TEAM_FAIL_TRACE_ORDER")
    print(f"log_path={path}")
    print(f"mode={args.mode}")
    for key in sorted(meta):
        print(f"{key}={meta[key]}")
    if issues:
        print("issues=" + ", ".join(issues))
        print("verdict=FAIL")
        return 1

    print("verdict=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
