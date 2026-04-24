#!/usr/bin/env python3
from __future__ import annotations

import csv
import json
import math
import sys
from pathlib import Path
from typing import Dict, List, Tuple


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def _float(row: Dict[str, str], key: str) -> float:
    value = row.get(key)
    _require(value is not None and value != "", f"missing CSV column: {key}")
    return float(value)


def _load_rows(path: Path) -> List[Dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as handle:
        rows = list(csv.DictReader(handle))
    _require(rows, f"empty history CSV: {path}")
    return rows


def _body_summary(rows: List[Dict[str, str]]) -> Dict[str, float]:
    initial = rows[0]
    x0 = _float(initial, "x")
    y0 = _float(initial, "y")
    theta0 = _float(initial, "theta")
    x_values = [_float(row, "x") for row in rows]
    y_values = [_float(row, "y") for row in rows]
    theta_values = [_float(row, "theta") for row in rows]
    max_displacement = max(math.hypot(x - x0, y - y0) for x, y in zip(x_values, y_values))
    final = rows[-1]
    return {
        "initial_x": x0,
        "initial_y": y0,
        "initial_theta": theta0,
        "final_x": _float(final, "x"),
        "final_y": _float(final, "y"),
        "final_theta": _float(final, "theta"),
        "max_displacement_from_initial": max_displacement,
        "theta_span": max(theta_values) - min(theta_values),
        "final_minus_initial_y": _float(final, "y") - y0,
        "final_minus_initial_theta": _float(final, "theta") - theta0,
    }


def main() -> int:
    if len(sys.argv) not in {2, 3}:
        raise SystemExit(
            "usage: scripts/analyze_mbd_2link_history.py <history.csv> [summary.json]"
        )

    history_path = Path(sys.argv[1]).resolve()
    summary_path = Path(sys.argv[2]).resolve() if len(sys.argv) == 3 else None
    rows = _load_rows(history_path)

    body_rows: Dict[int, List[Dict[str, str]]] = {}
    step_rows: Dict[int, Dict[int, Dict[str, str]]] = {}
    revolute_anchor_mismatch_max = 0.0
    revolute_radius_values: List[float] = []

    for row in rows:
        body_id = int(row["body_id"])
        step = int(row["step"])
        body_rows.setdefault(body_id, []).append(row)
        step_rows.setdefault(step, {})[body_id] = row
        if "revolute_anchor_mismatch_max" in row and row["revolute_anchor_mismatch_max"] != "":
            revolute_anchor_mismatch_max = max(
                revolute_anchor_mismatch_max,
                float(row["revolute_anchor_mismatch_max"]),
            )
        if "revolute_body_j_com_radius_max" in row and row["revolute_body_j_com_radius_max"] != "":
            revolute_radius_values.append(float(row["revolute_body_j_com_radius_max"]))

    _require(0 in body_rows and 1 in body_rows, "expected body_id 0 and 1 in history")

    body0 = _body_summary(body_rows[0])
    body1 = _body_summary(body_rows[1])

    com_distances: List[float] = []
    for step in sorted(step_rows):
        pair = step_rows[step]
        _require(0 in pair and 1 in pair, f"missing body rows at step {step}")
        com_distances.append(
            math.hypot(
                _float(pair[1], "x") - _float(pair[0], "x"),
                _float(pair[1], "y") - _float(pair[0], "y"),
            )
        )

    summary = {
        "history_csv": str(history_path),
        "step_count": len(step_rows),
        "body0": body0,
        "body1": body1,
        "body0_max_displacement_from_initial": body0["max_displacement_from_initial"],
        "body1_max_displacement_from_initial": body1["max_displacement_from_initial"],
        "body0_theta_span": body0["theta_span"],
        "body1_theta_span": body1["theta_span"],
        "body0_final_minus_initial_y": body0["final_minus_initial_y"],
        "body1_final_minus_initial_y": body1["final_minus_initial_y"],
        "body0_final_minus_initial_theta": body0["final_minus_initial_theta"],
        "body1_final_minus_initial_theta": body1["final_minus_initial_theta"],
        "body0_body1_com_distance_min": min(com_distances),
        "body0_body1_com_distance_max": max(com_distances),
        "revolute_anchor_mismatch_max": revolute_anchor_mismatch_max,
        "revolute_body_j_com_radius_min": min(revolute_radius_values) if revolute_radius_values else 0.0,
        "revolute_body_j_com_radius_max": max(revolute_radius_values) if revolute_radius_values else 0.0,
    }

    if summary_path is not None:
        summary_path.write_text(json.dumps(summary, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    print(f"history_csv={summary['history_csv']}")
    print(f"step_count={summary['step_count']}")
    print(f"body0 max displacement from initial = {summary['body0_max_displacement_from_initial']:.16e}")
    print(f"body1 max displacement from initial = {summary['body1_max_displacement_from_initial']:.16e}")
    print(f"body0 theta span = {summary['body0_theta_span']:.16e}")
    print(f"body1 theta span = {summary['body1_theta_span']:.16e}")
    print(
        "body0/body1 COM distance min/max = "
        f"{summary['body0_body1_com_distance_min']:.16e} / {summary['body0_body1_com_distance_max']:.16e}"
    )
    print(f"revolute anchor mismatch max = {summary['revolute_anchor_mismatch_max']:.16e}")
    print(
        "revolute body_j COM radius min/max = "
        f"{summary['revolute_body_j_com_radius_min']:.16e} / {summary['revolute_body_j_com_radius_max']:.16e}"
    )
    print(f"body0 final minus initial y = {summary['body0_final_minus_initial_y']:.16e}")
    print(f"body1 final minus initial y = {summary['body1_final_minus_initial_y']:.16e}")
    print(f"body0 final minus initial theta = {summary['body0_final_minus_initial_theta']:.16e}")
    print(f"body1 final minus initial theta = {summary['body1_final_minus_initial_theta']:.16e}")
    if summary_path is not None:
        print(f"summary_json={summary_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
