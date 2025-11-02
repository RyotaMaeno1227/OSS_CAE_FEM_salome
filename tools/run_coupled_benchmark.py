#!/usr/bin/env python3
"""
Run the Coupled constraint microbenchmark, persist metrics to CSV, and surface threshold warnings.
Designed for automation (CI / weekly cron) but usable locally as well.
"""

from __future__ import annotations

import argparse
import csv
import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple

try:
    import yaml  # type: ignore
except ImportError:
    yaml = None  # type: ignore


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BENCH_PATH = REPO_ROOT / "chrono-C-all" / "tests" / "bench_coupled_constraint"
DEFAULT_OUTPUT_PATH = REPO_ROOT / "data" / "coupled_benchmark_metrics.csv"
DEFAULT_BUILD_DIR = REPO_ROOT / "chrono-C-all"


@dataclass
class Thresholds:
    max_solve_time_us: float = 20.0
    max_condition: float = 1.0e9
    max_pending_steps: int = 1200


@dataclass
class FailThresholds:
    solve_time_us: float | None = None
    max_condition: float | None = None
    max_pending_steps: int | None = None
    unrecovered_drops: int | None = None


@dataclass
class BenchRow:
    eq_count: int
    epsilon: float
    max_condition: float
    avg_condition: float
    avg_solve_time_us: float
    drop_events: int
    drop_index_mask: int
    recovery_events: int
    avg_recovery_steps: float
    max_recovery_steps: int
    unrecovered_drops: int
    max_pending_steps: int

    @staticmethod
    def from_dict(row: Dict[str, str]) -> "BenchRow":
        return BenchRow(
            eq_count=int(row["eq_count"]),
            epsilon=float(row["epsilon"]),
            max_condition=float(row["max_condition"]),
            avg_condition=float(row["avg_condition"]),
            avg_solve_time_us=float(row["avg_solve_time_us"]),
            drop_events=int(row["drop_events"]),
            drop_index_mask=int(row["drop_index_mask"]),
            recovery_events=int(row["recovery_events"]),
            avg_recovery_steps=float(row["avg_recovery_steps"]),
            max_recovery_steps=int(row["max_recovery_steps"]),
            unrecovered_drops=int(row["unrecovered_drops"]),
            max_pending_steps=int(row["max_pending_steps"]),
        )


def run_command(cmd: List[str], cwd: Path | None = None) -> None:
    subprocess.run(cmd, cwd=cwd, check=True)


def build_benchmark(build_dir: Path) -> None:
    run_command(["make", "bench"], cwd=build_dir)


def run_benchmark(bench_path: Path, output_path: Path) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    run_command([str(bench_path), "--output", str(output_path)])


def load_csv(path: Path) -> List[BenchRow]:
    with path.open("r", newline="") as handle:
        reader = csv.DictReader(handle)
        rows = [BenchRow.from_dict(row) for row in reader]
    return rows


def emit_warning(message: str) -> None:
    # GitHub Actions command format; harmless during local runs.
    print(f"::warning::{message}")


def evaluate_thresholds(rows: Iterable[BenchRow], thresholds: Thresholds) -> bool:
    """
    Emits GitHub-style warnings when thresholds are exceeded.
    """
    ok = True

    for row in rows:
        if row.avg_solve_time_us > thresholds.max_solve_time_us:
            ok = False
            emit_warning(
                f"[CoupledBench] eq={row.eq_count} eps={row.epsilon:.1e} solve time "
                f"{row.avg_solve_time_us:.2f}us exceeds {thresholds.max_solve_time_us:.2f}us"
            )
        if row.max_condition > thresholds.max_condition:
            ok = False
            emit_warning(
                f"[CoupledBench] eq={row.eq_count} eps={row.epsilon:.1e} condition "
                f"{row.max_condition:.2e} exceeds {thresholds.max_condition:.2e}"
            )
        if row.max_pending_steps > thresholds.max_pending_steps:
            ok = False
            emit_warning(
                f"[CoupledBench] eq={row.eq_count} eps={row.epsilon:.1e} pending steps "
                f"{row.max_pending_steps} exceeds {thresholds.max_pending_steps}"
            )

        if row.eq_count == 1:
            if row.drop_events != 0 or row.unrecovered_drops != 0:
                ok = False
                emit_warning(
                    f"[CoupledBench] eq=1 should not drop equations "
                    f"(drop_events={row.drop_events}, unrecovered={row.unrecovered_drops})"
                )
        else:
            if row.drop_events == 0:
                ok = False
                emit_warning(
                    f"[CoupledBench] eq={row.eq_count} expected drop events but recorded none"
                )

    return ok


def collect_failures(rows: Iterable[BenchRow], thresholds: FailThresholds) -> List[str]:
    failures: List[str] = []
    for row in rows:
        descriptor = f"eq={row.eq_count} eps={row.epsilon:.1e}"
        if thresholds.solve_time_us is not None and row.avg_solve_time_us > thresholds.solve_time_us:
            failures.append(
                f"[CoupledBench] {descriptor} average solve time "
                f"{row.avg_solve_time_us:.2f}us exceeds hard limit {thresholds.solve_time_us:.2f}us"
            )
        if thresholds.max_condition is not None and row.max_condition > thresholds.max_condition:
            failures.append(
                f"[CoupledBench] {descriptor} max condition "
                f"{row.max_condition:.2e} exceeds hard limit {thresholds.max_condition:.2e}"
            )
        if thresholds.max_pending_steps is not None and row.max_pending_steps > thresholds.max_pending_steps:
            failures.append(
                f"[CoupledBench] {descriptor} pending drop duration "
                f"{row.max_pending_steps} exceeds hard limit {thresholds.max_pending_steps}"
            )
        if thresholds.unrecovered_drops is not None and row.unrecovered_drops > thresholds.unrecovered_drops:
            failures.append(
                f"[CoupledBench] {descriptor} unrecovered drops "
                f"{row.unrecovered_drops} exceeds hard limit {thresholds.unrecovered_drops}"
            )
    return failures


def summarize(rows: Iterable[BenchRow]) -> str:
    lines = [
        "eq_count epsilon   avg_time_us drop_events unrecovered_drops max_pending_steps max_condition"
    ]
    for row in rows:
        lines.append(
            f"{row.eq_count:7d} {row.epsilon:8.1e} "
            f"{row.avg_solve_time_us:11.3f} {row.drop_events:11d} "
            f"{row.unrecovered_drops:17d} {row.max_pending_steps:18d} "
            f"{row.max_condition:13.3e}"
        )
    return "\n".join(lines)


def parse_args(argv: List[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run Coupled constraint benchmark, persist CSV, and validate thresholds."
    )
    parser.add_argument(
        "--bench-path",
        default=str(DEFAULT_BENCH_PATH),
        help="Path to bench_coupled_constraint executable.",
    )
    parser.add_argument(
        "--output",
        default=str(DEFAULT_OUTPUT_PATH),
        help="Destination CSV path (default: %(default)s).",
    )
    parser.add_argument(
        "--build-dir",
        default=str(DEFAULT_BUILD_DIR),
        help="Directory containing the Makefile for building the benchmark.",
    )
    parser.add_argument(
        "--config",
        help="Optional YAML/JSON config file specifying warning/failure thresholds.",
    )
    parser.add_argument(
        "--skip-build",
        action="store_true",
        help="Skip running 'make bench' before executing the benchmark.",
    )
    parser.add_argument(
        "--max-solve-time-us",
        type=float,
        default=None,
        help="Warning threshold for average solve time (microseconds).",
    )
    parser.add_argument(
        "--max-condition",
        type=float,
        default=None,
        help="Warning threshold for maximum condition number.",
    )
    parser.add_argument(
        "--max-pending-steps",
        type=int,
        default=None,
        help="Warning threshold for pending drop steps duration.",
    )
    parser.add_argument(
        "--fail-on-solve-time-us",
        type=float,
        help="Fail when the average solve time (microseconds) exceeds the given limit.",
    )
    parser.add_argument(
        "--fail-on-max-condition",
        type=float,
        help="Fail when the maximum condition number exceeds the given limit.",
    )
    parser.add_argument(
        "--fail-on-max-pending-steps",
        type=int,
        help="Fail when unresolved drop duration exceeds the given step count.",
    )
    parser.add_argument(
        "--fail-on-unrecovered",
        type=int,
        help="Fail when unrecovered drop count exceeds the given limit.",
    )
    return parser.parse_args(argv)


def _load_config(path: Path) -> Dict[str, object]:
    if not path.exists():
        raise FileNotFoundError(f"Config file not found: {path}")
    if path.suffix.lower() in {".json"}:
        import json

        with path.open("r", encoding="utf-8") as handle:
            return json.load(handle)
    if path.suffix.lower() in {".yaml", ".yml"}:
        if yaml is None:
            raise RuntimeError("PyYAML is required to load YAML configuration files.")
        with path.open("r", encoding="utf-8") as handle:
            return yaml.safe_load(handle)
    raise RuntimeError(f"Unsupported config extension: {path.suffix}")


def _apply_threshold_config(config: Dict[str, object]) -> Tuple[Thresholds, FailThresholds]:
    warnings_cfg = config.get("warnings") if isinstance(config, dict) else None
    failures_cfg = config.get("failures") if isinstance(config, dict) else None

    thresholds = Thresholds()
    fail_thresholds = FailThresholds()

    if isinstance(warnings_cfg, dict):
        if "max_solve_time_us" in warnings_cfg:
            thresholds.max_solve_time_us = float(warnings_cfg["max_solve_time_us"])
        if "max_condition" in warnings_cfg:
            thresholds.max_condition = float(warnings_cfg["max_condition"])
        if "max_pending_steps" in warnings_cfg:
            thresholds.max_pending_steps = int(warnings_cfg["max_pending_steps"])

    if isinstance(failures_cfg, dict):
        if "solve_time_us" in failures_cfg:
            fail_thresholds.solve_time_us = float(failures_cfg["solve_time_us"])
        if "max_condition" in failures_cfg:
            fail_thresholds.max_condition = float(failures_cfg["max_condition"])
        if "max_pending_steps" in failures_cfg:
            fail_thresholds.max_pending_steps = int(failures_cfg["max_pending_steps"])
        if "unrecovered_drops" in failures_cfg:
            fail_thresholds.unrecovered_drops = int(failures_cfg["unrecovered_drops"])

    return thresholds, fail_thresholds


def main(argv: List[str]) -> int:
    args = parse_args(argv)

    bench_path = Path(args.bench_path)
    output_path = Path(args.output)
    build_dir = Path(args.build_dir)
    config_thresholds: Optional[Tuple[Thresholds, FailThresholds]] = None

    if args.config:
        config_data = _load_config(Path(args.config).expanduser())
        config_thresholds = _apply_threshold_config(config_data)

    if config_thresholds is not None:
        thresholds, fail_thresholds = config_thresholds
    else:
        thresholds, fail_thresholds = Thresholds(), FailThresholds()

    if args.max_solve_time_us is not None:
        thresholds.max_solve_time_us = args.max_solve_time_us
    if args.max_condition is not None:
        thresholds.max_condition = args.max_condition
    if args.max_pending_steps is not None:
        thresholds.max_pending_steps = args.max_pending_steps

    if args.fail_on_solve_time_us is not None:
        fail_thresholds.solve_time_us = args.fail_on_solve_time_us
    if args.fail_on_max_condition is not None:
        fail_thresholds.max_condition = args.fail_on_max_condition
    if args.fail_on_max_pending_steps is not None:
        fail_thresholds.max_pending_steps = args.fail_on_max_pending_steps
    if args.fail_on_unrecovered is not None:
        fail_thresholds.unrecovered_drops = args.fail_on_unrecovered

    if fail_thresholds.max_pending_steps is not None and fail_thresholds.max_pending_steps < 0:
        print("Error: max pending steps hard limit must be non-negative.", file=sys.stderr)
        return 2
    if fail_thresholds.unrecovered_drops is not None and fail_thresholds.unrecovered_drops < 0:
        print("Error: unrecovered drop hard limit must be non-negative.", file=sys.stderr)
        return 2
    if fail_thresholds.solve_time_us is not None and fail_thresholds.solve_time_us <= 0:
        print("Error: solve time hard limit must be positive.", file=sys.stderr)
        return 2
    if fail_thresholds.max_condition is not None and fail_thresholds.max_condition <= 0:
        print("Error: max condition hard limit must be positive.", file=sys.stderr)
        return 2

    if not args.skip_build:
        build_benchmark(build_dir)

    run_benchmark(bench_path, output_path)

    rows = load_csv(output_path)
    if not rows:
        emit_warning("[CoupledBench] CSV is empty - benchmark likely failed to emit results")
        return 1

    thresholds = Thresholds(
        max_solve_time_us=args.max_solve_time_us,
        max_condition=args.max_condition,
        max_pending_steps=args.max_pending_steps,
    )

    print("Coupled benchmark metrics summary:")
    print(summarize(rows))

    ok = evaluate_thresholds(rows, thresholds)

    fail_thresholds = FailThresholds(
        solve_time_us=args.fail_on_solve_time_us,
        max_condition=args.fail_on_max_condition,
        max_pending_steps=args.fail_on_max_pending_steps,
        unrecovered_drops=args.fail_on_unrecovered,
    )
    failures = collect_failures(rows, fail_thresholds)
    if failures:
        for message in failures:
            print(message, file=sys.stderr)
        return 3

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
