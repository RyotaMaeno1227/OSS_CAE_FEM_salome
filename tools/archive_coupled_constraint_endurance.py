#!/usr/bin/env python3
"""
Archive coupled constraint endurance CSV logs and keep summaries fresh.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import sys
import shutil
from datetime import datetime, timedelta, timezone
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
    parser.add_argument(
        "--prune-duplicates",
        action="store_true",
        help="Remove older archive entries that share the same hash.",
    )
    parser.add_argument(
        "--max-entries",
        type=int,
        help="Keep only the most recent N entries (after deduplication).",
    )
    parser.add_argument(
        "--max-age-days",
        type=int,
        help="Remove archived entries older than the specified number of days.",
    )
    parser.add_argument(
        "--max-file-size-mb",
        type=float,
        help="Fail if an input CSV exceeds the given size (in MiB).",
    )
    parser.add_argument(
        "--plan-csv",
        help="Optional path to write planned operations (CSV format).",
    )
    parser.add_argument(
        "--plan-markdown",
        help="Optional path to write planned operations (Markdown format).",
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


def parse_timestamp(value: str) -> datetime:
    if value.endswith("Z"):
        value = value[:-1] + "+00:00"
    return datetime.fromisoformat(value)


def _record_action(
    plan_records: Optional[List[Dict[str, str]]],
    *,
    action: str,
    target: str,
    detail: str = "",
    hash_sha256: str = "",
    reason: str = "",
) -> None:
    if plan_records is None:
        return
    plan_records.append(
        {
            "action": action,
            "target": target,
            "detail": detail,
            "hash": hash_sha256,
            "reason": reason,
        }
    )


def _print_plan_summary(plan_records: List[Dict[str, str]], heading: str) -> None:
    if not plan_records:
        print(f"{heading}: none")
        return

    print(heading)
    header = ("Action", "Target", "Detail", "Hash", "Reason")
    rows = [header]
    for record in plan_records:
        rows.append(
            (
                record.get("action", ""),
                record.get("target", ""),
                record.get("detail", ""),
                record.get("hash", ""),
                record.get("reason", ""),
            )
        )

    col_widths = [max(len(str(row[idx])) for row in rows) for idx in range(len(header))]
    for idx, row in enumerate(rows):
        line = "  ".join(str(cell).ljust(col_widths[col_idx]) for col_idx, cell in enumerate(row))
        if idx == 0:
            print(line)
            print("  ".join("-" * col_widths[col_idx] for col_idx in range(len(header))))
        else:
            print(line)


def _write_plan_csv(path: Path, plan_records: List[Dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=["action", "target", "detail", "hash", "reason"])
        writer.writeheader()
        for record in plan_records:
            writer.writerow(
                {
                    "action": record.get("action", ""),
                    "target": record.get("target", ""),
                    "detail": record.get("detail", ""),
                    "hash": record.get("hash", ""),
                    "reason": record.get("reason", ""),
                }
            )


def ensure_list(value: Optional[List[str]]) -> List[str]:
    return value or [str(default_csv_path())]


def _escape_markdown(text: str) -> str:
    return text.replace("|", "\\|")


def _write_plan_markdown(path: Path, plan_records: List[Dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not plan_records:
        path.write_text("No planned operations.\n", encoding="utf-8")
        return

    lines = ["| Action | Target | Detail | Hash | Reason |", "| --- | --- | --- | --- | --- |"]
    for record in plan_records:
        lines.append(
            "| {} | {} | {} | {} | {} |".format(
                _escape_markdown(record.get("action", "")),
                _escape_markdown(record.get("target", "")),
                _escape_markdown(record.get("detail", "")),
                _escape_markdown(record.get("hash", "")),
                _escape_markdown(record.get("reason", "")),
            )
        )

    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


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
    max_file_size_bytes: Optional[int],
    plan_records: Optional[List[Dict[str, str]]],
) -> Optional[Dict[str, object]]:
    entries: List[Dict[str, object]] = manifest["entries"]  # type: ignore[assignment]
    existing = next((entry for entry in entries if entry.get("hash_sha256") == file_hash), None)
    if existing:
        _record_action(
            plan_records,
            action="skip-duplicate",
            target=str(csv_path),
            detail=existing.get("archive_path", ""),
            hash_sha256=file_hash,
            reason="duplicate-hash",
        )
        print(
            f"Skipping archive for {csv_path} (duplicate of {existing.get('archive_path')})."
        )
        return None

    if max_file_size_bytes is not None:
        file_size = csv_path.stat().st_size
        if file_size > max_file_size_bytes:
            _record_action(
                plan_records,
                action="reject-large-file",
                target=str(csv_path),
                detail=f"{file_size} bytes",
                hash_sha256=file_hash,
                reason="max-file-size",
            )
            raise RuntimeError(
                f"Input CSV {csv_path} is {file_size / (1024 * 1024):.2f} MiB, which exceeds the limit "
                f"of {max_file_size_bytes / (1024 * 1024):.2f} MiB."
            )

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
        _record_action(
            plan_records,
            action="archive",
            target=str(csv_path),
            detail=archive_path.name,
            hash_sha256=file_hash,
            reason="planned",
        )
        entries.append(entry)
        return entry

    shutil.copy2(csv_path, archive_path)
    print(f"Archived {csv_path} -> {archive_path}")
    _record_action(
        plan_records,
        action="archive",
        target=str(csv_path),
        detail=archive_path.name,
        hash_sha256=file_hash,
        reason="completed",
    )

    if generate_summary:
        summary_md_path = archive_dir / f"{filename}.summary.md"
        summary_html_path = archive_dir / f"{filename}.summary.html"
        summary_json_path = archive_dir / f"{filename}.summary.json"
        summary_md_path.write_text(summary_as_markdown(summary) + "\n", encoding="utf-8")
        summary_html_path.write_text(summary_as_html(summary), encoding="utf-8")
        summary_json_path.write_text(
            json.dumps(summary.to_dict(), indent=2) + "\n",
            encoding="utf-8",
        )
        print(
            "Wrote summaries: "
            f"{summary_md_path.name}, {summary_html_path.name}, {summary_json_path.name}"
        )
        _record_action(
            plan_records,
            action="write-summaries",
            target=archive_path.name,
            detail="markdown/html/json",
            hash_sha256=file_hash,
            reason="completed",
        )

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
    plan_records: Optional[List[Dict[str, str]]],
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
    _record_action(
        plan_records,
        action="refresh-latest",
        target=str(latest_csv),
        detail="csv",
        reason="dry-run" if dry_run else "completed",
    )

    if not generate_summary:
        return

    latest_md = archive_dir / f"{latest_prefix}.summary.md"
    latest_html = archive_dir / f"{latest_prefix}.summary.html"
    latest_json = archive_dir / f"{latest_prefix}.summary.json"
    md_text = summary_as_markdown(summary) + "\n"
    html_text = summary_as_html(summary)
    json_text = json.dumps(summary.to_dict(), indent=2) + "\n"

    if dry_run:
        print(f"[dry-run] Would refresh {latest_md} / {latest_html} / {latest_json}")
        _record_action(
            plan_records,
            action="refresh-latest",
            target=str(latest_md),
            detail="markdown/html/json",
            reason="dry-run",
        )
        return

    latest_md.write_text(md_text, encoding="utf-8")
    latest_html.write_text(html_text, encoding="utf-8")
    latest_json.write_text(json_text, encoding="utf-8")
    print(f"Updated {latest_md}, {latest_html}, and {latest_json}")
    _record_action(
        plan_records,
        action="refresh-latest",
        target=str(latest_md),
        detail="markdown/html/json",
        reason="dry-run" if dry_run else "completed",
    )


def _delete_archive_artifacts(
    archive_dir: Path,
    filename: str,
    dry_run: bool,
    plan_records: Optional[List[Dict[str, str]]],
    reason: str = "",
) -> None:
    targets = [
        archive_dir / filename,
        archive_dir / f"{filename}.summary.md",
        archive_dir / f"{filename}.summary.html",
        archive_dir / f"{filename}.summary.json",
    ]
    for path in targets:
        if not path.exists():
            continue
        if dry_run:
            print(f"[dry-run] Would remove {path}")
        else:
            path.unlink()
            print(f"Removed {path}")
        _record_action(
            plan_records,
            action="delete",
            target=str(path),
            detail="planned" if dry_run else "removed",
            reason=reason or ("dry-run" if dry_run else "completed"),
        )


def prune_duplicate_entries(
    manifest: Dict[str, object],
    archive_dir: Path,
    dry_run: bool,
    plan_records: Optional[List[Dict[str, str]]],
) -> int:
    entries: List[Dict[str, object]] = manifest["entries"]  # type: ignore[assignment]
    if not entries:
        return 0

    ordered = sorted(entries, key=lambda item: item["timestamp"], reverse=True)  # type: ignore[index]
    kept: List[Dict[str, object]] = []
    seen_hashes = set()
    removed = 0

    for entry in ordered:
        file_hash = entry.get("hash_sha256")
        if file_hash in seen_hashes:
            removed += 1
            archive_name = entry.get("archive_path")
            if archive_name:
                _delete_archive_artifacts(
                    archive_dir, archive_name, dry_run, plan_records, reason="duplicate-prune"
                )
            continue
        seen_hashes.add(file_hash)
        kept.append(entry)

    kept = sorted(kept, key=lambda item: item["timestamp"])  # type: ignore[index]

    if not dry_run:
        manifest["entries"] = kept

    if removed:
        print(f"Pruned {removed} duplicate archive entr{'y' if removed == 1 else 'ies'}.")
    else:
        print("No duplicate archives detected.")

    return removed


def enforce_max_entries(
    manifest: Dict[str, object],
    archive_dir: Path,
    max_entries: int,
    dry_run: bool,
    plan_records: Optional[List[Dict[str, str]]],
) -> int:
    if max_entries <= 0:
        raise ValueError("--max-entries must be positive.")

    entries: List[Dict[str, object]] = manifest["entries"]  # type: ignore[assignment]
    if len(entries) <= max_entries:
        print(f"Archive contains {len(entries)} entries (<= {max_entries}); nothing to prune.")
        return 0

    ordered = sorted(entries, key=lambda item: item["timestamp"], reverse=True)  # type: ignore[index]
    kept = ordered[:max_entries]
    trimmed = ordered[max_entries:]

    for entry in trimmed:
        archive_name = entry.get("archive_path")
        if archive_name:
            _delete_archive_artifacts(
                archive_dir, archive_name, dry_run, plan_records, reason="max-entries"
            )

    if not dry_run:
        manifest["entries"] = sorted(kept, key=lambda item: item["timestamp"])  # type: ignore[index]

    print(f"Trimmed {len(trimmed)} archive entr{'y' if len(trimmed) == 1 else 'ies'} to enforce max={max_entries}.")
    return len(trimmed)


def enforce_max_age(
    manifest: Dict[str, object],
    archive_dir: Path,
    max_age_days: int,
    dry_run: bool,
    plan_records: Optional[List[Dict[str, str]]],
) -> int:
    if max_age_days <= 0:
        raise ValueError("--max-age-days must be positive.")

    cutoff = datetime.now(timezone.utc) - timedelta(days=max_age_days)
    entries: List[Dict[str, object]] = manifest["entries"]  # type: ignore[assignment]
    removed = 0
    kept: List[Dict[str, object]] = []

    for entry in entries:
        timestamp_str = entry.get("timestamp")
        if not timestamp_str:
            kept.append(entry)
            continue
        try:
            entry_time = parse_timestamp(timestamp_str)
        except ValueError:
            # Preserve unparsable entries but surface warning.
            print(f"Warning: could not parse timestamp '{timestamp_str}', keeping entry.", file=sys.stderr)
            kept.append(entry)
            continue

        if entry_time < cutoff:
            removed += 1
            archive_name = entry.get("archive_path")
            if archive_name:
                _delete_archive_artifacts(
                    archive_dir, archive_name, dry_run, plan_records, reason="max-age"
                )
        else:
            kept.append(entry)

    if not dry_run:
        manifest["entries"] = sorted(kept, key=lambda item: item["timestamp"])  # type: ignore[index]

    if removed:
        print(f"Pruned {removed} archive entr{'y' if removed == 1 else 'ies'} older than {max_age_days} days.")
    else:
        print(f"No archive entries older than {max_age_days} days.")

    return removed


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

    if args.max_entries is not None and args.max_entries <= 0:
        print("Error: --max-entries must be positive.", file=sys.stderr)
        return 2
    if args.max_age_days is not None and args.max_age_days <= 0:
        print("Error: --max-age-days must be positive.", file=sys.stderr)
        return 2
    if args.max_file_size_mb is not None and args.max_file_size_mb <= 0:
        print("Error: --max-file-size-mb must be positive.", file=sys.stderr)
        return 2

    max_file_size_bytes = (
        int(args.max_file_size_mb * 1024 * 1024) if args.max_file_size_mb is not None else None
    )

    plan_records: List[Dict[str, str]] = []
    plan_csv_path = Path(args.plan_csv).expanduser() if args.plan_csv else None
    plan_markdown_path = Path(args.plan_markdown).expanduser() if args.plan_markdown else None

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

        try:
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
                max_file_size_bytes=max_file_size_bytes,
                plan_records=plan_records,
            )
        except RuntimeError as exc:
            print(f"Error: {exc}", file=sys.stderr)
            return 3

        refresh_latest_outputs(
            csv_path=csv_path,
            archive_dir=archive_dir,
            summary=summary,
            latest_prefix=args.latest_prefix,
            generate_summary=not args.no_summary,
            dry_run=args.dry_run,
            plan_records=plan_records,
        )

        print()

    if args.prune_duplicates:
        prune_duplicate_entries(
            manifest=manifest,
            archive_dir=archive_dir,
            dry_run=args.dry_run,
            plan_records=plan_records,
        )

    if args.max_entries is not None:
        try:
            enforce_max_entries(
                manifest=manifest,
                archive_dir=archive_dir,
                max_entries=args.max_entries,
                dry_run=args.dry_run,
                plan_records=plan_records,
            )
        except ValueError as exc:
            print(f"Error: {exc}", file=sys.stderr)
            return 2

    if args.max_age_days is not None:
        try:
            enforce_max_age(
                manifest=manifest,
                archive_dir=archive_dir,
                max_age_days=args.max_age_days,
                dry_run=args.dry_run,
                plan_records=plan_records,
            )
        except ValueError as exc:
            print(f"Error: {exc}", file=sys.stderr)
            return 2

    heading = "Dry-run plan" if args.dry_run else "Operation summary"
    _print_plan_summary(plan_records, heading)

    if plan_csv_path:
        _write_plan_csv(plan_csv_path, plan_records)
        print(f"Wrote plan CSV to {plan_csv_path}")

    if plan_markdown_path:
        _write_plan_markdown(plan_markdown_path, plan_records)
        print(f"Wrote plan Markdown to {plan_markdown_path}")

    if args.dry_run:
        print("Dry-run complete; manifest not updated.")
        return 0

    save_manifest(manifest_path, manifest)
    print(f"Manifest written to {manifest_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
