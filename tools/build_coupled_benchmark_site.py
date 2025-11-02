#!/usr/bin/env python3
"""Generate a static site bundle for Coupled benchmark results."""

from __future__ import annotations

import argparse
import shutil
from pathlib import Path

from summarize_coupled_benchmark_history import (
    aggregate_by_eq,
    discover_inputs,
    load_run,
    render_html,
    render_markdown,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build static HTML site for Coupled benchmark data.")
    parser.add_argument(
        "--output-dir",
        required=True,
        help="Directory to place generated site files (index.html, assets, etc.).",
    )
    parser.add_argument(
        "inputs",
        nargs="*",
        help="CSV inputs (files, directories, or glob patterns). Defaults to data/coupled_benchmark_metrics.csv",
    )
    parser.add_argument(
        "--copy-data",
        action="store_true",
        help="Copy source CSV files into the output directory for download.",
    )
    parser.add_argument(
        "--latest",
        type=int,
        help="Limit processing to the most recent N runs.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    output_dir = Path(args.output_dir).expanduser()
    output_dir.mkdir(parents=True, exist_ok=True)

    paths = discover_inputs(args.inputs)
    if not paths:
        print("No benchmark CSV files found for site generation.")
        return 1

    runs = [load_run(path) for path in paths]
    runs.sort(key=lambda run: run.timestamp, reverse=True)
    if args.latest is not None and args.latest > 0:
        runs = runs[: args.latest]

    aggregates = aggregate_by_eq(runs)
    markdown = render_markdown(runs, aggregates)
    html = render_html(markdown, runs)

    index_path = output_dir / "index.html"
    index_path.write_text(html, encoding="utf-8")
    print(f"Wrote {index_path}")

    if args.copy_data:
        data_dir = output_dir / "data"
        data_dir.mkdir(exist_ok=True)
        for run in runs:
            destination = data_dir / run.path.name
            shutil.copy2(run.path, destination)
        print(f"Copied {len(runs)} CSV file(s) into {data_dir}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
