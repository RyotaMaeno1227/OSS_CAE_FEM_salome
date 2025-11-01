#!/usr/bin/env python3
"""
Shared helpers for coupled constraint endurance CSV processing.
"""

from __future__ import annotations

import csv
import html
import math
from dataclasses import asdict, dataclass
from pathlib import Path
from statistics import mean, median
from typing import Dict, List, Tuple, TypedDict

CHRONO_COUPLED_DIAG_RANK_DEFICIENT = 0x1
CHRONO_COUPLED_DIAG_CONDITION_WARNING = 0x2


class CoupledConstraintData(TypedDict):
    time: List[float]
    distance: List[float]
    angle: List[float]
    condition: List[float]
    flags: List[int]
    eq_ids: List[int]
    eq_force_distance: Dict[int, List[float]]
    eq_force_angle: Dict[int, List[float]]
    eq_impulse: Dict[int, List[float]]


@dataclass(frozen=True)
class CoupledConstraintSummary:
    samples: int
    time_start: float
    time_end: float
    duration: float
    max_condition: float
    mean_condition: float
    median_condition: float
    warn_frames: int
    warn_ratio: float
    rank_frames: int
    rank_ratio: float
    max_distance_abs: float
    max_angle_abs: float
    eq_ids: List[int]
    eq_force_distance_max: Dict[int, float]
    eq_force_angle_max: Dict[int, float]
    eq_impulse_max: Dict[int, float]

    def to_general_metrics(self) -> List[Tuple[str, str]]:
        lines = [
            ("Samples", str(self.samples)),
            (
                "Time range [s]",
                "n/a"
                if not self.samples
                else f"{self.time_start:.3f} - {self.time_end:.3f}",
            ),
            ("Duration [s]", f"{self.duration:.3f}"),
            ("Max condition number", f"{self.max_condition:.3e}"),
            ("Mean condition number", f"{self.mean_condition:.3e}"),
            ("Median condition number", f"{self.median_condition:.3e}"),
            (
                "Condition warnings",
                format_ratio(self.warn_frames, self.warn_ratio),
            ),
            (
                "Rank deficient frames",
                format_ratio(self.rank_frames, self.rank_ratio),
            ),
            ("Max |distance| [m]", f"{self.max_distance_abs:.6g}"),
            ("Max |angle| [rad]", f"{self.max_angle_abs:.6g}"),
        ]
        return lines

    def to_equation_metrics(self) -> List[Tuple[str, float, float, float]]:
        rows: List[Tuple[str, float, float, float]] = []
        for eq_id in self.eq_ids:
            rows.append(
                (
                    f"eq{eq_id}",
                    self.eq_force_distance_max.get(eq_id, 0.0),
                    self.eq_force_angle_max.get(eq_id, 0.0),
                    self.eq_impulse_max.get(eq_id, 0.0),
                )
            )
        return rows

    def to_dict(self) -> Dict[str, object]:
        """Return a JSON-serialisable view."""
        payload = asdict(self)
        # Ensure keys are friendly for JSON (convert int keys to str)
        payload["eq_force_distance_max"] = {
            f"eq{key}": value for key, value in self.eq_force_distance_max.items()
        }
        payload["eq_force_angle_max"] = {
            f"eq{key}": value for key, value in self.eq_force_angle_max.items()
        }
        payload["eq_impulse_max"] = {
            f"eq{key}": value for key, value in self.eq_impulse_max.items()
        }
        payload["eq_ids"] = [f"eq{key}" for key in self.eq_ids]
        return payload


def format_ratio(count: int, ratio: float) -> str:
    percentage = ratio * 100.0
    return f"{count} ({percentage:.2f}%)"


def default_csv_path() -> Path:
    project_root = Path(__file__).resolve().parent.parent
    return project_root / "data" / "coupled_constraint_endurance.csv"


def _collect_equation_indices(fieldnames: List[str], suffix: str) -> List[int]:
    indices: List[int] = []
    prefix = "eq"
    for name in fieldnames:
        if not name.startswith(prefix) or not name.endswith(suffix):
            continue
        middle = name[len(prefix) : -len(suffix)]
        if middle.isdigit():
            indices.append(int(middle))
    return sorted(set(indices))


def load_csv(path: Path) -> CoupledConstraintData:
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


def compute_summary(data: CoupledConstraintData) -> CoupledConstraintSummary:
    time = data["time"]
    distance = data["distance"]
    angle = data["angle"]
    condition = data["condition"]
    flags = data["flags"]
    eq_ids = data["eq_ids"]
    eq_force_distance = data["eq_force_distance"]
    eq_force_angle = data["eq_force_angle"]
    eq_impulse = data["eq_impulse"]

    samples = len(time)
    time_start = time[0] if samples else 0.0
    time_end = time[-1] if samples else 0.0
    duration = time_end - time_start if samples > 1 else 0.0
    max_condition = max(condition) if condition else 0.0

    # Use statistics helpers with defensive guards
    mean_condition = mean(condition) if condition else 0.0
    median_condition = median(condition) if condition else 0.0

    warn_frames = sum(1 for flag in flags if flag & CHRONO_COUPLED_DIAG_CONDITION_WARNING)
    rank_frames = sum(1 for flag in flags if flag & CHRONO_COUPLED_DIAG_RANK_DEFICIENT)
    warn_ratio = warn_frames / samples if samples else 0.0
    rank_ratio = rank_frames / samples if samples else 0.0

    max_distance_abs = max(map(abs, distance), default=0.0)
    max_angle_abs = max(map(abs, angle), default=0.0)

    eq_force_distance_max = {
        idx: max(map(abs, values), default=0.0) for idx, values in eq_force_distance.items()
    }
    eq_force_angle_max = {
        idx: max(map(abs, values), default=0.0) for idx, values in eq_force_angle.items()
    }
    eq_impulse_max = {
        idx: max(map(abs, values), default=0.0) for idx, values in eq_impulse.items()
    }

    return CoupledConstraintSummary(
        samples=samples,
        time_start=time_start,
        time_end=time_end,
        duration=duration,
        max_condition=max_condition,
        mean_condition=mean_condition,
        median_condition=median_condition,
        warn_frames=warn_frames,
        warn_ratio=warn_ratio,
        rank_frames=rank_frames,
        rank_ratio=rank_ratio,
        max_distance_abs=max_distance_abs,
        max_angle_abs=max_angle_abs,
        eq_ids=list(eq_ids),
        eq_force_distance_max=eq_force_distance_max,
        eq_force_angle_max=eq_force_angle_max,
        eq_impulse_max=eq_impulse_max,
    )


def summary_as_plain_text(summary: CoupledConstraintSummary) -> str:
    lines = [
        f"Samples: {summary.samples}",
        f"Time range: "
        + (
            f"{summary.time_start:.3f}s - {summary.time_end:.3f}s (duration {summary.duration:.3f}s)"
            if summary.samples
            else "n/a"
        ),
        f"Max condition number: {summary.max_condition:.3e}",
        f"Mean condition number: {summary.mean_condition:.3e}",
        f"Median condition number: {summary.median_condition:.3e}",
        f"Condition warnings: {format_ratio(summary.warn_frames, summary.warn_ratio)}",
        f"Rank deficient frames: {format_ratio(summary.rank_frames, summary.rank_ratio)}",
        f"Max |distance|: {summary.max_distance_abs:.6g}",
        f"Max |angle|: {summary.max_angle_abs:.6g}",
    ]

    for eq_label, f_dist, f_ang, impulse in summary.to_equation_metrics():
        lines.append(
            f"{eq_label}: max|force_distance|={f_dist:.3e}, "
            f"max|force_angle|={f_ang:.3e}, max|impulse|={impulse:.3e}"
        )

    return "\n".join(lines)


def summary_as_markdown(summary: CoupledConstraintSummary) -> str:
    lines = ["# Coupled Constraint Endurance Summary", ""]
    lines.append("| Metric | Value |")
    lines.append("| --- | --- |")
    for metric, value in summary.to_general_metrics():
        lines.append(f"| {metric} | {value} |")

    eq_rows = summary.to_equation_metrics()
    if eq_rows:
        lines.extend(["", "| Equation | max |force_distance| | max |force_angle| | max |impulse| |"])
        lines.append("| --- | --- | --- | --- |")
        for eq_label, f_dist, f_ang, impulse in eq_rows:
            lines.append(
                f"| {eq_label} | {f_dist:.3e} | {f_ang:.3e} | {impulse:.3e} |"
            )

    lines.append("")
    return "\n".join(lines)


def summary_as_html(summary: CoupledConstraintSummary) -> str:
    def esc(value: str) -> str:
        return html.escape(value, quote=True)

    parts = [
        "<h1>Coupled Constraint Endurance Summary</h1>",
        "<table>",
        "<thead><tr><th>Metric</th><th>Value</th></tr></thead>",
        "<tbody>",
    ]
    for metric, value in summary.to_general_metrics():
        parts.append(f"<tr><td>{esc(metric)}</td><td>{esc(value)}</td></tr>")
    parts.extend(["</tbody>", "</table>"])

    eq_rows = summary.to_equation_metrics()
    if eq_rows:
        parts.extend(
            [
                "<table>",
                "<thead><tr>"
                "<th>Equation</th><th>max |force_distance|</th>"
                "<th>max |force_angle|</th><th>max |impulse|</th>"
                "</tr></thead>",
                "<tbody>",
            ]
        )
        for eq_label, f_dist, f_ang, impulse in eq_rows:
            parts.append(
                "<tr>"
                f"<td>{esc(eq_label)}</td>"
                f"<td>{f_dist:.3e}</td>"
                f"<td>{f_ang:.3e}</td>"
                f"<td>{impulse:.3e}</td>"
                "</tr>"
            )
        parts.extend(["</tbody>", "</table>"])

    return "\n".join(parts)


__all__ = [
    "CHRONO_COUPLED_DIAG_RANK_DEFICIENT",
    "CHRONO_COUPLED_DIAG_CONDITION_WARNING",
    "CoupledConstraintData",
    "CoupledConstraintSummary",
    "compute_summary",
    "default_csv_path",
    "load_csv",
    "summary_as_html",
    "summary_as_markdown",
    "summary_as_plain_text",
]
