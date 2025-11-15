#!/usr/bin/env python3
"""Ensure preset references point to the Markdown cheat sheet.

By default the script checks README, Hands-on, and Wiki docs to ensure they
reference `docs/coupled_constraint_presets_cheatsheet.md`. Additional files can
be supplied via command-line arguments.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import sys

DEFAULT_FILES = [
    "README.md",
    "docs/coupled_constraint_hands_on.md",
]

TARGET = "docs/coupled_constraint_presets_cheatsheet.md"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("files", nargs="*", default=DEFAULT_FILES)
    args = parser.parse_args(argv)

    missing: list[str] = []
    for path_str in args.files:
        path = Path(path_str)
        if not path.is_file():
            missing.append(f"[missing-file] {path}")
            continue
        if TARGET not in path.read_text(encoding="utf-8"):
            missing.append(f"{path}: {TARGET} not referenced")

    if missing:
        print("Preset link check failed:", file=sys.stderr)
        for item in missing:
            print(f"  - {item}", file=sys.stderr)
        return 1

    print(f"Preset links verified for {len(args.files)} file(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
