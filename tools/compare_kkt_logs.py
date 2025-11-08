#!/usr/bin/env python3
"""Compare Chrono-C and chrono-main KKT/Spectral logs and emit a Markdown report."""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, Tuple


@dataclass
class KKTRecord:
    scenario: str
    eq_count: int
    kappa_bound: float
    kappa_spectral: float
    min_pivot: float
    max_pivot: float


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
    return parser.parse_args()


def load_records(path: Path) -> Dict[Tuple[str, int], KKTRecord]:
    records: Dict[Tuple[str, int], KKTRecord] = {}
    with path.open("r", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            scenario = row.get("scenario", "").strip() or "default"
            eq_count = int(row["eq_count"])
            records[(scenario, eq_count)] = KKTRecord(
                scenario=scenario,
                eq_count=eq_count,
                kappa_bound=float(row["kappa_bound"]),
                kappa_spectral=float(row["kappa_spectral"]),
                min_pivot=float(row["min_pivot"]),
                max_pivot=float(row["max_pivot"]),
            )
    return records


def format_scientific(value: float) -> str:
    return f"{value:.3e}"


def format_diff(a: float | None, b: float | None) -> str:
    if a is None or b is None:
        return "n/a"
    delta = abs(a - b)
    return f"{delta:.3e}"


def generate_report(chrono_c: Dict[Tuple[str, int], KKTRecord],
                    chrono_main: Dict[Tuple[str, int], KKTRecord]) -> str:
    keys = sorted(set(chrono_c.keys()) | set(chrono_main.keys()))
    lines = [
        "# Weekly KKT / Spectral Comparison",
        "",
        "| Scenario | eq_count | κ̂ (Chrono-C) | κ̂ (chrono-main) | Δκ̂ | κ_s (Chrono-C) | "
        "κ_s (chrono-main) | Δκ_s | min pivot Δ | max pivot Δ | Status |",
        "|----------|---------:|--------------:|-----------------:|-----:|---------------:|"
        "------------------:|------:|-------------:|-------------:|--------|",
    ]
    for key in keys:
        chrono_c_rec = chrono_c.get(key)
        chrono_main_rec = chrono_main.get(key)
        scenario, eq_count = key
        status = "⚠️"
        if chrono_c_rec and chrono_main_rec:
            # flag condition deltas above 5%
            bound_delta = abs(chrono_c_rec.kappa_bound - chrono_main_rec.kappa_bound)
            spectral_delta = abs(chrono_c_rec.kappa_spectral - chrono_main_rec.kappa_spectral)
            tolerance = max(chrono_c_rec.kappa_bound, chrono_main_rec.kappa_bound) * 0.05
            if bound_delta <= tolerance and spectral_delta <= tolerance:
                status = "✅"
        elif chrono_c_rec or chrono_main_rec:
            status = "⚠️"

        def val_or_na(value: float | None) -> str:
            return format_scientific(value) if value is not None else "n/a"

        lines.append(
            "| {scenario} | {eq_count} | {c_bound} | {m_bound} | {d_bound} | "
            "{c_spec} | {m_spec} | {d_spec} | {d_min} | {d_max} | {status} |".format(
                scenario=scenario,
                eq_count=eq_count,
                c_bound=val_or_na(chrono_c_rec.kappa_bound if chrono_c_rec else None),
                m_bound=val_or_na(chrono_main_rec.kappa_bound if chrono_main_rec else None),
                d_bound=format_diff(
                    chrono_c_rec.kappa_bound if chrono_c_rec else None,
                    chrono_main_rec.kappa_bound if chrono_main_rec else None,
                ),
                c_spec=val_or_na(chrono_c_rec.kappa_spectral if chrono_c_rec else None),
                m_spec=val_or_na(chrono_main_rec.kappa_spectral if chrono_main_rec else None),
                d_spec=format_diff(
                    chrono_c_rec.kappa_spectral if chrono_c_rec else None,
                    chrono_main_rec.kappa_spectral if chrono_main_rec else None,
                ),
                d_min=format_diff(
                    chrono_c_rec.min_pivot if chrono_c_rec else None,
                    chrono_main_rec.min_pivot if chrono_main_rec else None,
                ),
                d_max=format_diff(
                    chrono_c_rec.max_pivot if chrono_c_rec else None,
                    chrono_main_rec.max_pivot if chrono_main_rec else None,
                ),
                status=status,
            )
        )

    lines.append("")
    lines.append("Δ values are absolute differences; ✅ indicates both κ metrics aligned within 5%.")
    return "\n".join(lines)


def main() -> int:
    args = parse_args()
    chrono_c_path = Path(args.chrono_c)
    chrono_main_path = Path(args.chrono_main)
    report_text = generate_report(load_records(chrono_c_path), load_records(chrono_main_path))
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(report_text + "\n", encoding="utf-8")
    print(report_text)
    print(f"\nWrote report to {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
