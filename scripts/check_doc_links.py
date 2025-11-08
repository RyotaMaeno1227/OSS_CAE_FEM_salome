#!/usr/bin/env python3
"""Validate cross-document links inside the tutorial/Hands-on docs.

Usage:
    python scripts/check_doc_links.py \
        docs/coupled_constraint_tutorial_draft.md \
        docs/coupled_constraint_hands_on.md

The script searches for Markdown links that start with ``docs/`` (optionally
prefixed with ``../``) and verifies that the referenced files exist within the
repository. If a missing link is found the script prints the offending file and
exits with a non-zero status so CI can fail fast.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

LINK_RE = re.compile(r"\((?:\.{2}/)?(docs/[\w./-]+?\.md)(?:#[^)]+)?\)")


def collect_targets(markdown: str) -> set[str]:
    return set(LINK_RE.findall(markdown))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Validate docs/ links inside Markdown files.")
    parser.add_argument("files", nargs="+", help="Markdown files to scan")
    parser.add_argument(
        "--repo-root",
        default=Path(__file__).resolve().parents[1],
        type=Path,
        help="Repository root (defaults to script/../..)",
    )
    args = parser.parse_args(argv)

    repo_root: Path = args.repo_root.resolve()
    missing: list[str] = []

    for file_arg in args.files:
        md_path = Path(file_arg)
        if not md_path.exists():
            missing.append(f"[input-missing] {file_arg}")
            continue
        text = md_path.read_text(encoding="utf-8")
        for target in collect_targets(text):
            normalized = Path(target)
            if normalized.parts[0] == "..":
                normalized = normalized.relative_to("..")
            target_path = repo_root / normalized
            if not target_path.exists():
                missing.append(f"{md_path}: missing {normalized}")

    if missing:
        print("Broken links detected:", file=sys.stderr)
        for item in missing:
            print(f"  - {item}", file=sys.stderr)
        return 1

    print(f"All links validated across {len(args.files)} file(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
