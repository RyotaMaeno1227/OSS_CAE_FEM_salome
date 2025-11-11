#!/usr/bin/env python3
"""Compare Chrono-C and chrono-main KKT/Spectral logs and emit a Markdown report."""

from __future__ import annotations

import argparse
import csv
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Tuple


@dataclass
class KKTRecord:
    scenario: str
    eq_count: int
    kappa_bound: float
    kappa_spectral: float
    min_pivot: float
    max_pivot: float
    log_level_request: str
    log_level_actual: str
    log_category: str
    pivot_primary: float


@dataclass
class MultiOmegaSummary:
    scenario: str
    omega: float
    max_eq_count: int
    max_condition: float
    max_condition_spectral: float
    max_condition_gap: float
    min_pivot: float
    max_pivot: float
    drop_events: int
    avg_solve_time_us: float


@dataclass
class FailureBucket:
    week_start: str
    total_runs: int
    failures: int
    failure_rate: float


@dataclass
class FailureSummary:
    weeks: int
    buckets: List[FailureBucket]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--chrono-c",
        default="data/diagnostics/chrono_c_kkt_log.csv",
        help="Chrono-C CSV path (default: %(default)s)",
    )
    parser.add_argument(
        "--chrono-main",
        default="data/diagnostics/chrono_main_kkt_log.csv",
        help="chrono-main CSV path (default: %(default)s)",
    )
    parser.add_argument(
        "--output",
        default="docs/reports/kkt_spectral_weekly.md",
        help="Markdown output path (default: %(default)s)",
    )
    parser.add_argument(
        "--multi-omega",
        default="data/diagnostics/bench_coupled_constraint_multi.csv",
        help="Multi-ω bench CSV path (default: %(default)s)",
    )
    parser.add_argument(
        "--multi-omega-json",
        help="Optional Multi-ω bench JSON path (preferred when available).",
    )
    parser.add_argument(
        "--failure-json",
        default="data/diagnostics/archive_failure_rate_summary.json",
        help="Archive failure-rate summary JSON (default: %(default)s)",
    )
    parser.add_argument(
        "--kkt-stats",
        default="data/diagnostics/kkt_backend_stats.json",
        help="Chrono KKT backend stats JSON (default: %(default)s)",
    )
    parser.add_argument(
        "--csv-output",
        help="Optional CSV path mirroring the Scenario comparison table.",
    )
    return parser.parse_args()


def load_records(path: Path) -> Dict[Tuple[str, int], KKTRecord]:
    records: Dict[Tuple[str, int], KKTRecord] = {}
    with path.open("r", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            scenario = row.get("scenario", "").strip() or "default"
            eq_count = int(row["eq_count"])
            pivot_primary = float(row.get("pivot_log_primary", "0.0"))
            records[(scenario, eq_count)] = KKTRecord(
                scenario=scenario,
                eq_count=eq_count,
                kappa_bound=float(row["kappa_bound"]),
                kappa_spectral=float(row["kappa_spectral"]),
                min_pivot=float(row["min_pivot"]),
                max_pivot=float(row["max_pivot"]),
                log_level_request=row.get("log_level_request", ""),
                log_level_actual=row.get("log_level_actual", ""),
                log_category=row.get("log_category", ""),
                pivot_primary=pivot_primary,
            )
    return records


def load_multi_omega(path: Path) -> List[MultiOmegaSummary]:
    records: Dict[Tuple[str, float], MultiOmegaSummary] = {}
    if not path.exists():
        return []
    with path.open("r", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            scenario = row.get("scenario", "default")
            omega = float(row.get("omega", "0.0"))
            key = (scenario, omega)
            summary = records.get(key)
            eq_count = int(row.get("eq_count", "0"))
            max_condition = float(row.get("max_condition", "0"))
            max_condition_spectral = float(row.get("max_condition_spectral", "0"))
            max_condition_gap = float(row.get("max_condition_gap", "0"))
            drop_events = int(row.get("drop_events", "0"))
            avg_time = float(row.get("avg_solve_time_us", "0"))
            min_pivot = float(row.get("min_pivot", "0")) if "min_pivot" in row else 0.0
            max_pivot = float(row.get("max_pivot", "0")) if "max_pivot" in row else 0.0
            if summary is None:
                summary = MultiOmegaSummary(
                    scenario=scenario,
                    omega=omega,
                    max_eq_count=eq_count,
                    max_condition=max_condition,
                    max_condition_spectral=max_condition_spectral,
                    max_condition_gap=max_condition_gap,
                    min_pivot=min_pivot if min_pivot > 0 else max_pivot,
                    max_pivot=max_pivot,
                    drop_events=drop_events,
                    avg_solve_time_us=avg_time,
                )
                records[key] = summary
            else:
                summary.max_eq_count = max(summary.max_eq_count, eq_count)
                summary.max_condition = max(summary.max_condition, max_condition)
                summary.max_condition_spectral = max(
                    summary.max_condition_spectral, max_condition_spectral
                )
                summary.max_condition_gap = max(summary.max_condition_gap, max_condition_gap)
                summary.drop_events = max(summary.drop_events, drop_events)
                summary.avg_solve_time_us = max(summary.avg_solve_time_us, avg_time)
                if min_pivot > 0.0:
                    summary.min_pivot = min(summary.min_pivot, min_pivot) if summary.min_pivot > 0 else min_pivot
                summary.max_pivot = max(summary.max_pivot, max_pivot)
    return sorted(records.values(), key=lambda item: (item.scenario, item.omega))


def load_multi_omega_json(path: Path) -> List[MultiOmegaSummary]:
    if not path.exists():
        return []
    payload = json.loads(path.read_text(encoding="utf-8"))
    records: List[MultiOmegaSummary] = []
    for entry in payload:
        records.append(
            MultiOmegaSummary(
                scenario=entry.get("scenario", "default"),
                omega=float(entry.get("omega", 0.0)),
                max_eq_count=int(entry.get("eq_count", 0)),
                max_condition=float(entry.get("max_condition", 0.0)),
                max_condition_spectral=float(entry.get("max_condition_spectral", 0.0)),
                max_condition_gap=float(entry.get("max_condition_gap", 0.0)),
                min_pivot=float(entry.get("min_pivot", 0.0)),
                max_pivot=float(entry.get("max_pivot", 0.0)),
                drop_events=int(entry.get("drop_events", 0)),
                avg_solve_time_us=float(entry.get("avg_solve_time_us", 0.0)),
            )
        )
    return sorted(records, key=lambda item: (item.scenario, item.omega))


def load_failure_summary(path: Path) -> Optional[FailureSummary]:
    if not path.exists():
        return None
    payload = json.loads(path.read_text(encoding="utf-8"))
    buckets_data = payload.get("buckets", [])
    buckets: List[FailureBucket] = []
    for entry in buckets_data:
        buckets.append(
            FailureBucket(
                week_start=entry.get("week_start", "n/a"),
                total_runs=entry.get("total_runs", 0),
                failures=entry.get("failures", 0),
                failure_rate=float(entry.get("failure_rate", 0.0)),
            )
        )
    return FailureSummary(weeks=payload.get("weeks", 0), buckets=buckets)


def load_kkt_stats(path: Path) -> Optional[dict]:
    if not path.exists():
        return None
    return json.loads(path.read_text(encoding="utf-8"))


def format_scientific(value: float) -> str:
    return f"{value:.3e}"


def format_diff(a: float | None, b: float | None) -> str:
    if a is None or b is None:
        return "n/a"
    delta = abs(a - b)
    return f"{delta:.3e}"


def build_comparison_rows(chrono_c: Dict[Tuple[str, int], KKTRecord],
                          chrono_main: Dict[Tuple[str, int], KKTRecord]) -> List[dict]:
    keys = sorted(set(chrono_c.keys()) | set(chrono_main.keys()))
    rows: List[dict] = []
    for key in keys:
        chrono_c_rec = chrono_c.get(key)
        chrono_main_rec = chrono_main.get(key)
        scenario, eq_count = key
        status = "⚠️"
        bound_delta = None
        spectral_delta = None
        if chrono_c_rec and chrono_main_rec:
            bound_delta = abs(chrono_c_rec.kappa_bound - chrono_main_rec.kappa_bound)
            spectral_delta = abs(chrono_c_rec.kappa_spectral - chrono_main_rec.kappa_spectral)
            tolerance = max(chrono_c_rec.kappa_bound, chrono_main_rec.kappa_bound) * 0.05
            if bound_delta <= tolerance and spectral_delta <= tolerance:
                status = "✅"
        elif chrono_c_rec or chrono_main_rec:
            status = "⚠️"
        rows.append(
            {
                "scenario": scenario,
                "eq_count": eq_count,
                "c_bound": chrono_c_rec.kappa_bound if chrono_c_rec else None,
                "m_bound": chrono_main_rec.kappa_bound if chrono_main_rec else None,
                "d_bound": format_diff(
                    chrono_c_rec.kappa_bound if chrono_c_rec else None,
                    chrono_main_rec.kappa_bound if chrono_main_rec else None,
                ),
                "c_spec": chrono_c_rec.kappa_spectral if chrono_c_rec else None,
                "m_spec": chrono_main_rec.kappa_spectral if chrono_main_rec else None,
                "d_spec": format_diff(
                    chrono_c_rec.kappa_spectral if chrono_c_rec else None,
                    chrono_main_rec.kappa_spectral if chrono_main_rec else None,
                ),
                "d_min": format_diff(
                    chrono_c_rec.min_pivot if chrono_c_rec else None,
                    chrono_main_rec.min_pivot if chrono_main_rec else None,
                ),
                "d_max": format_diff(
                    chrono_c_rec.max_pivot if chrono_c_rec else None,
                    chrono_main_rec.max_pivot if chrono_main_rec else None,
                ),
                "d_pivot0": format_diff(
                    chrono_c_rec.pivot_primary if chrono_c_rec else None,
                    chrono_main_rec.pivot_primary if chrono_main_rec else None,
                ),
                "log_c": chrono_c_rec.log_level_actual if chrono_c_rec else "n/a",
                "log_m": chrono_main_rec.log_level_actual if chrono_main_rec else "n/a",
                "status": status,
            }
        )
    return rows


def generate_report(comparison_rows: List[dict],
                    omega_records: Optional[List[MultiOmegaSummary]] = None,
                    failure_summary: Optional[FailureSummary] = None,
                    kkt_stats: Optional[dict] = None) -> str:
    lines = [
        "# Weekly KKT / Spectral Comparison",
        "",
        "| Scenario | eq_count | κ̂ (Chrono-C) | κ̂ (chrono-main) | Δκ̂ | κ_s (Chrono-C) | "
        "κ_s (chrono-main) | Δκ_s | min pivot Δ | max pivot Δ | pivot₀ Δ | Log levels (C/main) | Status |",
        "|----------|---------:|--------------:|-----------------:|-----:|---------------:|"
        "------------------:|------:|-------------:|-------------:|-----------:|---------------------|--------|",
    ]
    def val_or_na(value: float | None) -> str:
        return format_scientific(value) if value is not None else "n/a"

    for row in comparison_rows:
        lines.append(
            "| {scenario} | {eq_count} | {c_bound} | {m_bound} | {d_bound} | "
            "{c_spec} | {m_spec} | {d_spec} | {d_min} | {d_max} | {d_pivot0} | {log_c}/{log_m} | {status} |".format(
                scenario=row["scenario"],
                eq_count=row["eq_count"],
                c_bound=val_or_na(row["c_bound"]),
                m_bound=val_or_na(row["m_bound"]),
                d_bound=row["d_bound"],
                c_spec=val_or_na(row["c_spec"]),
                m_spec=val_or_na(row["m_spec"]),
                d_spec=row["d_spec"],
                d_min=row["d_min"],
                d_max=row["d_max"],
                d_pivot0=row["d_pivot0"],
                log_c=row["log_c"],
                log_m=row["log_m"],
                status=row["status"],
            )
        )

    lines.append("")
    lines.append("Δ values are absolute differences; ✅ indicates both κ metrics aligned within 5%.")
    if omega_records:
        lines.append("")
        lines.append("## Multi-ω Bench Status")
        lines.append("")
        lines.append("| Scenario | ω | eq_countₘₐₓ | κ̂ₘₐₓ | κ_sₘₐₓ | Δκ | pivot span | drop events | avg solve (µs) | Status |")
        lines.append("|----------|---:|-----------:|-------:|--------:|-----:|-----------:|-----------:|---------------:|--------|")
        for record in omega_records:
            delta = abs(record.max_condition - record.max_condition_spectral)
            pivot_span = 0.0
            if record.min_pivot > 0.0 and record.max_pivot > 0.0:
                pivot_span = record.max_pivot - record.min_pivot
            status = "✅" if delta <= 0.05 * max(record.max_condition, 1e-9) else "⚠️"
            lines.append(
                "| {scenario} | {omega:.3f} | {eq} | {kappa} | {kappa_s} | {delta_val} | {pivot} | {drops} | {solve:.3f} | {status} |".format(
                    scenario=record.scenario,
                    omega=record.omega,
                    eq=record.max_eq_count,
                    kappa=format_scientific(record.max_condition),
                    kappa_s=format_scientific(record.max_condition_spectral),
                    delta_val=format_scientific(delta),
                    pivot=format_scientific(pivot_span) if pivot_span > 0 else "n/a",
                    drops=record.drop_events,
                    solve=record.avg_solve_time_us,
                    status=status,
                )
            )
    if failure_summary:
        lines.append("")
        lines.append("## Archive Failure Rate")
        if not failure_summary.buckets:
            lines.append("")
            lines.append("_No archive-and-summarize runs found for the configured window._")
        else:
            lines.append("")
            lines.append("| Week start | Runs | Failures | Failure rate |")
            lines.append("|------------|-----:|---------:|-------------:|")
            for bucket in failure_summary.buckets:
                lines.append(
                    f"| {bucket.week_start} | {bucket.total_runs} | {bucket.failures} | {bucket.failure_rate * 100.0:.1f}% |"
                )
    if kkt_stats:
        lines.append("")
        lines.append("## KKT Backend Cache Metrics")
        lines.append("")
        lines.append("| Calls | Fallback | Cache hits | Cache misses | Cache checks | Hit rate |")
        lines.append("|------:|---------:|-----------:|-------------:|-------------:|---------:|")
        lines.append(
            "| {calls} | {fallback} | {hits} | {misses} | {checks} | {rate:.2%} |".format(
                calls=kkt_stats.get("calls", 0),
                fallback=kkt_stats.get("fallback_calls", 0),
                hits=kkt_stats.get("cache_hits", 0),
                misses=kkt_stats.get("cache_misses", 0),
                checks=kkt_stats.get("cache_checks", 0),
                rate=kkt_stats.get("cache_hit_rate", 0.0),
            )
        )
        histogram_values = kkt_stats.get("size_histogram", [])
        histogram = ", ".join(str(value) for value in histogram_values)
        upper = len(histogram_values) - 1 if histogram_values else 0
        lines.append("")
        lines.append(f"Histogram (eq_count=0…{upper}): [{histogram}]")
    return "\n".join(lines)


def main() -> int:
    args = parse_args()
    chrono_c_path = Path(args.chrono_c)
    chrono_main_path = Path(args.chrono_main)
    if args.multi_omega_json:
        omega_records = load_multi_omega_json(Path(args.multi_omega_json))
    else:
        omega_records = load_multi_omega(Path(args.multi_omega))
    failure_summary = load_failure_summary(Path(args.failure_json))
    chrono_c_records = load_records(chrono_c_path)
    chrono_main_records = load_records(chrono_main_path)
    chrono_rows = build_comparison_rows(chrono_c_records, chrono_main_records)
    report_text = generate_report(
        chrono_rows,
        omega_records,
        failure_summary,
        load_kkt_stats(Path(args.kkt_stats)),
    )
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(report_text + "\n", encoding="utf-8")
    print(report_text)
    print(f"\nWrote report to {output_path}")

    if args.csv_output:
        csv_path = Path(args.csv_output)
        csv_path.parent.mkdir(parents=True, exist_ok=True)
        with csv_path.open("w", encoding="utf-8", newline="") as handle:
            writer = csv.writer(handle)
            writer.writerow(
                [
                    "scenario",
                    "eq_count",
                    "kappa_bound_chrono_c",
                    "kappa_bound_chrono_main",
                    "delta_bound",
                    "kappa_spectral_chrono_c",
                    "kappa_spectral_chrono_main",
                    "delta_spectral",
                    "delta_min_pivot",
                    "delta_max_pivot",
                    "delta_primary_pivot",
                    "log_levels",
                    "status",
                ]
            )
            for row in chrono_rows:
                writer.writerow(
                    [
                        row["scenario"],
                        row["eq_count"],
                        row["c_bound"] if row["c_bound"] is not None else "",
                        row["m_bound"] if row["m_bound"] is not None else "",
                        row["d_bound"],
                        row["c_spec"] if row["c_spec"] is not None else "",
                        row["m_spec"] if row["m_spec"] is not None else "",
                        row["d_spec"],
                        row["d_min"],
                        row["d_max"],
                        row["d_pivot0"],
                        f"{row['log_c']}/{row['log_m']}",
                        row["status"],
                    ]
                )
        print(f"Wrote comparison CSV to {csv_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
