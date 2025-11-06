#!/usr/bin/env python3
"""Generate a static site bundle for Coupled benchmark results."""

from __future__ import annotations

import argparse
import os
import shutil
from datetime import datetime, timezone
from pathlib import Path
from typing import List

from summarize_coupled_benchmark_history import (
    aggregate_by_eq,
    discover_inputs,
    generate_svg_charts,
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
    parser.add_argument(
        "--threshold-config",
        default="config/coupled_benchmark_thresholds.yaml",
        help="Path to the shared threshold configuration file (used for linking on the site).",
    )
    return parser.parse_args()


def _relative_to_output(path: Path, output_dir: Path) -> str:
    try:
        return path.relative_to(output_dir).as_posix()
    except ValueError:
        return path.as_posix()


def write_feed(
    runs,
    metadata,
    output_dir: Path,
    copy_data: bool,
    max_entries: int = 10,
) -> None:
    feed_path = output_dir / "feed.xml"
    generated_at = metadata.get("generated_at", datetime.now(timezone.utc).isoformat())
    repo = metadata.get("repo")
    ref = metadata.get("git_ref")
    feed_id = (
        f"tag:{repo},{generated_at[:10]}:coupled-benchmark"
        if repo
        else f"urn:uuid:{os.urandom(8).hex()}"
    )
    lines = [
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>",
        '<feed xmlns="http://www.w3.org/2005/Atom">',
        "  <title>Coupled Benchmark Updates</title>",
        f"  <updated>{generated_at}</updated>",
        f"  <id>{feed_id}</id>",
        "  <link rel=\"alternate\" type=\"text/html\" href=\"index.html\"/>",
    ]

    base_path = Path.cwd().resolve()
    for run in runs[:max_entries]:
        updated = datetime.fromtimestamp(run.timestamp, timezone.utc).isoformat()
        try:
            rel_path = run.path.resolve().relative_to(base_path).as_posix()
        except ValueError:
            rel_path = run.path.as_posix()
        if copy_data:
            link_href = f"data/{run.path.name}"
        elif repo and ref:
            link_href = f"https://github.com/{repo}/blob/{ref}/{rel_path}"
        else:
            link_href = rel_path

        summary_parts = []
        for row in run.rows[: min(3, len(run.rows))]:
            summary_parts.append(
                f"eq{row.eq_count}: κ_b={row.max_condition:.2e}, κ_s={row.max_condition_spectral:.2e}, Δ={row.max_condition_gap:.2e}"
            )
        summary_text = ", ".join(summary_parts) if summary_parts else "No data"
        entry_id = f"{feed_id}/{run.path.name}-{int(run.timestamp)}"

        lines.extend(
            [
                "  <entry>",
                f"    <title>Benchmark run {run.path.name}</title>",
                f"    <updated>{updated}</updated>",
                f"    <id>{entry_id}</id>",
                f"    <link href=\"{link_href}\"/>",
                f"    <summary>{summary_text}</summary>",
                "  </entry>",
            ]
        )

    lines.append("</feed>")
    feed_path.write_text("\n".join(lines), encoding="utf-8")


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

    generated_at = datetime.now(timezone.utc).isoformat()
    config_path = Path(args.threshold_config).as_posix()
    repo = os.environ.get("GITHUB_REPOSITORY")
    ref_name = os.environ.get("GITHUB_REF_NAME")
    commit_sha = os.environ.get("GITHUB_SHA")

    if repo and ref_name:
        config_url = f"https://github.com/{repo}/blob/{ref_name}/{config_path}"
    else:
        config_url = config_path

    metadata = {
        "generated_at": generated_at,
        "config_path": config_path,
        "config_url": config_url,
        "git_commit": commit_sha[:7] if commit_sha else None,
        "git_ref": ref_name,
        "repo": repo,
    }

    svg_paths: List[Path] = generate_svg_charts(runs, output_dir / "svg")
    if svg_paths:
        metadata["svg_paths"] = [_relative_to_output(path, output_dir) for path in svg_paths]

    aggregates = aggregate_by_eq(runs)
    markdown = render_markdown(runs, aggregates, metadata=metadata)
    html = render_html(markdown, runs, metadata=metadata)

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

    write_feed(runs, metadata, output_dir, args.copy_data)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
