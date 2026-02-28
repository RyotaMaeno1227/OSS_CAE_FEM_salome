#!/usr/bin/env python3
"""Plot helper for coupled constraint endurance CSV logs."""

import argparse
import json
import sys
from pathlib import Path

from coupled_constraint_endurance_analysis import (
    CHRONO_COUPLED_DIAG_CONDITION_WARNING,
    CHRONO_COUPLED_DIAG_RANK_DEFICIENT,
    COUPLED_SUMMARY_VERSION,
    compute_summary,
    default_csv_path,
    load_csv,
    summary_as_html,
    summary_as_markdown,
    summary_as_plain_text,
)


_Number = (int, float)


def _import_pyplot():
    try:
        import matplotlib.pyplot as plt  # type: ignore
    except ModuleNotFoundError as exc:  # pragma: no cover - import guard
        raise RuntimeError(
            "matplotlib is required for plotting. "
            "Install it (pip install matplotlib) or run with --skip-plot."
        ) from exc
    return plt


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
    parser.add_argument(
        "--summary-md",
        help="Write Markdown summary to the specified path.",
    )
    parser.add_argument(
        "--summary-html",
        help="Write HTML summary to the specified path.",
    )
    parser.add_argument(
        "--summary-json",
        help="Write JSON summary to the specified path.",
    )
    parser.add_argument(
        "--skip-plot",
        action="store_true",
        help="Skip figure generation (useful when only summaries are required).",
    )
    parser.add_argument(
        "--fail-on-max-condition",
        type=float,
        help="Exit with code 3 if max condition number exceeds this threshold.",
    )
    parser.add_argument(
        "--fail-on-warning-ratio",
        type=float,
        help="Exit with code 4 if warning ratio exceeds this threshold (0.0 - 1.0).",
    )
    parser.add_argument(
        "--fail-on-warning-ratio-below",
        type=float,
        help="Exit with code 6 if warning ratio falls below this threshold (0.0 - 1.0).",
    )
    parser.add_argument(
        "--fail-on-rank-ratio",
        type=float,
        help="Exit with code 5 if rank-deficient ratio exceeds this threshold (0.0 - 1.0).",
    )
    parser.add_argument(
        "--fail-on-rank-ratio-below",
        type=float,
        help="Exit with code 7 if rank-deficient ratio falls below this threshold (0.0 - 1.0).",
    )
    parser.add_argument(
        "--mark-stage",
        action="append",
        metavar="TIME[:LABEL]",
        dest="stage_markers",
        help=(
            "Annotate the plot with vertical lines at specific times. "
            "Specify seconds optionally followed by a label, e.g. '--mark-stage 12.5:Phase+Switch'. "
            "Repeat the option to add multiple markers."
        ),
    )
    return parser.parse_args()


def _validate_summary_schema(payload: dict) -> None:
    """Perform a lightweight schema validation for JSON output."""
    required_numeric_fields = {
        "samples",
        "time_start",
        "time_end",
        "duration",
        "max_condition",
        "mean_condition",
        "median_condition",
        "warn_frames",
        "warn_ratio",
        "rank_frames",
        "rank_ratio",
        "max_distance_abs",
        "max_angle_abs",
    }
    required_dict_fields = {
        "eq_force_distance_max",
        "eq_force_angle_max",
        "eq_impulse_max",
    }

    missing = [key for key in (*required_numeric_fields, *required_dict_fields, "eq_ids", "version") if key not in payload]
    if missing:
        raise ValueError(f"Summary payload missing fields: {', '.join(sorted(missing))}")

    version = payload["version"]
    if not isinstance(version, int):
        raise ValueError(f"Field 'version' must be an integer, got {type(version).__name__}")
    if version > COUPLED_SUMMARY_VERSION:
        raise ValueError(
            f"Summary version {version} is newer than supported {COUPLED_SUMMARY_VERSION}."
        )

    for key in required_numeric_fields:
        value = payload[key]
        if not isinstance(value, _Number):
            raise ValueError(f"Field '{key}' must be numeric, got {type(value).__name__}")
        if key in {"samples", "warn_frames", "rank_frames"} and not isinstance(value, int):
            raise ValueError(f"Field '{key}' must be an integer, got {type(value).__name__}")

    if not isinstance(payload["eq_ids"], list):
        raise ValueError("Field 'eq_ids' must be a list.")

    for eq_label in payload["eq_ids"]:
        if not isinstance(eq_label, str) or not eq_label.startswith("eq"):
            raise ValueError(f"eq_ids entry '{eq_label}' must be a string starting with 'eq'.")

    for dict_key in required_dict_fields:
        mapping = payload[dict_key]
        if not isinstance(mapping, dict):
            raise ValueError(f"Field '{dict_key}' must be an object.")
        for eq_label, value in mapping.items():
            if not isinstance(eq_label, str) or not eq_label.startswith("eq"):
                raise ValueError(f"Key '{eq_label}' in '{dict_key}' must start with 'eq'.")
            if not isinstance(value, _Number):
                raise ValueError(
                    f"Value for '{eq_label}' in '{dict_key}' must be numeric, got {type(value).__name__}"
                )

def _parse_stage_markers(raw_markers):
    markers = []
    if not raw_markers:
        return markers
    for raw in raw_markers:
        if ":" in raw:
            value_str, label = raw.split(":", 1)
        else:
            value_str, label = raw, ""
        try:
            time_value = float(value_str)
        except ValueError as exc:
            raise ValueError(f"Invalid stage marker '{raw}': time component must be numeric.") from exc
        markers.append({"time": time_value, "label": label})
    return markers


def _apply_stage_markers(axes, stage_markers):
    if not stage_markers:
        return
    colors = ["#7d3c98", "#c0392b", "#16a085", "#f39c12"]
    for idx, marker in enumerate(stage_markers):
        color = colors[idx % len(colors)]
        label = marker["label"] if marker["label"] else f"stage@{marker['time']:.3f}s"
        for ax_index, ax in enumerate(axes):
            line_label = label if ax_index == 0 else None
            ax.axvline(marker["time"], color=color, linestyle=":", linewidth=1.1, alpha=0.7, label=line_label)
        if label:
            axes[0].legend(loc="lower right")


def plot(data: dict, stage_markers=None):
    plt = _import_pyplot()
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

    _apply_stage_markers(axes, stage_markers or [])
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

    if args.fail_on_warning_ratio is not None and not 0.0 <= args.fail_on_warning_ratio <= 1.0:
        print("Error: --fail-on-warning-ratio expects a value between 0.0 and 1.0.", file=sys.stderr)
        return 2
    if args.fail_on_warning_ratio_below is not None and not 0.0 <= args.fail_on_warning_ratio_below <= 1.0:
        print("Error: --fail-on-warning-ratio-below expects a value between 0.0 and 1.0.", file=sys.stderr)
        return 2
    if args.fail_on_rank_ratio is not None and not 0.0 <= args.fail_on_rank_ratio <= 1.0:
        print("Error: --fail-on-rank-ratio expects a value between 0.0 and 1.0.", file=sys.stderr)
        return 2
    if args.fail_on_rank_ratio_below is not None and not 0.0 <= args.fail_on_rank_ratio_below <= 1.0:
        print("Error: --fail-on-rank-ratio-below expects a value between 0.0 and 1.0.", file=sys.stderr)
        return 2

    summary = compute_summary(data)
    print(summary_as_plain_text(summary))

    if args.summary_md:
        summary_md_path = Path(args.summary_md).expanduser()
        summary_md_path.parent.mkdir(parents=True, exist_ok=True)
        summary_md_path.write_text(summary_as_markdown(summary) + "\n", encoding="utf-8")
        print(f"Wrote Markdown summary to {summary_md_path}")

    if args.summary_html:
        summary_html_path = Path(args.summary_html).expanduser()
        summary_html_path.parent.mkdir(parents=True, exist_ok=True)
        summary_html_path.write_text(summary_as_html(summary), encoding="utf-8")
        print(f"Wrote HTML summary to {summary_html_path}")

    summary_payload = None

    if args.summary_json:
        summary_json_path = Path(args.summary_json).expanduser()
        summary_json_path.parent.mkdir(parents=True, exist_ok=True)
        if summary_payload is None:
            summary_payload = summary.to_dict()
        try:
            _validate_summary_schema(summary_payload)
        except ValueError as exc:
            print(f"Invalid summary schema: {exc}", file=sys.stderr)
            return 2
        summary_json_path.write_text(
            json.dumps(summary_payload, indent=2) + "\n",
            encoding="utf-8",
        )
        print(f"Wrote JSON summary to {summary_json_path}")

    violation_exit_code = 0
    if args.fail_on_max_condition is not None and summary.max_condition > args.fail_on_max_condition:
        print(
            f"Max condition number {summary.max_condition:.3e} exceeds threshold {args.fail_on_max_condition:.3e}",
            file=sys.stderr,
        )
        violation_exit_code = max(violation_exit_code, 3)

    if args.fail_on_warning_ratio is not None and summary.warn_ratio > args.fail_on_warning_ratio:
        print(
            f"Warning ratio {summary.warn_ratio:.3f} exceeds threshold {args.fail_on_warning_ratio:.3f}",
            file=sys.stderr,
        )
        violation_exit_code = max(violation_exit_code, 4)

    if args.fail_on_rank_ratio is not None and summary.rank_ratio > args.fail_on_rank_ratio:
        print(
            f"Rank-deficient ratio {summary.rank_ratio:.3f} exceeds threshold {args.fail_on_rank_ratio:.3f}",
            file=sys.stderr,
        )
        violation_exit_code = max(violation_exit_code, 5)

    if args.fail_on_warning_ratio_below is not None and summary.warn_ratio < args.fail_on_warning_ratio_below:
        print(
            f"Warning ratio {summary.warn_ratio:.3f} is below lower bound {args.fail_on_warning_ratio_below:.3f}",
            file=sys.stderr,
        )
        violation_exit_code = max(violation_exit_code, 6)

    if args.fail_on_rank_ratio_below is not None and summary.rank_ratio < args.fail_on_rank_ratio_below:
        print(
            f"Rank-deficient ratio {summary.rank_ratio:.3f} is below lower bound {args.fail_on_rank_ratio_below:.3f}",
            file=sys.stderr,
        )
        violation_exit_code = max(violation_exit_code, 7)

    if args.skip_plot:
        if args.output:
            print("Error: --skip-plot cannot be combined with --output.", file=sys.stderr)
            return 2
        if args.show:
            print("Error: --skip-plot cannot be combined with --show.", file=sys.stderr)
            return 2
        return violation_exit_code

    try:
        stage_markers = _parse_stage_markers(args.stage_markers)
    except ValueError as exc:
        print(str(exc), file=sys.stderr)
        return 2

    try:
        fig, _ = plot(data, stage_markers=stage_markers)
    except RuntimeError as exc:
        print(str(exc), file=sys.stderr)
        return 1

    if args.output:
        output_path = Path(args.output).expanduser()
        output_path.parent.mkdir(parents=True, exist_ok=True)
        fig.savefig(output_path, dpi=args.dpi, bbox_inches="tight")
        print(f"Saved figure to {output_path}")

    should_show = args.show or (not args.no_show and not args.output)
    if should_show:
        plt = _import_pyplot()
        plt.show()
    else:
        plt = _import_pyplot()
        plt.close(fig)

    return violation_exit_code


if __name__ == "__main__":
    sys.exit(main())
