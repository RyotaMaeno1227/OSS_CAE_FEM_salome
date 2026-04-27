#!/usr/bin/env python3
"""Validate collect_c_team_session_evidence preflight report output."""
from __future__ import annotations

import argparse
from pathlib import Path


REQUIRED_KEYS = ("collect_result", "preflight_mode", "preflight_result")
ALLOWED_PREFLIGHT_MODE = {"enabled", "disabled"}
ALLOWED_PREFLIGHT_RESULT = {"pass", "skipped"}
ALLOWED_REASON_CODES_SOURCE = {"checker", "fallback", "-"}
REVIEW_REQUIRED_VALUES = {"0", "1"}
REVIEW_OPTIONAL_KEYS = (
    "review_command_fail_reason",
    "review_command_fail_reason_codes",
    "review_command_fail_reason_codes_source",
    "review_command_retry_command",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Check preflight report emitted by collect_c_team_session_evidence.sh"
    )
    parser.add_argument("report_path", help="Path to collect output log")
    parser.add_argument(
        "--require-enabled",
        action="store_true",
        help="Require preflight_mode=enabled",
    )
    parser.add_argument(
        "--expect-team-status",
        help="Require preflight_team_status to match this path (resolved absolute path)",
    )
    parser.add_argument(
        "--require-review-keys",
        action="store_true",
        help="Require review command keys (reason/reason_codes/source/retry_command) in enabled preflight logs",
    )
    return parser.parse_args()


def parse_kv(text: str) -> dict[str, str]:
    values: dict[str, str] = {}
    for raw in text.splitlines():
        line = raw.strip()
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        values[key.strip()] = value.strip()
    return values


def main() -> int:
    args = parse_args()
    path = Path(args.report_path)
    try:
        text = path.read_text(encoding="utf-8")
    except OSError as exc:
        print(f"ERROR: failed to read report: {exc}")
        return 2

    values = parse_kv(text)
    missing = [key for key in REQUIRED_KEYS if key not in values]
    if missing:
        print("C_TEAM_COLLECT_PREFLIGHT_REPORT")
        print(f"report_path={path}")
        print(f"missing_keys={','.join(missing)}")
        print(f"review_keys_required={'1' if args.require_review_keys else '0'}")
        print("verdict=FAIL")
        return 1

    if values["collect_result"] != "PASS":
        print("C_TEAM_COLLECT_PREFLIGHT_REPORT")
        print(f"report_path={path}")
        print(f"collect_result={values['collect_result']}")
        print(f"review_keys_required={'1' if args.require_review_keys else '0'}")
        print("verdict=FAIL")
        return 1

    preflight_mode = values["preflight_mode"]
    preflight_result = values["preflight_result"]
    if preflight_mode not in ALLOWED_PREFLIGHT_MODE:
        print("C_TEAM_COLLECT_PREFLIGHT_REPORT")
        print(f"report_path={path}")
        print(f"preflight_mode={preflight_mode}")
        print(f"review_keys_required={'1' if args.require_review_keys else '0'}")
        print("verdict=FAIL")
        return 1
    if preflight_result not in ALLOWED_PREFLIGHT_RESULT:
        print("C_TEAM_COLLECT_PREFLIGHT_REPORT")
        print(f"report_path={path}")
        print(f"preflight_result={preflight_result}")
        print(f"review_keys_required={'1' if args.require_review_keys else '0'}")
        print("verdict=FAIL")
        return 1
    if args.require_enabled and preflight_mode != "enabled":
        print("C_TEAM_COLLECT_PREFLIGHT_REPORT")
        print(f"report_path={path}")
        print(f"preflight_mode={preflight_mode}")
        print(f"review_keys_required={'1' if args.require_review_keys else '0'}")
        print("verdict=FAIL")
        return 1
    if preflight_mode == "enabled":
        if "preflight_team_status" not in values:
            print("C_TEAM_COLLECT_PREFLIGHT_REPORT")
            print(f"report_path={path}")
            print("missing_keys=preflight_team_status")
            print(f"review_keys_required={'1' if args.require_review_keys else '0'}")
            print("verdict=FAIL")
            return 1
        if values["preflight_team_status"] == "":
            print("C_TEAM_COLLECT_PREFLIGHT_REPORT")
            print(f"report_path={path}")
            print("preflight_team_status=<empty>")
            print(f"review_keys_required={'1' if args.require_review_keys else '0'}")
            print("verdict=FAIL")
            return 1
        if preflight_result != "pass":
            print("C_TEAM_COLLECT_PREFLIGHT_REPORT")
            print(f"report_path={path}")
            print(f"preflight_result={preflight_result}")
            print(f"review_keys_required={'1' if args.require_review_keys else '0'}")
            print("verdict=FAIL")
            return 1
        if args.expect_team_status:
            expected = str(Path(args.expect_team_status).resolve())
            actual = str(Path(values["preflight_team_status"]).resolve())
            if actual != expected:
                print("C_TEAM_COLLECT_PREFLIGHT_REPORT")
                print(f"report_path={path}")
                print(f"expected_preflight_team_status={expected}")
                print(f"actual_preflight_team_status={actual}")
                print(f"review_keys_required={'1' if args.require_review_keys else '0'}")
                print("verdict=FAIL")
                return 1
        if args.require_review_keys and "review_command_required" not in values:
            print("C_TEAM_COLLECT_PREFLIGHT_REPORT")
            print(f"report_path={path}")
            print("missing_keys=review_command_required")
            print("review_keys_required=1")
            print("verdict=FAIL")
            return 1
        if "review_command_required" in values:
            review_missing = [key for key in REVIEW_OPTIONAL_KEYS if key not in values]
            if review_missing:
                print("C_TEAM_COLLECT_PREFLIGHT_REPORT")
                print(f"report_path={path}")
                print(f"missing_keys={','.join(review_missing)}")
                print(f"review_keys_required={'1' if args.require_review_keys else '0'}")
                print("verdict=FAIL")
                return 1
            review_required = values["review_command_required"]
            if review_required not in REVIEW_REQUIRED_VALUES:
                print("C_TEAM_COLLECT_PREFLIGHT_REPORT")
                print(f"report_path={path}")
                print(f"review_command_required={review_required}")
                print(f"review_keys_required={'1' if args.require_review_keys else '0'}")
                print("verdict=FAIL")
                return 1
            reason = values["review_command_fail_reason"]
            reason_codes = values["review_command_fail_reason_codes"]
            reason_codes_source = values["review_command_fail_reason_codes_source"]
            retry_command = values["review_command_retry_command"]
            if reason_codes_source not in ALLOWED_REASON_CODES_SOURCE:
                print("C_TEAM_COLLECT_PREFLIGHT_REPORT")
                print(f"report_path={path}")
                print(f"review_command_fail_reason_codes_source={reason_codes_source}")
                print(f"review_keys_required={'1' if args.require_review_keys else '0'}")
                print("verdict=FAIL")
                return 1
            if retry_command == "":
                print("C_TEAM_COLLECT_PREFLIGHT_REPORT")
                print(f"report_path={path}")
                print("review_command_retry_command=<empty>")
                print(f"review_keys_required={'1' if args.require_review_keys else '0'}")
                print("verdict=FAIL")
                return 1
            if reason == "-":
                if reason_codes != "-" or reason_codes_source != "-":
                    print("C_TEAM_COLLECT_PREFLIGHT_REPORT")
                    print(f"report_path={path}")
                    print(f"review_command_fail_reason={reason}")
                    print(f"review_command_fail_reason_codes={reason_codes}")
                    print(f"review_command_fail_reason_codes_source={reason_codes_source}")
                    print(f"review_keys_required={'1' if args.require_review_keys else '0'}")
                    print("verdict=FAIL")
                    return 1
            else:
                if reason_codes in ("", "-"):
                    print("C_TEAM_COLLECT_PREFLIGHT_REPORT")
                    print(f"report_path={path}")
                    print(f"review_command_fail_reason={reason}")
                    print(f"review_command_fail_reason_codes={reason_codes}")
                    print(f"review_keys_required={'1' if args.require_review_keys else '0'}")
                    print("verdict=FAIL")
                    return 1
                if reason_codes_source == "-":
                    print("C_TEAM_COLLECT_PREFLIGHT_REPORT")
                    print(f"report_path={path}")
                    print(f"review_command_fail_reason={reason}")
                    print(f"review_command_fail_reason_codes_source={reason_codes_source}")
                    print(f"review_keys_required={'1' if args.require_review_keys else '0'}")
                    print("verdict=FAIL")
                    return 1

    print("C_TEAM_COLLECT_PREFLIGHT_REPORT")
    print(f"report_path={path}")
    print(f"collect_result={values['collect_result']}")
    print(f"preflight_mode={preflight_mode}")
    print(f"preflight_result={preflight_result}")
    print(f"review_keys_required={'1' if args.require_review_keys else '0'}")
    print("verdict=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
