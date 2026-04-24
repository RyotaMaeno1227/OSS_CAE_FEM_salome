#!/usr/bin/env python3
import csv
import json
import math
import sys
from pathlib import Path


ROW_KEY_FIELDS = ("load_step", "pair_id")


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


def load_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    if not rows:
        fail(f"{path} has no data rows")
    return rows


def parse_int(row: dict[str, str], field: str, path: Path) -> int:
    try:
        value = float(row[field])
    except KeyError:
        fail(f"{path} missing field {field}")
    except ValueError as exc:
        fail(f"{path} field {field} invalid integer-like value: {exc}")
    if not math.isfinite(value):
        fail(f"{path} field {field} must be finite")
    if abs(value - round(value)) > 1.0e-9:
        fail(f"{path} field {field} must be integer-like, got {value!r}")
    return int(round(value))


def parse_float(row: dict[str, str], field: str, path: Path) -> float:
    try:
        value = float(row[field])
    except KeyError:
        fail(f"{path} missing field {field}")
    except ValueError as exc:
        fail(f"{path} field {field} invalid float value: {exc}")
    if not math.isfinite(value):
        fail(f"{path} field {field} must be finite")
    return value


def build_map(path: Path) -> dict[tuple[int, int], dict[str, str]]:
    rows = load_rows(path)
    mapped: dict[tuple[int, int], dict[str, str]] = {}
    for row in rows:
        key = tuple(parse_int(row, field, path) for field in ROW_KEY_FIELDS)
        if key in mapped:
            fail(f"{path} duplicate row key {key!r}")
        mapped[key] = row
    return mapped


def main() -> None:
    if len(sys.argv) not in {4, 6}:
        raise SystemExit(
            "Usage: python3 scripts/compare_fem_macro_local_iterations.py "
            "<prev_feedback.csv> <curr_feedback.csv> <out.json> [tol_abs tol_rel]"
        )

    prev_path = Path(sys.argv[1]).resolve()
    curr_path = Path(sys.argv[2]).resolve()
    out_path = Path(sys.argv[3]).resolve()
    tol_abs = float(sys.argv[4]) if len(sys.argv) == 6 else 1.0e-6
    tol_rel = float(sys.argv[5]) if len(sys.argv) == 6 else 1.0e-6
    if tol_abs < 0.0 or tol_rel < 0.0:
        fail("tol_abs and tol_rel must be >= 0")

    prev_map = build_map(prev_path)
    curr_map = build_map(curr_path)

    prev_keys = set(prev_map)
    curr_keys = set(curr_map)
    common_keys = sorted(prev_keys & curr_keys)

    max_abs_gamma_n_diff = 0.0
    max_rel_gamma_n_diff = 0.0
    for key in common_keys:
        prev_gamma = parse_float(prev_map[key], "gamma_n", prev_path)
        curr_gamma = parse_float(curr_map[key], "gamma_n", curr_path)
        abs_diff = abs(curr_gamma - prev_gamma)
        rel_diff = abs_diff / max(abs(prev_gamma), 1.0e-12)
        max_abs_gamma_n_diff = max(max_abs_gamma_n_diff, abs_diff)
        max_rel_gamma_n_diff = max(max_rel_gamma_n_diff, rel_diff)

    row_key_match = prev_keys == curr_keys
    converged = row_key_match and (
        max_abs_gamma_n_diff <= tol_abs or max_rel_gamma_n_diff <= tol_rel
    )

    payload = {
        "row_key": list(ROW_KEY_FIELDS),
        "prev_feedback_csv": str(prev_path),
        "curr_feedback_csv": str(curr_path),
        "prev_row_count": len(prev_map),
        "curr_row_count": len(curr_map),
        "common_row_count": len(common_keys),
        "row_key_match": row_key_match,
        "missing_in_curr_count": len(prev_keys - curr_keys),
        "missing_in_prev_count": len(curr_keys - prev_keys),
        "max_abs_gamma_n_diff": max_abs_gamma_n_diff,
        "max_rel_gamma_n_diff": max_rel_gamma_n_diff,
        "tol_abs": tol_abs,
        "tol_rel": tol_rel,
        "converged": converged,
    }
    out_path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(
        "PASS compare_fem_macro_local_iterations "
        f"prev_row_count={payload['prev_row_count']} "
        f"curr_row_count={payload['curr_row_count']} "
        f"row_key_match={payload['row_key_match']} "
        f"max_abs_gamma_n_diff={payload['max_abs_gamma_n_diff']:.16e} "
        f"max_rel_gamma_n_diff={payload['max_rel_gamma_n_diff']:.16e} "
        f"converged={payload['converged']}"
    )


if __name__ == "__main__":
    main()
