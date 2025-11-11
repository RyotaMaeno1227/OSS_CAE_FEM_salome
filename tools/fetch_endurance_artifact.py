#!/usr/bin/env python3
"""
Helper to download the latest coupled endurance artifact and suggest a repro command.
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Optional, Sequence


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Download coupled endurance artifacts from a workflow run and generate a repro command."
    )
    parser.add_argument(
        "run_id",
        nargs="?",
        help="GitHub Actions run ID (numeric) or run URL.",
    )
    parser.add_argument(
        "--output-dir",
        default="artifacts/coupled_endurance",
        help="Directory to store downloaded artifacts (default: %(default)s).",
    )
    parser.add_argument(
        "--artifact-prefix",
        default="coupled-endurance",
        help="Artifact name prefix (default: %(default)s). Run ID is appended automatically.",
    )
    parser.add_argument(
        "--gh-path",
        default="gh",
        help="Path to the GitHub CLI executable (default: %(default)s).",
    )
    parser.add_argument(
        "--threshold-max-condition",
        type=float,
        default=5.0e8,
        help="Threshold passed to --fail-on-max-condition for the repro command (default: %(default)s).",
    )
    parser.add_argument(
        "--threshold-rank-ratio",
        type=float,
        default=0.05,
        help="Threshold passed to --fail-on-rank-ratio for the repro command (default: %(default)s).",
    )
    parser.add_argument(
        "--summary-out",
        default="repro/latest.summary.json",
        help="Path where the repro command will write the JSON summary (default: %(default)s).",
    )
    parser.add_argument(
        "--interactive",
        action="store_true",
        help="Interactively select a run via `gh run list` if run_id is omitted.",
    )
    parser.add_argument(
        "--auto-latest",
        action="store_true",
        help="Automatically select the newest run (prefers failures).",
    )
    parser.add_argument(
        "--latest",
        action="store_true",
        help="Alias for --auto-latest.",
    )
    parser.add_argument(
        "--workflow",
        help="Workflow file or ID to filter when auto-selecting runs.",
    )
    parser.add_argument(
        "--run-status",
        choices=["failure", "success", "completed"],
        default="failure",
        help="Preferred conclusion when --auto-latest is used (default: %(default)s).",
    )
    parser.add_argument(
        "--job-name",
        default="archive-and-summarize",
        help="Job name used when linking logs (default: %(default)s).",
    )
    parser.add_argument(
        "--max-retries",
        type=int,
        default=2,
        help="Number of retries for failed download commands (default: %(default)s).",
    )
    parser.add_argument(
        "--repo",
        help="Owner/repo slug used to build run URLs (default: GITHUB_REPOSITORY env).",
    )
    parser.add_argument(
        "--comment-file",
        help="Write a Markdown comment template to the specified path.",
    )
    parser.add_argument(
        "--comment-target",
        help="Target for posting a comment (format: pr/<number> or issue/<number>).",
    )
    parser.add_argument(
        "--post-comment",
        action="store_true",
        help="Post the generated comment using gh issue/pr comment.",
    )
    parser.add_argument(
        "--console-comment-only",
        action="store_true",
        help="Restrict comment output to the console log (skip GitHub comment posting).",
    )
    return parser.parse_args()


def derive_run_id(run: str) -> str:
    if run.isdigit():
        return run
    # Accept URLs like https://github.com/org/repo/actions/runs/1234567890
    parts = run.rstrip("/").split("/")
    if parts and parts[-1].isdigit():
        return parts[-1]
    raise ValueError(f"Could not extract run ID from '{run}'.")


def ensure_directory(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def run_command(cmd: list[str]) -> None:
    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as exc:
        raise RuntimeError(f"Command {' '.join(cmd)} failed with exit code {exc.returncode}") from exc


def find_artifact_file(root: Path, filename: str) -> Optional[Path]:
    matches = list(root.rglob(filename))
    if not matches:
        return None
    # Prefer shortest path (closest to root)
    return min(matches, key=lambda p: len(p.parts))


def interactive_select_run(gh_path: str) -> str:
    list_cmd = [gh_path, "run", "list", "--limit", "20", "--json", "databaseId,headBranch,status,conclusion,displayTitle"]
    try:
        output = subprocess.check_output(list_cmd, text=True)
    except subprocess.CalledProcessError as exc:
        raise RuntimeError(f"`{' '.join(list_cmd)}` failed with exit code {exc.returncode}") from exc

    import json  # local import to avoid eager dependency when not needed

    try:
        runs = json.loads(output)
    except json.JSONDecodeError as exc:
        raise RuntimeError("Failed to parse gh run list output.") from exc

    if not runs:
        raise RuntimeError("No recent workflow runs found.")

    print("Select a workflow run:")
    for idx, run in enumerate(runs, start=1):
        ident = run.get("databaseId", "n/a")
        branch = run.get("headBranch", "unknown")
        status = run.get("status", "?")
        conclusion = run.get("conclusion") or "-"
        title = run.get("displayTitle", "")
        print(f"  {idx:2d}) #{ident} [{status}/{conclusion}] {branch} - {title}")

    while True:
        selection = input("Enter run number (or 'q' to abort): ").strip()
        if selection.lower() == "q":
            raise RuntimeError("Selection aborted by user.")
        if selection.isdigit():
            idx = int(selection)
            if 1 <= idx <= len(runs):
                return str(runs[idx - 1].get("databaseId"))
        print("Invalid selection. Please try again.")


def download_with_retries(cmd: Sequence[str], max_retries: int) -> None:
    attempt = 0
    while True:
        try:
            run_command(list(cmd))
            return
        except RuntimeError as exc:
            if attempt >= max_retries:
                raise
            attempt += 1
            print(f"Warning: {exc}. Retrying ({attempt}/{max_retries})...")


def build_comment_body(
    *,
    run_id: str,
    repo: Optional[str],
    artifact_name: str,
    download_dir: Path,
    summary: Optional[dict],
    repro_cmd: Sequence[str],
    plan_csv: Optional[Path],
    plan_md: Optional[Path],
    job_name: Optional[str],
    job_url: Optional[str],
) -> str:
    run_url = f"https://github.com/{repo}/actions/runs/{run_id}" if repo else None

    lines = [
        "## Coupled Endurance Failure Snapshot",
        f"- Run: [{run_id}]({run_url})" if run_url else f"- Run: {run_id}",
        f"- Artifact: `{artifact_name}`",
        f"- Download directory: `{download_dir}`",
    ]
    if job_name and job_url:
        lines.append(f"- Job log: [{job_name}]({job_url})")

    if summary:
        version = summary.get("version")
        metrics = [
            ("Samples", summary.get("samples", "n/a")),
            (
                "Max condition",
                f"{summary.get('max_condition', 0):.3e}"
                if isinstance(summary.get("max_condition"), (int, float))
                else summary.get("max_condition"),
            ),
            (
                "Warning ratio",
                f"{summary.get('warn_ratio', 0):.3%}"
                if isinstance(summary.get("warn_ratio"), (int, float))
                else summary.get("warn_ratio"),
            ),
            (
                "Rank ratio",
                f"{summary.get('rank_ratio', 0):.3%}"
                if isinstance(summary.get("rank_ratio"), (int, float))
                else summary.get("rank_ratio"),
            ),
            (
                "Duration [s]",
                f"{summary.get('duration', 0):.3f}"
                if isinstance(summary.get("duration"), (int, float))
                else summary.get("duration"),
            ),
        ]
        if version is not None:
            metrics.insert(0, ("Summary version", version))
        lines.append("")
        lines.append("| Metric | Value |")
        lines.append("| --- | --- |")
        for key, value in metrics:
            lines.append(f"| {key} | {value} |")

    if plan_csv or plan_md:
        lines.append("")
        lines.append("### Plan Files")
        if plan_csv:
            lines.append(f"- CSV: `{Path(plan_csv)}`")
        if plan_md:
            lines.append(f"- Markdown: `{Path(plan_md)}`")

    lines.append("")
    lines.append("### Reproduction Command")
    lines.append("```bash")
    lines.append(" ".join(repro_cmd))
    lines.append("```")

    if summary:
        import json

        lines.append("")
        lines.append("<details>")
        lines.append("<summary>latest.summary.json</summary>")
        lines.append("")
        lines.append("```json")
        lines.append(json.dumps(summary, indent=2))
        lines.append("```")
        lines.append("")
        lines.append("</details>")

    return "\n".join(lines)


def auto_select_latest_run(
    gh_path: str,
    *,
    workflow: Optional[str],
    prefer_status: str,
) -> str:
    cmd = [
        gh_path,
        "run",
        "list",
        "--limit",
        "50",
        "--json",
        "databaseId,conclusion,displayTitle,createdAt",
    ]
    if workflow:
        cmd.extend(["--workflow", workflow])
    try:
        output = subprocess.check_output(cmd, text=True)
    except subprocess.CalledProcessError as exc:
        raise RuntimeError(f"`{' '.join(cmd)}` failed with exit code {exc.returncode}") from exc

    try:
        runs = json.loads(output)
    except json.JSONDecodeError as exc:
        raise RuntimeError("Failed to parse gh run list output.") from exc

    if not runs:
        raise RuntimeError("No workflow runs available for auto selection.")

    prefer = prefer_status.lower()
    for run in runs:
        conclusion = (run.get("conclusion") or "").lower()
        if prefer == "completed" and conclusion in {"success", "failure", "cancelled"}:
            return str(run.get("databaseId"))
        if conclusion == prefer:
            return str(run.get("databaseId"))
    return str(runs[0].get("databaseId"))


def resolve_job_log_link(
    gh_path: str,
    *,
    run_id: str,
    job_name: Optional[str],
) -> tuple[Optional[str], Optional[str]]:
    cmd = [gh_path, "run", "view", run_id, "--json", "jobs"]
    try:
        output = subprocess.check_output(cmd, text=True)
    except subprocess.CalledProcessError as exc:
        print(f"Warning: failed to inspect job metadata ({exc}).", file=sys.stderr)
        return None, None

    try:
        payload = json.loads(output)
    except json.JSONDecodeError:
        return None, None

    jobs = payload.get("jobs") or []
    target = (job_name or "").strip().lower()
    if target:
        for job in jobs:
            if job.get("name", "").strip().lower() == target:
                return job.get("name"), job.get("html_url")
    for job in jobs:
        conclusion = (job.get("conclusion") or "").lower()
        if conclusion not in {"", "success"}:
            return job.get("name"), job.get("html_url")
    if jobs:
        job = jobs[0]
        return job.get("name"), job.get("html_url")
    return None, None


def main() -> int:
    args = parse_args()

    run_id: Optional[str] = None
    if args.run_id:
        try:
            run_id = derive_run_id(args.run_id)
        except ValueError as exc:
            print(f"Error: {exc}", file=sys.stderr)
            return 1
    elif args.auto_latest or args.latest:
        try:
            run_id = auto_select_latest_run(
                args.gh_path,
                workflow=args.workflow,
                prefer_status=args.run_status,
            )
            print(f"Auto-selected workflow run {run_id}")
        except RuntimeError as exc:
            print(f"Error: {exc}", file=sys.stderr)
            return 1
    elif args.interactive:
        try:
            run_id = interactive_select_run(args.gh_path)
        except RuntimeError as exc:
            print(f"Error: {exc}", file=sys.stderr)
            return 1
    else:
        print("Error: run_id is required unless --interactive or --auto-latest is used.", file=sys.stderr)
        return 1

    output_dir = Path(args.output_dir).expanduser().resolve()
    ensure_directory(output_dir)

    artifact_name = f"{args.artifact_prefix}-{run_id}"
    download_target = output_dir / artifact_name
    ensure_directory(download_target)

    print(f"Downloading artifact '{artifact_name}' for run {run_id} into {download_target}")
    download_with_retries(
        [
            args.gh_path,
            "run",
            "download",
            run_id,
            "--name",
            artifact_name,
            "--dir",
            str(download_target),
        ],
        max(args.max_retries, 0),
    )

    csv_path = find_artifact_file(download_target, "latest.csv")
    summary_path = find_artifact_file(download_target, "latest.summary.json")
    plan_csv_path = find_artifact_file(download_target, "plan.csv")
    plan_md_path = find_artifact_file(download_target, "plan.md")

    if not csv_path:
        print("Warning: latest.csv not found in artifact.", file=sys.stderr)
    else:
        print(f"Located CSV at {csv_path}")

    summary_data: Optional[dict] = None
    if summary_path:
        print(f"Located summary at {summary_path}")
        try:
            summary_text = summary_path.read_text(encoding="utf-8")
            summary_data = json.loads(summary_text)
        except (OSError, json.JSONDecodeError) as exc:
            print(f"Warning: failed to parse summary JSON: {exc}", file=sys.stderr)
    else:
        print("Warning: latest.summary.json not found in artifact.", file=sys.stderr)

    if plan_csv_path:
        print(f"Located plan CSV at {plan_csv_path}")
    if plan_md_path:
        print(f"Located plan Markdown at {plan_md_path}")

    repro_summary = Path(args.summary_out).expanduser()
    repro_summary.parent.mkdir(parents=True, exist_ok=True)

    repro_cmd = [
        "python",
        "tools/plot_coupled_constraint_endurance.py",
        str(csv_path) if csv_path else "path/to/latest.csv",
        "--skip-plot",
        "--summary-json",
        str(repro_summary),
        "--fail-on-max-condition",
        f"{args.threshold_max_condition}",
        "--fail-on-rank-ratio",
        f"{args.threshold_rank_ratio}",
        "--no-show",
    ]

    job_display, job_log_url = resolve_job_log_link(
        args.gh_path,
        run_id=run_id,
        job_name=args.job_name,
    )

    repo_slug = args.repo or os.getenv("GITHUB_REPOSITORY")
    comment_body = build_comment_body(
        run_id=run_id,
        repo=repo_slug,
        artifact_name=artifact_name,
        download_dir=download_target,
        summary=summary_data,
        repro_cmd=repro_cmd,
        plan_csv=plan_csv_path,
        plan_md=plan_md_path,
        job_name=job_display,
        job_url=job_log_url,
    )

    if args.comment_file:
        comment_path = Path(args.comment_file).expanduser()
        comment_path.parent.mkdir(parents=True, exist_ok=True)
        comment_path.write_text(comment_body, encoding="utf-8")
        print(f"Wrote comment Markdown to {comment_path}")

    if args.console_comment_only and args.post_comment:
        print("Console-only mode enabled; skipping GitHub comment posting.", file=sys.stderr)

    should_post_comment = args.post_comment and not args.console_comment_only

    if should_post_comment:
        if not args.comment_target:
            print("Error: --post-comment requires --comment-target.", file=sys.stderr)
            return 1
        target = args.comment_target
        if target.startswith("pr/"):
            number = target.split("/", 1)[1]
            gh_command = [args.gh_path, "pr", "comment", number, "--body-file"]
        elif target.startswith("issue/"):
            number = target.split("/", 1)[1]
            gh_command = [args.gh_path, "issue", "comment", number, "--body-file"]
        else:
            print("Error: --comment-target must start with 'pr/' or 'issue/'.", file=sys.stderr)
            return 1

        with tempfile.NamedTemporaryFile("w", encoding="utf-8", delete=False) as tmp:
            tmp.write(comment_body)
            tmp_path = Path(tmp.name)
        try:
            run_command(gh_command + [str(tmp_path)])
        finally:
            try:
                tmp_path.unlink()
            except OSError:
                pass
        print(f"Posted comment to {target}")

    print("\nComment template preview:\n")
    print(comment_body)

    print("\nReproduction command:")
    print(" ".join(repro_cmd))
    print("\nRemember to install GitHub CLI and authenticate (`gh auth login`) before running this helper.")
    if not csv_path:
        print("After adjusting the CSV path, rerun the command above to reproduce the failure.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
