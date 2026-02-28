#!/usr/bin/env python3
"""
Summarise condition-number related anomalies from Coupled benchmark JSONL logs.

The Coupled benchmark tooling can append validation results to a JSON Lines file
via ``--csv-validation-jsonl``.  This helper scans one or more JSONL files and
highlights entries whose type or message mentions condition numbers so that CI
failures can be triaged quickly.
"""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Iterator, List, Sequence


SeverityOrder = {"error": 3, "warning": 2, "info": 1, "ok": 0}


@dataclass(frozen=True)
class Anomaly:
    file: Path
    line_no: int
    level: str
    message: str
    payload: dict


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Extract condition-number anomalies from Coupled benchmark JSONL output."
    )
    parser.add_argument(
        "inputs",
        nargs="+",
        help="JSONL files produced by Coupled benchmark tooling.",
    )
    parser.add_argument(
        "--fail-on",
        choices=["ok", "info", "warning", "error", "never"],
        default="warning",
        help="Exit with a non-zero status when anomalies at or above the given severity appear "
        "(default: %(default)s). Use 'never' to always exit 0.",
    )
    parser.add_argument(
        "--max-entries",
        type=int,
        help="Limit the number of anomalies displayed per file.",
    )
    parser.add_argument(
        "--quiet",
        action="store_true",
        help="Suppress per-entry output; still returns a non-zero exit code based on --fail-on.",
    )
    return parser.parse_args(argv)


def _iter_jsonl(path: Path) -> Iterator[tuple[int, dict]]:
    try:
        with path.open("r", encoding="utf-8") as handle:
            for idx, line in enumerate(handle, start=1):
                text = line.strip()
                if not text:
                    continue
                try:
                    yield idx, json.loads(text)
                except json.JSONDecodeError as exc:
                    raise RuntimeError(f"{path}:{idx}: failed to parse JSON: {exc}") from exc
    except FileNotFoundError as exc:
        raise RuntimeError(f"JSONL file not found: {path}") from exc


def _looks_like_condition_issue(record: dict) -> bool:
    keywords = ["condition", "condition_number"]
    for key in list(record.keys()):
        if any(token in key.lower() for token in keywords):
            return True
    message = str(record.get("message", "")).lower()
    if any(token in message for token in keywords):
        return True
    type_field = str(record.get("type", "")).lower()
    if any(token in type_field for token in keywords):
        return True
    return False


def collect_anomalies(path: Path) -> List[Anomaly]:
    anomalies: List[Anomaly] = []
    for line_no, payload in _iter_jsonl(path):
        if not isinstance(payload, dict):
            continue
        if not _looks_like_condition_issue(payload):
            continue
        level = str(payload.get("level", "info")).lower()
        if level not in SeverityOrder:
            level = "info"
        message = str(payload.get("message", "") or payload.get("detail", ""))
        anomalies.append(Anomaly(file=path, line_no=line_no, level=level, message=message, payload=payload))
    return anomalies


def highest_severity(records: Iterable[Anomaly]) -> str:
    worst = "ok"
    for record in records:
        if SeverityOrder[record.level] > SeverityOrder[worst]:
            worst = record.level
    return worst


def should_fail(threshold: str, worst: str) -> bool:
    if threshold == "never":
        return False
    return SeverityOrder.get(worst, 0) >= SeverityOrder.get(threshold, 0)


def format_anomaly(record: Anomaly) -> str:
    pieces = [
        f"[{record.level}]",
        f"{record.file}:{record.line_no}",
        record.message or "<no message>",
    ]
    extra_fields = []
    for key in ("row", "eq_count", "max_condition", "value", "hash"):
        if key in record.payload:
            extra_fields.append(f"{key}={record.payload[key]}")
    if extra_fields:
        pieces.append("(" + ", ".join(extra_fields) + ")")
    return " ".join(pieces)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    any_failure = False
    printed = False

    for raw_path in args.inputs:
        path = Path(raw_path).expanduser()
        anomalies = collect_anomalies(path)
        worst = highest_severity(anomalies)
        if should_fail(args.fail_on, worst):
            any_failure = True

        if args.quiet:
            continue

        print(f"=== {path} ===")
        if not anomalies:
            print("No condition anomalies detected.")
            printed = True
            continue

        print(f"Detected {len(anomalies)} condition-related entries (worst level: {worst}).")
        limit = args.max_entries if args.max_entries is not None else len(anomalies)
        for record in anomalies[:limit]:
            print(" -", format_anomaly(record))
        hidden = len(anomalies) - limit
        if hidden > 0:
            print(f"   ... {hidden} additional entries not shown (use --max-entries to adjust).")
        printed = True

    if args.quiet and not printed:
        # Maintain deterministic output when --quiet is used with no files.
        pass

    return 1 if any_failure else 0


if __name__ == "__main__":
    sys.exit(main())

