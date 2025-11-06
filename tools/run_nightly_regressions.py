#!/usr/bin/env python3
"""
Run the nightly regression subset and emit a CSV summary.

This helper focuses on the Island solver and coupled constraint tests so that a
dedicated GitHub Actions workflow can track regressions without the noise from
the broader test suite.
"""

from __future__ import annotations

import argparse
import csv
import subprocess
import sys
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable, List, Sequence

REPO_ROOT = Path(__file__).resolve().parents[1]

DEFAULT_TESTS = [
    "chrono-C-all/tests/test_coupled_constraint",
    "chrono-C-all/tests/test_coupled_constraint_endurance",
    "chrono-C-all/tests/test_island_contact_constraint",
    "chrono-C-all/tests/test_island_parallel_contacts",
    "chrono-C-all/tests/test_island_polygon_longrun",
]


@dataclass
class TestResult:
    name: str
    status: str
    exit_code: int
    duration: float
    log_path: Path
    started_at: datetime
    finished_at: datetime


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Execute the nightly coupled/island regression tests and capture results."
    )
    parser.add_argument(
        "--tests",
        nargs="+",
        help="Override the default test binary list (relative to the repository root).",
    )
    parser.add_argument(
        "--log-dir",
        default="artifacts/nightly/logs",
        help="Directory for per-test log files (default: %(default)s).",
    )
    parser.add_argument(
        "--output-csv",
        default="artifacts/nightly/regression_results.csv",
        help="CSV path for the aggregated results (default: %(default)s).",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        help="Optional timeout in seconds for each test binary.",
    )
    return parser.parse_args(argv)


def run_test(binary: Path, log_dir: Path, timeout: int | None) -> TestResult:
    name = binary.relative_to(REPO_ROOT).as_posix()
    log_dir.mkdir(parents=True, exist_ok=True)
    log_path = log_dir / (binary.name + ".log")

    started_at = datetime.now(timezone.utc)
    start = time.perf_counter()
    with log_path.open("w", encoding="utf-8") as handle:
        process = subprocess.run(
            [str(binary)],
            stdout=handle,
            stderr=subprocess.STDOUT,
            cwd=binary.parent,
            timeout=timeout,
            check=False,
        )
    duration = time.perf_counter() - start
    finished_at = datetime.now(timezone.utc)

    status = "passed" if process.returncode == 0 else "failed"
    return TestResult(
        name=name,
        status=status,
        exit_code=process.returncode,
        duration=duration,
        log_path=log_path,
        started_at=started_at,
        finished_at=finished_at,
    )


def iter_tests(test_paths: Iterable[str]) -> Iterable[Path]:
    for entry in test_paths:
        candidate = Path(entry)
        if not candidate.is_absolute():
            candidate = REPO_ROOT / candidate
        candidate = candidate.resolve()
        if not candidate.exists():
            raise FileNotFoundError(f"Test binary not found: {candidate}")
        yield candidate


def write_results(csv_path: Path, results: Sequence[TestResult]) -> None:
    csv_path.parent.mkdir(parents=True, exist_ok=True)
    with csv_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(
            [
                "test_name",
                "status",
                "exit_code",
                "duration_seconds",
                "log_path",
                "started_at",
                "finished_at",
            ]
        )
        for result in results:
            writer.writerow(
                [
                    result.name,
                    result.status,
                    result.exit_code,
                    f"{result.duration:.3f}",
                    str(result.log_path.relative_to(REPO_ROOT)),
                    result.started_at.isoformat(),
                    result.finished_at.isoformat(),
                ]
            )


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    tests = args.tests if args.tests else DEFAULT_TESTS
    binaries = list(iter_tests(tests))

    results: List[TestResult] = []
    log_dir = Path(args.log_dir)
    for binary in binaries:
        try:
            result = run_test(binary, log_dir, args.timeout)
            results.append(result)
            print(f"[{result.status.upper()}] {result.name} ({result.duration:.1f}s)")
        except subprocess.TimeoutExpired:
            log_path = log_dir / (binary.name + ".log")
            log_path.parent.mkdir(parents=True, exist_ok=True)
            log_path.write_text("Test timed out.\n", encoding="utf-8")
            finished_at = datetime.now(timezone.utc)
            started_at = finished_at  # start unknown due to timeout
            results.append(
                TestResult(
                    name=binary.relative_to(REPO_ROOT).as_posix(),
                    status="timeout",
                    exit_code=124,
                    duration=float(args.timeout or 0),
                    log_path=log_path,
                    started_at=started_at,
                    finished_at=finished_at,
                )
            )
            print(f"[TIMEOUT] {binary.relative_to(REPO_ROOT)} (> {args.timeout}s)")

    write_results(Path(args.output_csv), results)

    failures = [result for result in results if result.status != "passed"]
    if failures:
        print(f"{len(failures)} test(s) failed or timed out.", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
