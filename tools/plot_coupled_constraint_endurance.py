#!/usr/bin/env python3
"""
Plot helper for coupled constraint endurance CSV logs.

Usage:
    python tools/plot_coupled_constraint_endurance.py [path/to/csv] --output figure.png
"""

import argparse
import csv
import os
import sys
from pathlib import Path
from typing import Dict, List, Tuple

import matplotlib.pyplot as plt

CHRONO_COUPLED_DIAG_RANK_DEFICIENT = 0x1
CHRONO_COUPLED_DIAG_CONDITION_WARNING = 0x2


def default_csv_path() -> Path:
    project_root = Path(__file__).resolve().parent.parent
    return project_root / "data" / "coupled_constraint_endurance.csv"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Visualize coupled constraint endurance diagnostics."
    )
    parser.add_argument(
        "csv_path",
        nargs="?",
        default=str(default_csv_path()),
        help="Path to coupled_constraint_endurance.csv (default: %(default)s)",
    )
    parser.add_argument(
        "-o",
        "--output",
        help="Optional output image path. When omitted the plot is not saved.",
    )
    parser.add_argument(
        "--dpi",
        type=int,
        default=160,
        help="Figure DPI when saving (default: %(default)s).",
    )
    parser.add_argument(
        "--show",
        action="store_true",
        help="Display the figure window even when --output is supplied.",
    )
    parser.add_argument(
        "--no-show",
        action="store_true",
        help="Do not display the figure window (useful for CI pipelines).",
    )
    return parser.parse_args()


def _collect_equation_indices(fieldnames: List[str], suffix: str) -> List[int]:
    indices = []
    prefix = "eq"
    for name in fieldnames:
        if not name.startswith(prefix) or not name.endswith(suffix):
            continue
        middle = name[len(prefix) : -len(suffix)]
        if not middle.isdigit():
            continue
        indices.append(int(middle))
    return sorted(set(indices))


def load_csv(path: Path) -> Dict[str, object]:
    with path.open("r", newline="") as handle:
        reader = csv.DictReader(handle)
        if not reader.fieldnames:
            raise ValueError("CSV file does not contain a header row.")

        eq_ids = _collect_equation_indices(reader.fieldnames, "_force_distance")
        if not eq_ids:
            raise ValueError("Could not identify equation columns in the CSV.")

        time: List[float] = []
        distance: List[float] = []
        angle: List[float] = []
        condition: List[float] = []
        flags: List[int] = []
        eq_force_distance: Dict[int, List[float]] = {idx: [] for idx in eq_ids}
        eq_force_angle: Dict[int, List[float]] = {idx: [] for idx in eq_ids}
        eq_impulse: Dict[int, List[float]] = {idx: [] for idx in eq_ids}

        for row in reader:
            try:
                time.append(float(row["time"]))
                distance.append(float(row["distance"]))
                angle.append(float(row["angle"]))
                condition.append(float(row["condition_number"]))
                flags.append(int(row["diagnostics_flags"]))
            except (ValueError, KeyError) as exc:
                raise ValueError(f"Malformed row encountered: {row}") from exc

            for idx in eq_ids:
                force_dist_key = f"eq{idx}_force_distance"
                force_ang_key = f"eq{idx}_force_angle"
                impulse_key = f"eq{idx}_impulse"
                eq_force_distance[idx].append(float(row.get(force_dist_key, 0.0)))
                eq_force_angle[idx].append(float(row.get(force_ang_key, 0.0)))
                eq_impulse[idx].append(float(row.get(impulse_key, 0.0)))

    return {
        "time": time,
        "distance": distance,
        "angle": angle,
        "condition": condition,
        "flags": flags,
        "eq_ids": eq_ids,
        "eq_force_distance": eq_force_distance,
        "eq_force_angle": eq_force_angle,
        "eq_impulse": eq_impulse,
    }


def summarize(data: Dict[str, object]) -> None:
    time = data["time"]
    condition = data["condition"]
    flags = data["flags"]

    max_condition = max(condition) if condition else 0.0
    warn_frames = sum(1 for f in flags if f & CHRONO_COUPLED_DIAG_CONDITION_WARNING)
    rank_frames = sum(1 for f in flags if f & CHRONO_COUPLED_DIAG_RANK_DEFICIENT)

    print(f"Loaded {len(time)} samples.")
    print(f"Maximum condition number: {max_condition:.3e}")
    print(f"Condition warning frames: {warn_frames}")
    print(f"Rank deficient frames: {rank_frames}")


def plot(data: Dict[str, object]) -> Tuple[plt.Figure, List[plt.Axes]]:
    time = data["time"]
    distance = data["distance"]
    angle = data["angle"]
    condition_raw = data["condition"]
    flags = data["flags"]
    eq_ids = data["eq_ids"]
    eq_force_distance = data["eq_force_distance"]
    eq_force_angle = data["eq_force_angle"]

    fig, axes = plt.subplots(3, 1, figsize=(12, 10), sharex=True)

    ax_state = axes[0]
    ax_state.plot(time, distance, color="C0", label="distance [m]")
    ax_state.set_ylabel("Distance")
    ax_state.grid(True, which="both", linestyle="--", alpha=0.3)
    ax_state_right = ax_state.twinx()
    ax_state_right.plot(time, angle, color="C1", label="angle [rad]")
    ax_state_right.set_ylabel("Angle")
    ax_state.set_title("Coupled Constraint Endurance")
    handles_left, labels_left = ax_state.get_legend_handles_labels()
    handles_right, labels_right = ax_state_right.get_legend_handles_labels()
    ax_state.legend(handles_left + handles_right, labels_left + labels_right, loc="upper right")

    ax_force = axes[1]
    for idx in eq_ids:
        ax_force.plot(time, eq_force_distance[idx], label=f"eq{idx} distance", linewidth=1.0)
        ax_force.plot(
            time,
            eq_force_angle[idx],
            label=f"eq{idx} angle",
            linewidth=1.0,
            linestyle="--",
        )
    ax_force.set_ylabel("Forces")
    ax_force.grid(True, linestyle="--", alpha=0.3)
    ax_force.legend(loc="upper right", ncol=2)

    ax_condition = axes[2]
    condition = [max(val, 1e-12) for val in condition_raw]
    ax_condition.plot(time, condition, color="C3", label="condition number")
    ax_condition.set_ylabel("Condition #")
    ax_condition.set_xlabel("Time [s]")
    ax_condition.set_yscale("log")
    ax_condition.grid(True, which="both", linestyle="--", alpha=0.3)

    warning_times = [t for t, flag in zip(time, flags) if flag & CHRONO_COUPLED_DIAG_CONDITION_WARNING]
    warning_values = [c for c, flag in zip(condition, flags) if flag & CHRONO_COUPLED_DIAG_CONDITION_WARNING]
    if warning_times:
        ax_condition.scatter(warning_times, warning_values, color="red", s=18, label="condition warning")

    rank_times = [t for t, flag in zip(time, flags) if flag & CHRONO_COUPLED_DIAG_RANK_DEFICIENT]
    rank_values = [c for c, flag in zip(condition, flags) if flag & CHRONO_COUPLED_DIAG_RANK_DEFICIENT]
    if rank_times:
        ax_condition.scatter(rank_times, rank_values, color="black", marker="x", s=28, label="rank deficient")

    if warning_times or rank_times:
        ax_condition.legend(loc="upper right")

    fig.tight_layout()
    return fig, list(axes)


def main() -> int:
    args = parse_args()
    csv_path = Path(args.csv_path).expanduser()
    if not csv_path.exists():
        print(f"Error: CSV file not found at {csv_path}", file=sys.stderr)
        return 1

    try:
        data = load_csv(csv_path)
    except ValueError as exc:
        print(f"Error while reading {csv_path}: {exc}", file=sys.stderr)
        return 1

    summarize(data)
    fig, _ = plot(data)

    if args.output:
        output_path = Path(args.output).expanduser()
        output_path.parent.mkdir(parents=True, exist_ok=True)
        fig.savefig(output_path, dpi=args.dpi, bbox_inches="tight")
        print(f"Saved figure to {output_path}")

    should_show = args.show or (not args.no_show and not args.output)
    if should_show:
        plt.show()
    else:
        plt.close(fig)

    return 0


if __name__ == "__main__":
    sys.exit(main())
