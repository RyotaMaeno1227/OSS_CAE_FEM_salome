#!/usr/bin/env python3
"""
Extract Coupled/Island failure snippets from CI test logs.

The `make test` target emits blocks in the form `Running <binary>`. This helper
pulls out sections whose binary name mentions "coupled" or "island" by default
so that troubleshooting the larger `test.log` becomes faster.
"""

from __future__ import annotations

import argparse
from collections import deque
from pathlib import Path
from typing import Iterable, List, Sequence

DEFAULT_KEYWORDS = ["coupled", "island"]


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Filter CI logs to only include Coupled/Island related failures."
    )
    parser.add_argument(
        "log",
        nargs="?",
        default="test.log",
        help="Path to the CI log file (default: %(default)s).",
    )
    parser.add_argument(
        "--output",
        help="Optional output file. If omitted, results are written to stdout.",
    )
    parser.add_argument(
        "--keywords",
        nargs="+",
        default=DEFAULT_KEYWORDS,
        help="Keywords that mark relevant tests (default: %(default)s).",
    )
    parser.add_argument(
        "--context",
        type=int,
        default=3,
        help="Number of context lines to include before keyword matches outside structured sections (default: %(default)s).",
    )
    parser.add_argument(
        "--tag-input",
        action="store_true",
        help="Annotate the source log with keyword tags (in-place unless --tag-output is provided).",
    )
    parser.add_argument(
        "--tag-output",
        help="Optional path for the tagged log (implies --tag-input).",
    )
    parser.add_argument(
        "--tag-only",
        action="store_true",
        help="Only annotate the log; skip the extraction step.",
    )
    return parser.parse_args(argv)


def section_matches(header: str, keywords: Iterable[str]) -> bool:
    target = header.lower()
    return any(keyword in target for keyword in keywords)


def extract_sections(lines: List[str], keywords: Iterable[str]) -> List[str]:
    matches: List[str] = []
    buffer: List[str] = []
    include_section = False

    def flush():
        nonlocal buffer, include_section, matches
        if include_section and buffer:
            matches.extend(buffer)
            if not buffer[-1].endswith("\n"):
                matches.append("\n")
        buffer = []
        include_section = False

    for line in lines:
        if line.startswith("Running "):
            flush()
            buffer = [line]
            include_section = section_matches(line, keywords)
        else:
            buffer.append(line)
    flush()
    return matches


def extract_keyword_hits(lines: List[str], keywords: Iterable[str], context: int) -> List[str]:
    keywords_lower = [keyword.lower() for keyword in keywords]
    window: deque[str] = deque(maxlen=context)
    snippets: List[str] = []
    for line in lines:
        lower = line.lower()
        window.append(line)
        if any(keyword in lower for keyword in keywords_lower):
            snippets.extend(list(window))
            snippets.append("\n")
            window.clear()
    return snippets


def annotate_lines(lines: List[str], keywords: Iterable[str]) -> List[str]:
    keywords_lower = [keyword.lower() for keyword in keywords]
    tagged: List[str] = []
    for line in lines:
        if line.startswith("Running "):
            lower = line.lower()
            matched = next((kw for kw in keywords_lower if kw in lower), None)
            if matched and not line.lstrip().startswith("["):
                tag = matched.upper()
                line = f"[{tag}] {line}"
        tagged.append(line)
    return tagged


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    log_path = Path(args.log).expanduser()
    if not log_path.exists():
        print(f"Log file not found: {log_path}")
        return 1

    content = log_path.read_text(encoding="utf-8", errors="replace").splitlines(keepends=True)
    if args.tag_input or args.tag_output:
        tagged = annotate_lines(content, args.keywords)
        target_path = Path(args.tag_output).expanduser() if args.tag_output else log_path
        target_path.write_text("".join(tagged), encoding="utf-8")
        content = tagged
        if args.tag_only:
            return 0

    if args.tag_only:
        return 0

    sections = extract_sections(content, args.keywords)
    keyword_hits = extract_keyword_hits(content, args.keywords, args.context)
    output_lines: List[str] = []
    if sections:
        output_lines.append("### Coupled/Island test sections ###\n")
        output_lines.extend(sections)
    if keyword_hits:
        output_lines.append("### Keyword hits ###\n")
        output_lines.extend(keyword_hits)
    if not output_lines:
        output_lines.append("No matching sections found.\n")

    text = "".join(output_lines)
    if args.output:
        output_path = Path(args.output).expanduser()
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(text, encoding="utf-8")
    else:
        print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
