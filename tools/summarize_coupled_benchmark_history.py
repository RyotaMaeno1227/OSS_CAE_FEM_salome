#!/usr/bin/env python3
"""Summarise Coupled benchmark CSV history into Markdown/HTML tables."""

from __future__ import annotations

import argparse
import csv
import math
import statistics
from dataclasses import dataclass, field
from datetime import datetime, timezone
import json
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple


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


@dataclass
class RunRecord:
    path: Path
    timestamp: float
    rows: List[BenchRow] = field(default_factory=list)


@dataclass
class EqAggregate:
    eq_count: int
    sample_count: int
    avg_solve_mean: float
    avg_solve_max: float
    max_condition: float
    drop_mean: float
    unrecovered_mean: float


def discover_inputs(inputs: Sequence[str]) -> List[Path]:
    discovered: List[Path] = []
    for raw in inputs or ["data/coupled_benchmark_metrics.csv"]:
        path = Path(raw).expanduser()
        if path.is_dir():
            discovered.extend(sorted(path.glob("*.csv")))
        else:
            if any(char in raw for char in "*?[]"):
                discovered.extend(sorted(Path().glob(raw)))
            else:
                discovered.append(path)
    unique: List[Path] = []
    seen = set()
    for item in discovered:
        if item.exists() and item not in seen:
            unique.append(item)
            seen.add(item)
    return unique


def load_run(path: Path) -> RunRecord:
    with path.open("r", newline="") as handle:
        reader = csv.DictReader(handle)
        rows = [BenchRow.from_dict(row) for row in reader]
    return RunRecord(path=path, timestamp=path.stat().st_mtime, rows=rows)


def aggregate_by_eq(runs: Iterable[RunRecord]) -> List[EqAggregate]:
    bucket: Dict[int, List[BenchRow]] = {}
    for run in runs:
        for row in run.rows:
            bucket.setdefault(row.eq_count, []).append(row)

    aggregates: List[EqAggregate] = []
    for eq_count, rows in sorted(bucket.items()):
        avg_solve_samples = [row.avg_solve_time_us for row in rows]
        drop_samples = [row.drop_events for row in rows]
        unrecovered_samples = [row.unrecovered_drops for row in rows]
        aggregates.append(
            EqAggregate(
                eq_count=eq_count,
                sample_count=len(rows),
                avg_solve_mean=statistics.mean(avg_solve_samples) if avg_solve_samples else 0.0,
                avg_solve_max=max(avg_solve_samples) if avg_solve_samples else 0.0,
                max_condition=max(row.max_condition for row in rows) if rows else 0.0,
                drop_mean=statistics.mean(drop_samples) if drop_samples else 0.0,
                unrecovered_mean=statistics.mean(unrecovered_samples) if unrecovered_samples else 0.0,
            )
        )
    return aggregates


def compute_trend_metrics(runs: List[RunRecord]) -> List[Dict[str, Optional[float]]]:
    runs_sorted = sorted(runs, key=lambda run: run.timestamp)
    eq_ids = sorted({row.eq_count for run in runs_sorted for row in run.rows})
    metrics: List[Dict[str, Optional[float]]] = []
    for eq in eq_ids:
        series: List[Optional[float]] = []
        for run in runs_sorted:
            match = next((row for row in run.rows if row.eq_count == eq), None)
            series.append(match.max_condition if match else None)
        numeric_series = [value for value in series if value is not None]
        if not numeric_series:
            continue
        latest = numeric_series[-1]
        previous = numeric_series[-2] if len(numeric_series) >= 2 else None
        delta = latest - previous if previous is not None else None
        pct = (delta / previous * 100.0) if previous not in (None, 0.0) else None
        window = numeric_series[-3:]
        moving_avg = statistics.mean(window) if window else latest
        metrics.append(
            {
                "eq_count": eq,
                "latest": latest,
                "previous": previous,
                "delta": delta,
                "pct": pct,
                "moving_avg": moving_avg,
            }
        )
    return metrics


def render_markdown(
    runs: List[RunRecord],
    aggregates: List[EqAggregate],
    metadata: Optional[Dict[str, object]] = None,
) -> str:
    lines: List[str] = []
    lines.append("# Coupled Benchmark Report")
    lines.append("")
    if metadata:
        banner_parts = []
        generated = metadata.get("generated_at")
        if generated:
            banner_parts.append(f"Generated: {generated}")
        commit = metadata.get("git_commit")
        if commit:
            banner_parts.append(f"Commit: {commit}")
        config_url = metadata.get("config_url") or metadata.get("config_path")
        if config_url:
            banner_parts.append(f"Thresholds: {config_url}")
        if banner_parts:
            lines.append("> " + " | ".join(banner_parts))
            lines.append("")
    lines.append("## Aggregate by Equation Count")
    lines.append("| eq_count | samples | mean solve [µs] | max solve [µs] | max condition | mean drops | mean unrecovered |")
    lines.append("|---------:|--------:|----------------:|---------------:|--------------:|-----------:|-----------------:|")
    for agg in aggregates:
        lines.append(
            f"| {agg.eq_count} | {agg.sample_count} | {agg.avg_solve_mean:.3f} | {agg.avg_solve_max:.3f} | "
            f"{agg.max_condition:.3e} | {agg.drop_mean:.2f} | {agg.unrecovered_mean:.2f} |"
        )

    if runs:
        latest = max(runs, key=lambda run: run.timestamp)
        lines.append("")
        lines.append("## Latest Run Snapshot")
        lines.append(f"Source: `{latest.path}`")
        lines.append("")
        lines.append("| eq_count | epsilon | avg solve [µs] | max condition | drops | unrecovered |")
        lines.append("|---------:|--------:|----------------:|--------------:|------:|------------:|")
        for row in sorted(latest.rows, key=lambda r: (r.eq_count, r.epsilon)):
            lines.append(
                f"| {row.eq_count} | {row.epsilon:.1e} | {row.avg_solve_time_us:.3f} | {row.max_condition:.3e} | "
                f"{row.drop_events} | {row.unrecovered_drops} |"
            )

    if runs:
        lines.append("")
        lines.append("## Chronology")
        lines.append("| run | timestamp (UTC) | file |")
        lines.append("|----:|-----------------|------|")
        for index, run in enumerate(sorted(runs, key=lambda r: r.timestamp), start=1):
            stamp = datetime.fromtimestamp(run.timestamp, tz=timezone.utc).isoformat()
            lines.append(f"| {index} | {stamp} | `{run.path}` |")

    if runs:
        trend = compute_trend_metrics(runs)
        if trend:
            lines.append("")
            lines.append("## Condition Trend")
            lines.append("| eq_count | latest | previous | delta | pct change [%] | moving avg (last 3) |")
            lines.append("|---------:|-------:|---------:|------:|---------------:|------------------:|")
            for item in trend:
                prev = item["previous"]
                delta = item["delta"]
                pct = item["pct"]
                lines.append(
                    "| {eq_count} | {latest:.3e} | {prev} | {delta} | {pct} | {ma:.3e} |".format(
                        eq_count=item["eq_count"],
                        latest=item["latest"],
                        prev=f"{prev:.3e}" if prev is not None else "-",
                        delta=f"{delta:.3e}" if delta is not None else "-",
                        pct=f"{pct:.2f}" if pct is not None else "-",
                        ma=item["moving_avg"],
                    )
                )

    if metadata and metadata.get("svg_paths"):
        lines.append("")
        for path in metadata["svg_paths"]:
            lines.append(f"![Condition Trends]({path})")

    return "\n".join(lines) + "\n"


def _moving_average(series: List[Optional[float]], window: int = 3) -> List[Optional[float]]:
    results: List[Optional[float]] = []
    numeric_series: List[float] = []
    for value in series:
        if value is None:
            numeric_series.append(float("nan"))
        else:
            numeric_series.append(value)
    for idx, value in enumerate(series):
        if value is None:
            results.append(None)
            continue
        window_values = [
            numeric_series[i]
            for i in range(max(0, idx - window + 1), idx + 1)
            if not math.isnan(numeric_series[i])
        ]
        if window_values:
            results.append(statistics.mean(window_values))
        else:
            results.append(value)
    return results


def _build_timeline_datasets(
    runs: List[RunRecord],
) -> Tuple[
    List[str],
    Dict[str, List[Optional[float]]],
    Dict[str, List[Optional[int]]],
    Dict[str, List[Optional[float]]],
]:
    runs_sorted = sorted(runs, key=lambda run: run.timestamp)
    labels = [datetime.fromtimestamp(run.timestamp, tz=timezone.utc).isoformat() for run in runs_sorted]
    eq_ids = sorted({row.eq_count for run in runs_sorted for row in run.rows})
    condition_map: Dict[str, List[Optional[float]]] = {f"eq{eq}": [] for eq in eq_ids}
    pending_map: Dict[str, List[Optional[int]]] = {f"eq{eq}": [] for eq in eq_ids}

    for run in runs_sorted:
        row_map = {row.eq_count: row for row in run.rows}
        for eq in eq_ids:
            key = f"eq{eq}"
            if eq in row_map:
                condition_map[key].append(row_map[eq].max_condition)
                pending_map[key].append(row_map[eq].max_pending_steps)
            else:
                condition_map[key].append(None)
                pending_map[key].append(None)

    condition_ma_map: Dict[str, List[Optional[float]]] = {
        key: _moving_average(values, window=3) for key, values in condition_map.items()
    }

    return labels, condition_map, pending_map, condition_ma_map


def render_html(
    markdown: str,
    runs: List[RunRecord],
    metadata: Optional[Dict[str, object]] = None,
) -> str:
    rows = markdown.strip().splitlines()
    html_lines: List[str] = [
        "<html>",
        "<head><meta charset=\"utf-8\"><title>Coupled Benchmark Report</title>",
        '<script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>',
        "</head>",
        "<body>",
    ]
    table_buffer: List[str] = []

    def flush_table() -> None:
        nonlocal table_buffer
        if not table_buffer:
            return
        header = table_buffer[0].strip("| ").split("|")
        html_lines.append("<table border=\"1\" cellspacing=\"0\" cellpadding=\"4\">")
        html_lines.append("  <thead><tr>" + "".join(f"<th>{cell.strip()}</th>" for cell in header) + "</tr></thead>")
        html_lines.append("  <tbody>")
        for row in table_buffer[2:]:
            cells = row.strip("| ").split("|")
            html_lines.append("    <tr>" + "".join(f"<td>{cell.strip()}</td>" for cell in cells) + "</tr>")
        html_lines.append("  </tbody></table>")
        table_buffer = []

    for line in rows:
        if line.startswith("|"):
            table_buffer.append(line)
            continue
        flush_table()
        if line.startswith("#"):
            level = len(line) - len(line.lstrip("#"))
            content = line.lstrip("# ")
            html_lines.append(f"<h{level}>{content}</h{level}>")
        elif line:
            html_lines.append(f"<p>{line}</p>")
    flush_table()
    if metadata:
        banner_parts = []
        generated = metadata.get("generated_at")
        if generated:
            banner_parts.append(f"Generated: {generated}")
        commit = metadata.get("git_commit")
        if commit:
            banner_parts.append(f"Commit: {commit}")
        config_url = metadata.get("config_url")
        config_label = metadata.get("config_path") or metadata.get("config_url")
        if config_url and config_label:
            banner_parts.append(f"Thresholds: <a href=\"{config_url}\">{config_label}</a>")
        elif config_label:
            banner_parts.append(f"Thresholds: {config_label}")
        if banner_parts:
            html_lines.append("<p><strong>" + " | ".join(banner_parts) + "</strong></p>")
    if runs:
        labels, condition_map, pending_map, condition_ma_map = _build_timeline_datasets(runs)
        html_lines.append('<h2>Condition Number Timeline</h2>')
        html_lines.append('<canvas id="conditionChart" height="200"></canvas>')
        html_lines.append('<h2>Pending Steps Timeline</h2>')
        html_lines.append('<canvas id="pendingChart" height="200"></canvas>')
        html_lines.append("<script>")
        html_lines.append("const conditionLabels = " + json.dumps(labels) + ";")
        html_lines.append("const conditionDatasets = [];")
        for key, values in condition_map.items():
            html_lines.append(
                "conditionDatasets.push({label: '"
                + key
                + "', data: "
                + json.dumps(values)
                + ", spanGaps: true});"
            )
        for key, values in condition_ma_map.items():
            html_lines.append(
                "conditionDatasets.push({label: '"
                + key
                + " (MA3)', data: "
                + json.dumps(values)
                + ", spanGaps: true, borderDash: [6,3], borderWidth: 2});"
            )
        html_lines.append(
            "new Chart(document.getElementById('conditionChart'), {type: 'line', data: {labels: conditionLabels, datasets: conditionDatasets}, options: {responsive: true, plugins: {legend: {position: 'bottom'}}, scales: {y: {type: 'logarithmic', min: 1}}}});")

        html_lines.append("const pendingDatasets = [];")
        for key, values in pending_map.items():
            html_lines.append(
                "pendingDatasets.push({label: '"
                + key
                + "', data: "
                + json.dumps(values)
                + ", spanGaps: true});"
            )
        html_lines.append(
            "new Chart(document.getElementById('pendingChart'), {type: 'line', data: {labels: conditionLabels, datasets: pendingDatasets}, options: {responsive: true, plugins: {legend: {position: 'bottom'}}, scales: {y: {beginAtZero: true}}}});")
        html_lines.append("</script>")

    if metadata and metadata.get("svg_paths"):
        html_lines.append('<h2>Condition Trends (SVG)</h2>')
        for path in metadata["svg_paths"]:
            html_lines.append(f'<img src="{path}" alt="Condition trends" style="max-width:100%;height:auto;"/>')

    html_lines.append("</body></html>")
    return "\n".join(html_lines)


def generate_svg_charts(runs: List[RunRecord], output_dir: Path) -> List[Path]:
    if not runs:
        return []
    labels, condition_map, _, condition_ma_map = _build_timeline_datasets(runs)
    output_dir.mkdir(parents=True, exist_ok=True)
    svg_path = output_dir / "condition_trends.svg"
    _write_condition_svg(labels, condition_map, condition_ma_map, svg_path)
    return [svg_path]


def _write_condition_svg(
    labels: List[str],
    datasets: Dict[str, List[Optional[float]]],
    moving_datasets: Dict[str, List[Optional[float]]],
    output_path: Path,
) -> None:
    width, height = 800, 260
    margin_left = 60
    margin_right = 20
    margin_top = 40
    margin_bottom = 40
    inner_width = width - margin_left - margin_right
    inner_height = height - margin_top - margin_bottom

    values = [val for series in datasets.values() for val in series if val and val > 0]
    if not values:
        values = [1.0]
    min_val = min(values)
    max_val = max(values)
    if min_val <= 0:
        min_val = min(val for val in values if val > 0)
    if min_val == max_val:
        max_val = min_val * 1.1
    min_log = math.log10(min_val)
    max_log = math.log10(max_val)
    denom = max(max_log - min_log, 1e-6)

    def x_coord(index: int) -> float:
        if len(labels) == 1:
            return margin_left + inner_width / 2
        return margin_left + inner_width * index / (len(labels) - 1)

    def y_coord(value: float) -> float:
        value = max(value, 1e-12)
        rel = (math.log10(value) - min_log) / denom
        rel = max(0.0, min(1.0, rel))
        return margin_top + (1 - rel) * inner_height

    palette = [
        "#1f77b4",
        "#ff7f0e",
        "#2ca02c",
        "#d62728",
        "#9467bd",
        "#8c564b",
    ]

    svg_lines = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        f'<rect x="0" y="0" width="{width}" height="{height}" fill="#ffffff" stroke="#dddddd"/>',
    ]

    # Axes
    svg_lines.append(
        f'<line x1="{margin_left}" y1="{margin_top}" x2="{margin_left}" y2="{margin_top + inner_height}" '
        'stroke="#333" stroke-width="1"/>'
    )
    svg_lines.append(
        f'<line x1="{margin_left}" y1="{margin_top + inner_height}" x2="{margin_left + inner_width}" '
        'y2="{margin_top + inner_height}" stroke="#333" stroke-width="1"/>'
    )

    # Y-axis ticks
    for tick in range(5):
        frac = tick / 4 if 4 else 0
        log_value = min_log + denom * (1 - frac)
        value = 10 ** log_value
        y = margin_top + inner_height * frac
        svg_lines.append(
            f'<line x1="{margin_left - 5}" y1="{y:.1f}" x2="{margin_left}" y2="{y:.1f}" stroke="#555" stroke-width="1"/>'
        )
        svg_lines.append(
            f'<text x="{margin_left - 10}" y="{y + 4:.1f}" font-size="10" text-anchor="end">{value:.1e}</text>'
        )

    # X-axis labels (limit to avoid clutter)
    max_labels = 6
    step = max(1, len(labels) // max_labels)
    for idx in range(0, len(labels), step):
        x = x_coord(idx)
        svg_lines.append(
            f'<line x1="{x:.1f}" y1="{margin_top + inner_height}" x2="{x:.1f}" y2="{margin_top + inner_height + 4}" stroke="#555" stroke-width="1"/>'
        )
        svg_lines.append(
            f'<text x="{x:.1f}" y="{margin_top + inner_height + 16:.1f}" font-size="10" text-anchor="middle">{labels[idx][:10]}</text>'
        )

    # Plot datasets
    for index, (key, series) in enumerate(datasets.items()):
        color = palette[index % len(palette)]
        commands = []
        pen_up = True
        for idx, value in enumerate(series):
            if value is None or value <= 0:
                pen_up = True
                continue
            x = x_coord(idx)
            y = y_coord(value)
            if pen_up:
                commands.append(f"M{x:.1f},{y:.1f}")
                pen_up = False
            else:
                commands.append(f"L{x:.1f},{y:.1f}")
        if commands:
            svg_lines.append(
                f'<path d="{' '.join(commands)}" fill="none" stroke="{color}" stroke-width="1.5" stroke-linejoin="round" stroke-linecap="round"/>'
            )
        ma_series = moving_datasets.get(key)
        if ma_series:
            commands = []
            pen_up = True
            for idx, value in enumerate(ma_series):
                if value is None or value <= 0:
                    pen_up = True
                    continue
                x = x_coord(idx)
                y = y_coord(value)
                if pen_up:
                    commands.append(f"M{x:.1f},{y:.1f}")
                    pen_up = False
                else:
                    commands.append(f"L{x:.1f},{y:.1f}")
            if commands:
                svg_lines.append(
                    f'<path d="{' '.join(commands)}" fill="none" stroke="{color}" stroke-width="1.5" stroke-dasharray="6 3" opacity="0.6"/>'
                )

    # Legend
    legend_x = margin_left + 10
    legend_y = margin_top - 20
    svg_lines.append(f'<text x="{legend_x}" y="{legend_y}" font-size="12" font-weight="bold">Condition Number (log10)</text>')
    legend_y += 16
    for index, key in enumerate(datasets.keys()):
        color = palette[index % len(palette)]
        svg_lines.append(
            f'<line x1="{legend_x}" y1="{legend_y - 6}" x2="{legend_x + 20}" y2="{legend_y - 6}" stroke="{color}" stroke-width="2"/>'
        )
        svg_lines.append(
            f'<text x="{legend_x + 26}" y="{legend_y}" font-size="11">{key}</text>'
        )
        legend_y += 14

    svg_lines.append("</svg>")
    output_path.write_text("\n".join(svg_lines), encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Summarise Coupled benchmark CSV history.")
    parser.add_argument("inputs", nargs="*", help="CSV paths, directories, or glob patterns.")
    parser.add_argument("--output-md", help="Write Markdown report to the specified path.")
    parser.add_argument("--output-html", help="Write HTML report to the specified path.")
    parser.add_argument("--latest", type=int, help="Limit processing to the most recent N runs.")
    parser.add_argument("--output-svg-dir", help="Destination directory for generated SVG charts.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    paths = discover_inputs(args.inputs)
    if not paths:
        print("No benchmark CSV files found.")
        return 1

    runs = [load_run(path) for path in paths]
    runs.sort(key=lambda run: run.timestamp, reverse=True)
    if args.latest is not None and args.latest > 0:
        runs = runs[: args.latest]

    svg_paths: List[Path] = []
    if args.output_svg_dir:
        svg_dir = Path(args.output_svg_dir).expanduser()
        svg_paths = generate_svg_charts(runs, svg_dir)

    aggregates = aggregate_by_eq(runs)
    metadata: Dict[str, object] = {}
    if svg_paths:
        metadata["svg_paths"] = [str(path) for path in svg_paths]
    markdown = render_markdown(runs, aggregates, metadata if metadata else None)

    if args.output_md:
        md_path = Path(args.output_md).expanduser()
        md_path.parent.mkdir(parents=True, exist_ok=True)
        md_path.write_text(markdown, encoding="utf-8")
        print(f"Wrote Markdown report to {md_path}")
    else:
        print(markdown)

    if args.output_html:
        html = render_html(markdown, runs, metadata if metadata else None)
        html_path = Path(args.output_html).expanduser()
        html_path.parent.mkdir(parents=True, exist_ok=True)
        html_path.write_text(html, encoding="utf-8")
        print(f"Wrote HTML report to {html_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
