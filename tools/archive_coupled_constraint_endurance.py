#!/usr/bin/env python3
"""
Archive coupled constraint endurance CSV logs and keep summaries fresh.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
import shutil
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Optional, Tuple

from coupled_constraint_endurance_analysis import (
    CoupledConstraintSummary,
    compute_summary,
    default_csv_path,
    load_csv,
    summary_as_html,
    summary_as_markdown,
)

MANIFEST_VERSION = 1


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Archive coupled constraint endurance CSV files with duplicate detection "
            "and optional summary generation."
        )
    )
    parser.add_argument(
        "inputs",
        nargs="*",
        help="CSV files to archive. Defaults to the primary endurance log when omitted.",
    )
    parser.add_argument(
        "--archive-dir",
        default=str(Path("data") / "endurance_archive"),
        help="Directory where archived CSV files and summaries are stored.",
    )
    parser.add_argument(
        "--manifest",
        help="Path to manifest JSON (default: <archive-dir>/manifest.json).",
    )
    parser.add_argument(
        "--label",
        help="Optional label recorded alongside the manifest entry (e.g. CI job id).",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show planned actions without copying files or updating the manifest.",
    )
    parser.add_argument(
        "--no-summary",
        action="store_true",
        help="Do not generate per-run Markdown/HTML summaries.",
    )
    parser.add_argument(
        "--latest-prefix",
        default="latest",
        help="Prefix for refreshable outputs (e.g. latest.csv). Set empty to disable.",
    )
    return parser.parse_args()


def compute_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_manifest(path: Path) -> Dict[str, object]:
    if not path.exists():
        return {"version": MANIFEST_VERSION, "entries": []}
    with path.open("r", encoding="utf-8") as handle:
        manifest = json.load(handle)
    if manifest.get("version") != MANIFEST_VERSION:
        raise RuntimeError(
            f"Unsupported manifest version {manifest.get('version')} (expected {MANIFEST_VERSION})."
        )
    if "entries" not in manifest or not isinstance(manifest["entries"], list):
        raise RuntimeError("Manifest file is missing 'entries' list.")
    return manifest


def save_manifest(path: Path, manifest: Dict[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        json.dump(manifest, handle, indent=2, sort_keys=True)
        handle.write("\n")


def current_timestamp() -> Tuple[str, str]:
    now = datetime.now(timezone.utc)
    iso = now.replace(microsecond=0).isoformat().replace("+00:00", "Z")
    tag = now.strftime("%Y%m%dT%H%M%SZ")
    return iso, tag


def ensure_list(value: Optional[List[str]]) -> List[str]:
    return value or [str(default_csv_path())]


def archive_single_run(
    csv_path: Path,
    archive_dir: Path,
    manifest: Dict[str, object],
    summary: CoupledConstraintSummary,
    file_hash: str,
    timestamp_iso: str,
    timestamp_tag: str,
    label: Optional[str],
    generate_summary: bool,
    dry_run: bool,
) -> Optional[Dict[str, object]]:
    entries: List[Dict[str, object]] = manifest["entries"]  # type: ignore[assignment]
    existing = next((entry for entry in entries if entry.get("hash_sha256") == file_hash), None)
    if existing:
        print(
            f"Skipping archive for {csv_path} (duplicate of {existing.get('archive_path')})."
        )
        return None

    archive_dir.mkdir(parents=True, exist_ok=True)
    filename = f"{csv_path.stem}_{timestamp_tag}_{file_hash[:8]}.csv"
    archive_path = archive_dir / filename
    entry = {
        "timestamp": timestamp_iso,
        "archive_path": archive_path.name,
        "hash_sha256": file_hash,
        "source": str(csv_path),
        "label": label,
        "summary": summary.to_dict(),
    }

    if dry_run:
        print(f"[dry-run] Would copy {csv_path} -> {archive_path}")
        entries.append(entry)
        return entry

    shutil.copy2(csv_path, archive_path)
    print(f"Archived {csv_path} -> {archive_path}")

    if generate_summary:
        summary_md_path = archive_dir / f"{filename}.summary.md"
        summary_html_path = archive_dir / f"{filename}.summary.html"
        summary_md_path.write_text(summary_as_markdown(summary) + "\n", encoding="utf-8")
        summary_html_path.write_text(summary_as_html(summary), encoding="utf-8")
        print(f"Wrote summaries: {summary_md_path.name}, {summary_html_path.name}")

    entries.append(entry)
    entries.sort(key=lambda item: item["timestamp"])  # type: ignore[index]
    return entry


def refresh_latest_outputs(
    csv_path: Path,
    archive_dir: Path,
    summary: CoupledConstraintSummary,
    latest_prefix: str,
    generate_summary: bool,
    dry_run: bool,
) -> None:
    if not latest_prefix:
        return

    latest_csv = archive_dir / f"{latest_prefix}.csv"
    if dry_run:
        print(f"[dry-run] Would refresh {latest_csv}")
    else:
        archive_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(csv_path, latest_csv)
        print(f"Updated {latest_csv}")

    if not generate_summary:
        return

    latest_md = archive_dir / f"{latest_prefix}.summary.md"
    latest_html = archive_dir / f"{latest_prefix}.summary.html"
    md_text = summary_as_markdown(summary) + "\n"
    html_text = summary_as_html(summary)

    if dry_run:
        print(f"[dry-run] Would refresh {latest_md} / {latest_html}")
        return

    latest_md.write_text(md_text, encoding="utf-8")
    latest_html.write_text(html_text, encoding="utf-8")
    print(f"Updated {latest_md} and {latest_html}")


def main() -> int:
    args = parse_args()
    archive_dir = Path(args.archive_dir).expanduser()
    manifest_path = (
        Path(args.manifest).expanduser()
        if args.manifest
        else archive_dir / "manifest.json"
    )

    try:
        manifest = load_manifest(manifest_path)
    except RuntimeError as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    inputs = [Path(item).expanduser() for item in ensure_list(args.inputs)]

    for csv_path in inputs:
        if not csv_path.exists():
            print(f"Warning: input CSV not found at {csv_path}; skipping.", file=sys.stderr)
            continue

        print(f"Processing {csv_path}")
        try:
            data = load_csv(csv_path)
        except ValueError as exc:
            print(f"Error while reading {csv_path}: {exc}", file=sys.stderr)
            return 1

        summary = compute_summary(data)
        file_hash = compute_sha256(csv_path)
        timestamp_iso, timestamp_tag = current_timestamp()

        archive_single_run(
            csv_path=csv_path,
            archive_dir=archive_dir,
            manifest=manifest,
            summary=summary,
            file_hash=file_hash,
            timestamp_iso=timestamp_iso,
            timestamp_tag=timestamp_tag,
            label=args.label,
            generate_summary=not args.no_summary,
            dry_run=args.dry_run,
        )

        refresh_latest_outputs(
            csv_path=csv_path,
            archive_dir=archive_dir,
            summary=summary,
            latest_prefix=args.latest_prefix,
            generate_summary=not args.no_summary,
            dry_run=args.dry_run,
        )

        print()

    if args.dry_run:
        print("Dry-run complete; manifest not updated.")
        return 0

    save_manifest(manifest_path, manifest)
    print(f"Manifest written to {manifest_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
