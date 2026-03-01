#!/usr/bin/env python3
"""Validate coupled-freeze forbidden path list for C-team staging audits."""
from __future__ import annotations

import argparse
import re
from pathlib import Path

DEFAULT_PATH = "scripts/c_coupled_freeze_forbidden_paths.txt"
ALLOWED_PREFIX_RE = re.compile(r"^(FEM4C|docs|scripts|chrono-2d)/")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Validate coupled-freeze forbidden path file"
    )
    parser.add_argument("path", nargs="?", default=DEFAULT_PATH)
    return parser.parse_args()


def load_patterns(path: Path) -> tuple[list[str], list[str]]:
    if not path.exists():
        return [], [f"file_not_found: {path}"]
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        return [], [f"read_error: {exc}"]

    patterns: list[str] = []
    errors: list[str] = []
    seen: set[str] = set()
    for line_no, raw in enumerate(lines, start=1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if line in seen:
            errors.append(f"duplicate_pattern@{line_no}: {line}")
            continue
        seen.add(line)
        if not ALLOWED_PREFIX_RE.match(line):
            errors.append(f"invalid_prefix@{line_no}: {line}")
            continue
        patterns.append(line)
    if not patterns:
        errors.append("no_active_patterns")
    return patterns, errors


def main() -> int:
    args = parse_args()
    target = Path(args.path)
    patterns, errors = load_patterns(target)

    print("COUPLED_FREEZE_FILE_CHECK")
    print(f"path: {target}")
    print(f"active_patterns: {len(patterns)}")
    if errors:
        print("verdict: FAIL")
        print("reasons:")
        for item in errors:
            print(f"- {item}")
        return 1

    print("verdict: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
