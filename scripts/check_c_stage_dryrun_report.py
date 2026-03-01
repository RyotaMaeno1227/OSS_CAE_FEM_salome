#!/usr/bin/env python3
"""Validate c_stage_dryrun report format and verdict."""
from __future__ import annotations

import argparse
import re
from pathlib import Path


REQUIRED_KEYS = (
    "dryrun_method",
    "dryrun_targets",
    "dryrun_changed_targets",
    "dryrun_cached_list",
    "forbidden_check",
    "coupled_freeze_file",
    "coupled_freeze_hits",
    "coupled_freeze_check",
    "required_set_check",
    "safe_stage_targets",
    "safe_stage_command",
    "dryrun_result",
)
SAFE_STAGE_GIT_ADD_RE = re.compile(r"^git\s+add(?:\s+--)?\s+\S+")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Validate c_stage_dryrun report")
    parser.add_argument("report_path")
    parser.add_argument(
        "--policy",
        choices=("any", "pass"),
        default="pass",
        help="any: only format check, pass: require pass verdict",
    )
    return parser.parse_args()


def read_report(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except OSError as exc:
        raise SystemExit(f"ERROR: failed to read report: {exc}") from exc


def collect_keys(text: str) -> set[str]:
    keys: set[str] = set()
    for line in text.splitlines():
        if line.endswith("<<EOF"):
            key = line[: -len("<<EOF")]
            keys.add(key)
            continue
        if "=" in line:
            key = line.split("=", 1)[0].strip()
            if key:
                keys.add(key)
    return keys


def value_of(text: str, key: str) -> str | None:
    prefix = f"{key}="
    for line in text.splitlines():
        if line.startswith(prefix):
            return line[len(prefix) :].strip()
    return None


def parse_safe_stage_command_targets(command: str) -> list[str]:
    tokens = command.split()
    if len(tokens) < 3 or tokens[0] != "git" or tokens[1] != "add":
        return []
    targets = tokens[2:]
    if targets and targets[0] == "--":
        targets = targets[1:]
    return targets


def main() -> int:
    args = parse_args()
    text = read_report(Path(args.report_path))
    keys = collect_keys(text)

    missing = [key for key in REQUIRED_KEYS if key not in keys]
    issues: list[str] = []
    if missing:
        issues.append(f"missing_keys={','.join(missing)}")

    if args.policy == "pass":
        if value_of(text, "dryrun_result") != "pass":
            issues.append("dryrun_result_not_pass")
        if value_of(text, "forbidden_check") != "pass":
            issues.append("forbidden_check_not_pass")
        if value_of(text, "coupled_freeze_check") != "pass":
            issues.append("coupled_freeze_check_not_pass")
        if value_of(text, "required_set_check") != "pass":
            issues.append("required_set_check_not_pass")
        safe_stage_command = value_of(text, "safe_stage_command")
        safe_stage_targets = value_of(text, "safe_stage_targets")
        if not safe_stage_command or not SAFE_STAGE_GIT_ADD_RE.match(safe_stage_command):
            issues.append("safe_stage_command_not_git_add")
        if safe_stage_targets is None:
            issues.append("safe_stage_targets_missing")
        else:
            expected_targets = safe_stage_targets.split()
            actual_targets = parse_safe_stage_command_targets(safe_stage_command or "")
            if set(expected_targets) != set(actual_targets):
                issues.append("safe_stage_command_target_mismatch")

    print("C_STAGE_DRYRUN_REPORT_CHECK")
    print(f"report_path={args.report_path}")
    print(f"policy={args.policy}")
    print(f"required_keys={len(REQUIRED_KEYS)}")
    print(f"detected_keys={len(keys)}")
    if issues:
        print("verdict=FAIL")
        for issue in issues:
            print(f"reason={issue}")
        return 1
    print("verdict=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
