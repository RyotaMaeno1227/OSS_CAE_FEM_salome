#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path

EXPECTED_TARGET = "coupled_compare_reason_code_contract_checks"
REQUIRED_KEYS = (
    "contract_audit_target",
    "contract_audit_mode",
    "contract_audit_log_path",
    "contract_audit_cache_log",
    "contract_audit_result",
)
REQUIRED_PASS_LINES = (
    "PASS: coupled_compare reason-code consistency check is stable",
    "PASS: coupled_compare manifest validator rejects undocumented reason codes",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Validate the repo-root focused contract audit wrapper report."
    )
    parser.add_argument(
        "audit_report_log",
        nargs="?",
        help="Log emitted by run_coupled_compare_reason_code_contract_audit.sh",
    )
    parser.add_argument(
        "--expected-target",
        default=EXPECTED_TARGET,
        help="Expected contract_audit_target value.",
    )
    parser.add_argument(
        "--print-required-keys",
        action="store_true",
        help="Print the required metadata keys and pass lines, then exit.",
    )
    return parser.parse_args()


def parse_key_values(log_path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    with log_path.open("r", encoding="utf-8") as handle:
        for raw_line in handle:
            line = raw_line.rstrip("\n")
            if "=" not in line:
                continue
            key, value = line.split("=", 1)
            values[key] = value
    return values


def resolve_file(path_str: str, label: str) -> Path:
    path = Path(path_str).expanduser().resolve()
    if not path.is_file():
        raise SystemExit(f"FAIL: {label} missing: {path_str}")
    return path


def require_keys(values: dict[str, str], keys: tuple[str, ...], context: str) -> None:
    missing = [key for key in keys if not values.get(key)]
    if missing:
        raise SystemExit(f"FAIL: {context} missing keys: {','.join(missing)}")


def require_exact(values: dict[str, str], key: str, expected: str, context: str) -> None:
    if values.get(key) != expected:
        raise SystemExit(f"FAIL: {context} expected {key}={expected}, got {values.get(key, '-')}")


def require_pass(values: dict[str, str], key: str, context: str) -> None:
    require_exact(values, key, "pass", context)


def require_lines(log_path: Path, expected_lines: tuple[str, ...], context: str) -> None:
    lines = {line.rstrip("\n") for line in log_path.read_text(encoding="utf-8").splitlines()}
    for expected_line in expected_lines:
        if expected_line not in lines:
            raise SystemExit(f"FAIL: {context} missing line: {expected_line}")


def main() -> int:
    args = parse_args()
    if args.print_required_keys:
        print("contract_audit_required_keys=" + ",".join(REQUIRED_KEYS))
        print("contract_audit_required_pass_lines=" + "|".join(REQUIRED_PASS_LINES))
        return 0
    if not args.audit_report_log:
        raise SystemExit(
            "FAIL: audit_report_log is required unless --print-required-keys is used"
        )

    audit_report_log = resolve_file(args.audit_report_log, "audit_report_log")
    values = parse_key_values(audit_report_log)
    require_keys(values, REQUIRED_KEYS, "contract audit report")
    require_exact(values, "contract_audit_target", args.expected_target, "contract audit report")
    require_pass(values, "contract_audit_result", "contract audit report")
    cache_log = resolve_file(values["contract_audit_cache_log"], "contract_audit_cache_log")
    require_lines(cache_log, REQUIRED_PASS_LINES, "contract audit cache log")

    mode = values["contract_audit_mode"]
    if mode not in {"stdout", "logfile"}:
        raise SystemExit(
            "FAIL: contract audit report expected contract_audit_mode=stdout|logfile, "
            f"got {mode}"
        )

    if mode == "stdout":
        require_exact(values, "contract_audit_log_path", "<stdout>", "contract audit report")
        require_lines(audit_report_log, REQUIRED_PASS_LINES, "contract audit report")
    else:
        log_path = values["contract_audit_log_path"]
        if log_path == "<stdout>":
            raise SystemExit(
                "FAIL: contract audit report expected logfile path, got <stdout>"
            )
        nested_log = resolve_file(log_path, "contract_audit_log_path")
        require_lines(nested_log, REQUIRED_PASS_LINES, "contract audit logfile")
        require_lines(audit_report_log, REQUIRED_PASS_LINES, "contract audit report")

    print("PASS: coupled_compare reason-code contract audit report")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
