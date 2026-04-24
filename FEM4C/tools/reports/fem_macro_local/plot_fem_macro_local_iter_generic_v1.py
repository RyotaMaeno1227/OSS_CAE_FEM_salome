#!/usr/bin/env python3
import csv
import importlib.util
import json
import math
import shutil
import subprocess
import sys
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[3]
LOCAL_PLOT_SCRIPT = ROOT_DIR / "tools" / "reports" / "local_patch" / "plot_local_patch_generic_solver_v1.py"
if not LOCAL_PLOT_SCRIPT.is_file():
    LOCAL_PLOT_SCRIPT = ROOT_DIR / "scripts" / "plot_local_patch_generic_solver_v1.py"


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


def load_json(path: Path) -> dict:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        fail(f"missing JSON file: {path}")
    except json.JSONDecodeError as exc:
        fail(f"invalid JSON file {path}: {exc}")
    if not isinstance(data, dict):
        fail(f"{path} must contain a JSON object")
    return data


def load_csv(path: Path) -> list[dict[str, str]]:
    try:
        with path.open(newline="", encoding="utf-8") as handle:
            rows = list(csv.DictReader(handle))
    except FileNotFoundError:
        fail(f"missing CSV file: {path}")
    if not rows:
        fail(f"CSV has no data rows: {path}")
    return rows


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


def parse_int(row: dict[str, str], field: str, path: Path) -> int:
    value = parse_float(row, field, path)
    if abs(value - round(value)) > 1.0e-9:
        fail(f"{path} field {field} must be integer-like, got {value!r}")
    return int(round(value))


def load_local_plot_module():
    spec = importlib.util.spec_from_file_location("local_plot_v1", LOCAL_PLOT_SCRIPT)
    if spec is None or spec.loader is None:
        fail(f"unable to load local plot module from {LOCAL_PLOT_SCRIPT}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


LOCAL_PLOT = load_local_plot_module()


def draw_axes(pixels, width: int, height: int, left: int, top: int, plot_w: int, plot_h: int) -> None:
    LOCAL_PLOT.draw_line(pixels, width, height, left, top, left, top + plot_h, (0, 0, 0))
    LOCAL_PLOT.draw_line(
        pixels,
        width,
        height,
        left,
        top + plot_h,
        left + plot_w,
        top + plot_h,
        (0, 0, 0),
    )


def draw_series(
    pixels,
    width: int,
    height: int,
    left: int,
    top: int,
    plot_w: int,
    plot_h: int,
    values: list[float],
    color: tuple[int, int, int],
) -> None:
    if not values:
        return
    vmax = max(values)
    vmin = min(values)
    span = max(vmax - vmin, 1.0e-16)
    denom_x = max(1, len(values) - 1)
    prev = None
    for idx, value in enumerate(values):
        x = int(left + idx / denom_x * plot_w)
        y = int(top + plot_h - ((value - vmin) / span) * plot_h)
        if prev is not None:
            LOCAL_PLOT.draw_line(pixels, width, height, prev[0], prev[1], x, y, color)
        LOCAL_PLOT.draw_rect(pixels, width, height, x - 2, y - 2, x + 3, y + 3, color)
        prev = (x, y)


def draw_tick_labels(
    pixels,
    width: int,
    height: int,
    left: int,
    top: int,
    plot_w: int,
    plot_h: int,
    labels: list[str],
) -> None:
    denom_x = max(1, len(labels) - 1)
    for idx, label in enumerate(labels):
        x = int(left + idx / denom_x * plot_w)
        LOCAL_PLOT.draw_line(pixels, width, height, x, top + plot_h, x, top + plot_h + 8, (0, 0, 0))
        LOCAL_PLOT.draw_text(pixels, width, height, x - 10, top + plot_h + 12, label, (0, 0, 0), 1)


def compute_gamma_series(run_dir: Path, actual_iter_count: int) -> list[dict[str, float]]:
    rows: list[dict[str, float]] = []
    for iter_index in range(actual_iter_count):
        iter_tag = f"iter{iter_index:03d}"
        iter_dir = run_dir / iter_tag
        raw_csv = iter_dir / "exported_feedback" / "local_feedback_reduced_raw.csv"
        used_csv = iter_dir / "exported_feedback" / "local_feedback_reduced.csv"
        raw_rows = load_csv(raw_csv)
        used_rows = load_csv(used_csv)
        if len(raw_rows) != len(used_rows):
            fail(f"row count mismatch between {raw_csv} and {used_csv}")
        raw_values = [parse_float(row, "gamma_n", raw_csv) for row in raw_rows]
        used_values = [parse_float(row, "gamma_n", used_csv) for row in used_rows]
        rows.append(
            {
                "iter_index": iter_index,
                "feedback_row_count": len(raw_rows),
                "raw_gamma_n_mean": sum(raw_values) / len(raw_values),
                "raw_gamma_n_min": min(raw_values),
                "raw_gamma_n_max": max(raw_values),
                "used_gamma_n_mean": sum(used_values) / len(used_values),
                "used_gamma_n_min": min(used_values),
                "used_gamma_n_max": max(used_values),
            }
        )
    return rows


def plot_iteration_history(
    run_dir: Path,
    history_rows: list[dict[str, str]],
    summary: dict,
    gamma_rows: list[dict[str, float]],
) -> Path:
    width, height = 1280, 1120
    pixels = LOCAL_PLOT.make_canvas(width, height, (255, 255, 255))
    plots = [
        (80, 120, 500, 180, "ABS GAMMA DIFF", [float(row["max_abs_gamma_n_diff"]) for row in history_rows], (25, 100, 210)),
        (680, 120, 500, 180, "REL GAMMA DIFF", [float(row["max_rel_gamma_n_diff"]) for row in history_rows], (210, 90, 40)),
        (80, 420, 500, 180, "RESPONSE CHANGED", [float(row["response_changed_rows"]) for row in history_rows], (30, 150, 90)),
        (680, 420, 500, 180, "GAMMA SHIFT ROWS", [float(row["gamma_shift_rows"]) for row in history_rows], (190, 60, 150)),
        (80, 720, 1100, 220, "RAW AND USED GAMMA", [], (0, 0, 0)),
    ]
    LOCAL_PLOT.draw_text(pixels, width, height, 70, 24, "ITER HISTORY STATIC COUPLING", (0, 0, 0), 3)
    LOCAL_PLOT.draw_text(
        pixels,
        width,
        height,
        70,
        66,
        f"CONVERGED {str(summary['converged']).upper()}  ACTUAL ITER {summary['actual_iter_count']}",
        (0, 0, 0),
        2,
    )
    LOCAL_PLOT.draw_text(
        pixels,
        width,
        height,
        70,
        972,
        "USED RELAXED FEEDBACK CSV DRIVES CONVERGENCE",
        (0, 0, 0),
        2,
    )
    LOCAL_PLOT.draw_text(
        pixels,
        width,
        height,
        70,
        1004,
        "RAW LOCAL GAMMA IS REVIEW ONLY",
        (170, 30, 30),
        2,
    )

    iter_labels = [str(int(row["iter_index"])) for row in history_rows]
    for left, top, plot_w, plot_h, title, values, color in plots[:4]:
        draw_axes(pixels, width, height, left, top, plot_w, plot_h)
        draw_series(pixels, width, height, left, top, plot_w, plot_h, values, color)
        draw_tick_labels(pixels, width, height, left, top, plot_w, plot_h, iter_labels)
        LOCAL_PLOT.draw_text(pixels, width, height, left, top - 36, title, (0, 0, 0), 2)

    left, top, plot_w, plot_h, title, _, _ = plots[4]
    draw_axes(pixels, width, height, left, top, plot_w, plot_h)
    raw_values = [row["raw_gamma_n_mean"] for row in gamma_rows]
    used_values = [row["used_gamma_n_mean"] for row in gamma_rows]
    combined = raw_values + used_values
    vmax = max(combined)
    vmin = min(combined)
    span = max(vmax - vmin, 1.0e-16)
    denom_x = max(1, len(gamma_rows) - 1)
    prev_raw = None
    prev_used = None
    for idx, gamma_row in enumerate(gamma_rows):
        x = int(left + idx / denom_x * plot_w)
        raw_y = int(top + plot_h - ((gamma_row["raw_gamma_n_mean"] - vmin) / span) * plot_h)
        used_y = int(top + plot_h - ((gamma_row["used_gamma_n_mean"] - vmin) / span) * plot_h)
        if prev_raw is not None:
            LOCAL_PLOT.draw_line(pixels, width, height, prev_raw[0], prev_raw[1], x, raw_y, (170, 30, 30))
        if prev_used is not None:
            LOCAL_PLOT.draw_line(pixels, width, height, prev_used[0], prev_used[1], x, used_y, (30, 100, 210))
        LOCAL_PLOT.draw_rect(pixels, width, height, x - 2, raw_y - 2, x + 3, raw_y + 3, (170, 30, 30))
        LOCAL_PLOT.draw_rect(pixels, width, height, x - 2, used_y - 2, x + 3, used_y + 3, (30, 100, 210))
        prev_raw = (x, raw_y)
        prev_used = (x, used_y)
    draw_tick_labels(pixels, width, height, left, top, plot_w, plot_h, iter_labels)
    LOCAL_PLOT.draw_text(pixels, width, height, left, top - 36, title, (0, 0, 0), 2)
    LOCAL_PLOT.draw_text(pixels, width, height, left + 760, top - 12, "RAW", (170, 30, 30), 2)
    LOCAL_PLOT.draw_text(pixels, width, height, left + 860, top - 12, "USED", (30, 100, 210), 2)

    out_path = run_dir / "iteration_history.png"
    LOCAL_PLOT.write_png(out_path, width, height, pixels)
    return out_path


def plot_baseline_vs_final(run_dir: Path, summary: dict) -> Path:
    baseline_trace = run_dir / "baseline" / "baseline_generic.out.fem_contact_generic_trace.csv"
    final_replay_trace = Path(summary["final_replay_trace_csv"]).resolve()
    baseline_rows = load_csv(baseline_trace)
    final_rows = load_csv(final_replay_trace)
    baseline_map = {
        (
            parse_int(row, "load_step", baseline_trace),
            parse_int(row, "pair_id", baseline_trace),
            parse_int(row, "slave_node_id", baseline_trace),
            parse_int(row, "master_segment_id", baseline_trace),
        ): row
        for row in baseline_rows
    }
    final_pairs: list[tuple[tuple[int, int, int, int], float, float, float, float]] = []
    for row in final_rows:
        key = (
            parse_int(row, "load_step", final_replay_trace),
            parse_int(row, "pair_id", final_replay_trace),
            parse_int(row, "slave_node_id", final_replay_trace),
            parse_int(row, "master_segment_id", final_replay_trace),
        )
        base = baseline_map.get(key)
        if base is None:
            fail(f"missing baseline row for {key!r}")
        final_pairs.append(
            (
                key,
                parse_float(base, "fn_n", baseline_trace),
                parse_float(row, "fn_n", final_replay_trace),
                parse_float(base, "penetration_m", baseline_trace),
                parse_float(row, "penetration_m", final_replay_trace),
            )
        )
    final_pairs.sort(key=lambda item: item[0])
    width, height = 1200, 900
    pixels = LOCAL_PLOT.make_canvas(width, height, (255, 255, 255))
    LOCAL_PLOT.draw_text(pixels, width, height, 60, 24, "BASELINE REPLAY COMPARE", (0, 0, 0), 3)
    LOCAL_PLOT.draw_text(pixels, width, height, 60, 66, "FIRST CLASS FEEDBACK IS GAMMA ONLY", (0, 0, 0), 2)
    LOCAL_PLOT.draw_text(pixels, width, height, 60, 98, "ROW KEY LOADSTEP PAIR NODE SEGMENT", (0, 0, 0), 2)

    fn_left, fn_top, fn_w, fn_h = 80, 180, 480, 540
    pen_left, pen_top, pen_w, pen_h = 660, 180, 480, 540
    draw_axes(pixels, width, height, fn_left, fn_top, fn_w, fn_h)
    draw_axes(pixels, width, height, pen_left, pen_top, pen_w, pen_h)
    LOCAL_PLOT.draw_text(pixels, width, height, fn_left, fn_top - 36, "FN BASELINE REPLAY", (0, 0, 0), 2)
    LOCAL_PLOT.draw_text(pixels, width, height, pen_left, pen_top - 36, "PEN BASELINE REPLAY", (0, 0, 0), 2)

    fn_max = max(max(base_fn, replay_fn) for _, base_fn, replay_fn, _, _ in final_pairs)
    pen_max = max(max(base_pen, replay_pen) for _, _, _, base_pen, replay_pen in final_pairs)
    count = len(final_pairs)
    slot_w = fn_w / max(1, count)
    for idx, (_, base_fn, replay_fn, base_pen, replay_pen) in enumerate(final_pairs):
        x0 = int(fn_left + idx * slot_w + 20)
        mid = x0 + 50
        x1 = x0 + 100
        fn_base_h = int((base_fn / max(fn_max, 1.0e-16)) * (fn_h - 40))
        fn_replay_h = int((replay_fn / max(fn_max, 1.0e-16)) * (fn_h - 40))
        pen_base_h = int((base_pen / max(pen_max, 1.0e-16)) * (pen_h - 40))
        pen_replay_h = int((replay_pen / max(pen_max, 1.0e-16)) * (pen_h - 40))
        pen_x0 = int(pen_left + idx * slot_w + 20)
        pen_mid = pen_x0 + 50
        pen_x1 = pen_x0 + 100
        LOCAL_PLOT.draw_rect(pixels, width, height, x0, fn_top + fn_h - fn_base_h, mid - 8, fn_top + fn_h, (140, 140, 140))
        LOCAL_PLOT.draw_rect(pixels, width, height, mid, fn_top + fn_h - fn_replay_h, x1, fn_top + fn_h, (40, 120, 210))
        LOCAL_PLOT.draw_rect(pixels, width, height, pen_x0, pen_top + pen_h - pen_base_h, pen_mid - 8, pen_top + pen_h, (140, 140, 140))
        LOCAL_PLOT.draw_rect(pixels, width, height, pen_mid, pen_top + pen_h - pen_replay_h, pen_x1, pen_top + pen_h, (40, 120, 210))
        LOCAL_PLOT.draw_text(pixels, width, height, x0 + 8, fn_top + fn_h + 14, str(idx), (0, 0, 0), 1)
        LOCAL_PLOT.draw_text(pixels, width, height, pen_x0 + 8, pen_top + pen_h + 14, str(idx), (0, 0, 0), 1)

    LOCAL_PLOT.draw_text(pixels, width, height, 80, 760, "GRAY BASELINE", (80, 80, 80), 2)
    LOCAL_PLOT.draw_text(pixels, width, height, 280, 760, "BLUE FINAL REPLAY", (40, 120, 210), 2)
    out_path = run_dir / "baseline_vs_final_replay.png"
    LOCAL_PLOT.write_png(out_path, width, height, pixels)
    return out_path


def build_plot_summary(run_dir: Path, summary: dict, gamma_rows: list[dict[str, float]]) -> tuple[Path, Path]:
    final_iter_dir = run_dir / summary["final_iter_tag"]
    oneway_rows_csv = final_iter_dir / "oneway" / "local_oneway_rows.csv"
    oneway_rows = load_csv(oneway_rows_csv)
    representative = None
    for row in oneway_rows:
        if parse_int(row, "valid_flag", oneway_rows_csv) == 1:
            representative = row
            break
    if representative is None:
        representative = oneway_rows[0]

    response_rel = representative["response_path"]
    response_dir = final_iter_dir / "oneway" / Path(response_rel).parent
    subprocess.run([sys.executable, str(LOCAL_PLOT_SCRIPT), str(response_dir)], check=True)
    src_png = response_dir / "pressure_field_heatmap.png"
    if not src_png.is_file():
        fail(f"missing representative local pressure plot: {src_png}")
    dst_png = run_dir / "representative_local_pressure_field.png"
    shutil.copyfile(src_png, dst_png)

    gamma_csv = run_dir / "gamma_evolution.csv"
    with gamma_csv.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(
            handle,
            fieldnames=[
                "iter_index",
                "feedback_row_count",
                "raw_gamma_n_mean",
                "raw_gamma_n_min",
                "raw_gamma_n_max",
                "used_gamma_n_mean",
                "used_gamma_n_min",
                "used_gamma_n_max",
            ],
        )
        writer.writeheader()
        writer.writerows(gamma_rows)

    summary_json = run_dir / "plot_summary.json"
    payload = {
        "iteration_history_png": str(run_dir / "iteration_history.png"),
        "baseline_vs_final_replay_png": str(run_dir / "baseline_vs_final_replay.png"),
        "representative_local_pressure_field_png": str(dst_png),
        "gamma_evolution_csv": str(gamma_csv),
        "convergence_truth": "converged=true is based on used/relaxed feedback CSV local_feedback_reduced.csv; it does not claim raw local gamma_n fixed-point convergence",
        "representative_row": {
            "load_step": parse_int(representative, "load_step", oneway_rows_csv),
            "pair_id": parse_int(representative, "pair_id", oneway_rows_csv),
            "slave_node_id": parse_int(representative, "slave_node_id", oneway_rows_csv),
            "master_segment_id": parse_int(representative, "master_segment_id", oneway_rows_csv),
            "status": representative["status"],
            "valid_flag": parse_int(representative, "valid_flag", oneway_rows_csv),
            "response_dir": str(response_dir),
        },
    }
    summary_json.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return dst_png, summary_json


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit(
            "Usage: python3 tools/reports/fem_macro_local/plot_fem_macro_local_iter_generic_v1.py <iter_outdir>"
        )

    run_dir = Path(sys.argv[1]).resolve()
    summary = load_json(run_dir / "iter_summary.json")
    history_rows = load_csv(run_dir / "iteration_history.csv")
    actual_iter_count = int(summary["actual_iter_count"])
    gamma_rows = compute_gamma_series(run_dir, actual_iter_count)

    iteration_png = plot_iteration_history(run_dir, history_rows, summary, gamma_rows)
    compare_png = plot_baseline_vs_final(run_dir, summary)
    representative_png, plot_summary_json = build_plot_summary(run_dir, summary, gamma_rows)

    print(
        "PASS fem_macro_local_iter_generic_visualization_v1 "
        f"actual_iter_count={actual_iter_count} "
        f"iteration_history_png={iteration_png} "
        f"baseline_vs_final_replay_png={compare_png} "
        f"representative_local_pressure_field_png={representative_png} "
        f"plot_summary_json={plot_summary_json}"
    )


if __name__ == "__main__":
    main()
