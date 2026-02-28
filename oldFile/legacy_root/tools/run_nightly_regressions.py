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
import difflib
import hashlib
import os
import subprocess
import sys
import time
from collections import defaultdict
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple

REPO_ROOT = Path(__file__).resolve().parents[1]

DEFAULT_TESTS = [
    "chrono-C-all/tests/test_coupled_constraint",
    "chrono-C-all/tests/test_coupled_constraint_endurance",
    "chrono-C-all/tests/test_island_contact_constraint",
    "chrono-C-all/tests/test_island_parallel_contacts",
    "chrono-C-all/tests/test_island_polygon_longrun",
]

MODE_CHOICES = ["openmp", "serial"]
MODE_PRIORITY = {mode: idx for idx, mode in enumerate(MODE_CHOICES)}


@dataclass
class TestResult:
    name: str
    mode: str
    status: str
    exit_code: int
    duration: float
    log_path: Path
    log_digest: str
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
    parser.add_argument(
        "--modes",
        nargs="+",
        choices=MODE_CHOICES,
        default=["openmp", "serial"],
        help="Execution modes to run (default: both openmp and serial).",
    )
    parser.add_argument(
        "--openmp-threads",
        type=int,
        help="Override OMP_NUM_THREADS when running in openmp mode.",
    )
    parser.add_argument(
        "--skip-mode-comparison",
        action="store_true",
        help="Skip comparing logs between modes.",
    )
    parser.add_argument(
        "--diff-summary",
        help="Optional CSV path summarising mode mismatches.",
    )
    return parser.parse_args(argv)


def build_environment(mode: str, openmp_threads: Optional[int]) -> Dict[str, str]:
    env = os.environ.copy()
    if mode == "serial":
        env["OMP_NUM_THREADS"] = "1"
        env["CHRONO_DISABLE_OPENMP"] = "1"
    else:
        if openmp_threads is not None and openmp_threads > 0:
            env["OMP_NUM_THREADS"] = str(openmp_threads)
        env.pop("CHRONO_DISABLE_OPENMP", None)
    env.setdefault("OMP_PROC_BIND", "close")
    return env


def digest_file(path: Path) -> str:
    hasher = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(65536), b""):
            if not chunk:
                break
            hasher.update(chunk)
    return hasher.hexdigest()


def run_test(binary: Path, log_dir: Path, timeout: int | None, mode: str, env: Dict[str, str]) -> TestResult:
    name = binary.relative_to(REPO_ROOT).as_posix()
    log_dir.mkdir(parents=True, exist_ok=True)
    log_path = log_dir / f"{binary.name}.{mode}.log"

    started_at = datetime.now(timezone.utc)
    start = time.perf_counter()
    try:
        with log_path.open("w", encoding="utf-8") as handle:
            process = subprocess.run(
                [str(binary)],
                stdout=handle,
                stderr=subprocess.STDOUT,
                cwd=binary.parent,
                timeout=timeout,
                check=False,
                env=env,
            )
        duration = time.perf_counter() - start
        finished_at = datetime.now(timezone.utc)
        status = "passed" if process.returncode == 0 else "failed"
        log_digest = digest_file(log_path)
        return TestResult(
            name=name,
            mode=mode,
            status=status,
            exit_code=process.returncode,
            duration=duration,
            log_path=log_path,
            log_digest=log_digest,
            started_at=started_at,
            finished_at=finished_at,
        )
    except subprocess.TimeoutExpired:
        duration = time.perf_counter() - start
        finished_at = datetime.now(timezone.utc)
        with log_path.open("a", encoding="utf-8") as handle:
            handle.write(f"Test timed out after {timeout} seconds.\n")
        log_digest = digest_file(log_path)
        return TestResult(
            name=name,
            mode=mode,
            status="timeout",
            exit_code=124,
            duration=duration if duration > 0 else float(timeout or 0),
            log_path=log_path,
            log_digest=log_digest,
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
                "mode",
                "status",
                "exit_code",
                "duration_seconds",
                "log_path",
                "log_digest",
                "started_at",
                "finished_at",
            ]
        )
        for result in results:
            writer.writerow(
                [
                    result.name,
                    result.mode,
                    result.status,
                    result.exit_code,
                    f"{result.duration:.3f}",
                    str(result.log_path.relative_to(REPO_ROOT)),
                    result.log_digest,
                    result.started_at.isoformat(),
                    result.finished_at.isoformat(),
                ]
            )


Mismatch = Tuple[str, TestResult, TestResult, Path]


def compare_mode_results(results: Sequence[TestResult], log_dir: Path) -> List[Mismatch]:
    grouped: Dict[str, List[TestResult]] = defaultdict(list)
    for result in results:
        grouped[result.name].append(result)

    mismatches: List[Mismatch] = []
    for name, entries in grouped.items():
        if len(entries) < 2:
            continue
        ordered = sorted(entries, key=lambda item: MODE_PRIORITY.get(item.mode, 99))
        baseline = ordered[0]
        for candidate in ordered[1:]:
            status_differs = candidate.status != baseline.status
            digest_differs = candidate.log_digest != baseline.log_digest
            if not status_differs and not digest_differs:
                continue
            diff_filename = f"{Path(name).name}.{baseline.mode}_vs_{candidate.mode}.diff"
            diff_path = log_dir / diff_filename
            base_lines = baseline.log_path.read_text(encoding="utf-8", errors="replace").splitlines()
            cand_lines = candidate.log_path.read_text(encoding="utf-8", errors="replace").splitlines()
            diff_lines = list(
                difflib.unified_diff(
                    base_lines,
                    cand_lines,
                    fromfile=f"{baseline.mode}:{baseline.log_path.name}",
                    tofile=f"{candidate.mode}:{candidate.log_path.name}",
                    lineterm="",
                )
            )
            if not diff_lines:
                diff_lines = [
                    f"Status mismatch between modes: {baseline.status} vs {candidate.status}"
                ]
            diff_path.write_text("\n".join(diff_lines) + "\n", encoding="utf-8")
            mismatches.append((name, baseline, candidate, diff_path))
    return mismatches


def write_diff_summary(path: Path, mismatches: Sequence[Mismatch]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(
            [
                "test_name",
                "baseline_mode",
                "other_mode",
                "baseline_status",
                "other_status",
                "diff_path",
            ]
        )
        for name, baseline, other, diff_path in mismatches:
            writer.writerow(
                [
                    name,
                    baseline.mode,
                    other.mode,
                    baseline.status,
                    other.status,
                    str(diff_path.relative_to(REPO_ROOT)),
                ]
            )


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    tests = args.tests if args.tests else DEFAULT_TESTS
    binaries = list(iter_tests(tests))

    results: List[TestResult] = []
    log_dir = Path(args.log_dir)
    modes = args.modes or ["openmp"]
    for binary in binaries:
        for mode in modes:
            env = build_environment(mode, args.openmp_threads)
            result = run_test(binary, log_dir, args.timeout, mode, env)
            results.append(result)
            status_label = result.status.upper()
            print(f"[{mode.upper()} {status_label}] {result.name} ({result.duration:.1f}s)")

    write_results(Path(args.output_csv), results)

    failures = [result for result in results if result.status != "passed"]
    exit_code = 0
    if failures:
        print(f"{len(failures)} test(s) failed or timed out.", file=sys.stderr)
        exit_code = 1

    if not args.skip_mode_comparison and len(modes) > 1:
        mismatches = compare_mode_results(results, log_dir)
        if mismatches:
            exit_code = max(exit_code, 2)
            if args.diff_summary:
                write_diff_summary(Path(args.diff_summary).expanduser(), mismatches)
            for name, baseline, other, diff_path in mismatches:
                relative_diff = diff_path.relative_to(REPO_ROOT)
                print(
                    f"[MISMATCH] {name}: {baseline.mode} vs {other.mode} differ "
                    f"(see {relative_diff})",
                    file=sys.stderr,
                )
        elif args.diff_summary:
            write_diff_summary(Path(args.diff_summary).expanduser(), [])

    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
