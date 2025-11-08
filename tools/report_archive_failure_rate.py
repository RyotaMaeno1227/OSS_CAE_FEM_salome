#!/usr/bin/env python3
"""Visualize archive-and-summarize job failure rates and emit Slack summaries."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import requests


GITHUB_API_ROOT = "https://api.github.com"


@dataclass
class RunRecord:
    run_id: int
    created_at: dt.datetime
    job_conclusion: Optional[str]


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Fetch archive-and-summarize runs and build weekly failure-rate reports."
    )
    parser.add_argument(
        "--repo",
        required=True,
        help="Target repository in owner/name form.",
    )
    parser.add_argument(
        "--workflow",
        default="coupled_endurance.yml",
        help="Workflow file name or ID (default: %(default)s).",
    )
    parser.add_argument(
        "--job-name",
        default="archive-and-summarize",
        help="Job name to inspect (default: %(default)s).",
    )
    parser.add_argument(
        "--weeks",
        type=int,
        default=8,
        help="Number of weeks to include in the report (default: %(default)s).",
    )
    parser.add_argument(
        "--token",
        help="GitHub token; falls back to GITHUB_TOKEN env if omitted.",
    )
    parser.add_argument(
        "--output-chart",
        required=True,
        help="Path for the generated PNG chart.",
    )
    parser.add_argument(
        "--output-slack",
        required=True,
        help="Path for the Slack payload JSON.",
    )
    parser.add_argument(
        "--output-json",
        help="Optional path for a machine-readable JSON summary.",
    )
    parser.add_argument(
        "--timezone",
        default="UTC",
        help="Timezone label used in chart annotations (no conversion is performed).",
    )
    return parser.parse_args(argv)


def _headers(token: Optional[str]) -> Dict[str, str]:
    headers = {
        "Accept": "application/vnd.github+json",
        "User-Agent": "coupled-endurance-report",
    }
    if token:
        headers["Authorization"] = f"Bearer {token}"
    return headers


def _iso_to_datetime(value: str) -> dt.datetime:
    if value.endswith("Z"):
        value = value[:-1] + "+00:00"
    return dt.datetime.fromisoformat(value)


def _fetch_paginated(
    session: requests.Session,
    url: str,
    headers: Dict[str, str],
    params: Optional[Dict[str, str]] = None,
) -> Iterable[dict]:
    page = 1
    while True:
        page_params = dict(params or {})
        page_params["per_page"] = "100"
        page_params["page"] = str(page)
        response = session.get(url, headers=headers, params=page_params, timeout=30)
        if response.status_code != 200:
            raise RuntimeError(
                f"GitHub API request failed ({response.status_code}): {response.text}"
            )
        payload = response.json()
        items: List[dict] = payload.get("workflow_runs") or payload.get("jobs") or []
        if not items:
            break
        for item in items:
            yield item
        if len(items) < 100:
            break
        page += 1


def fetch_runs_since(
    *,
    session: requests.Session,
    repo: str,
    workflow: str,
    job_name: str,
    since: dt.datetime,
    headers: Dict[str, str],
) -> List[RunRecord]:
    workflow_runs_url = f"{GITHUB_API_ROOT}/repos/{repo}/actions/workflows/{workflow}/runs"
    runs: List[RunRecord] = []
    for run in _fetch_paginated(
        session,
        workflow_runs_url,
        headers,
        params={"status": "completed", "exclude_pull_requests": "true"},
    ):
        created_at = _iso_to_datetime(run["created_at"]).astimezone(dt.timezone.utc)
        if created_at < since:
            # Runs are returned newest-first; stop when we leave the window.
            break
        run_id = run["id"]
        job_conclusion = fetch_job_conclusion(
            session=session,
            repo=repo,
            run_id=run_id,
            job_name=job_name,
            headers=headers,
        )
        runs.append(
            RunRecord(
                run_id=run_id,
                created_at=created_at,
                job_conclusion=job_conclusion,
            )
        )
    return runs


def fetch_job_conclusion(
    *,
    session: requests.Session,
    repo: str,
    run_id: int,
    job_name: str,
    headers: Dict[str, str],
) -> Optional[str]:
    jobs_url = f"{GITHUB_API_ROOT}/repos/{repo}/actions/runs/{run_id}/jobs"
    normalized_target = job_name.strip().lower()
    for job in _fetch_paginated(session, jobs_url, headers):
        if job.get("name", "").strip().lower() == normalized_target:
            return job.get("conclusion") or job.get("status")
    return None


def bucket_by_week(runs: Iterable[RunRecord]) -> List[Tuple[dt.date, Dict[str, object]]]:
    buckets: Dict[dt.date, Dict[str, object]] = defaultdict(
        lambda: {"total": 0, "failures": 0, "run_ids": []}
    )
    for record in runs:
        week_start = (record.created_at.date() - dt.timedelta(days=record.created_at.weekday()))
        bucket = buckets[week_start]
        bucket["total"] += 1
        bucket["run_ids"].append(record.run_id)
        if (record.job_conclusion or "").lower() != "success":
            bucket["failures"] += 1

    ordered = sorted(buckets.items(), key=lambda item: item[0])
    for _, stats in ordered:
        total = stats["total"] or 1
        stats["failure_rate"] = stats["failures"] / total
    return ordered


def write_chart(
    *,
    data: List[Tuple[dt.date, Dict[str, object]]],
    output_path: Path,
    timezone_label: str,
) -> None:
    fig, ax = plt.subplots(figsize=(10, 4))
    if not data:
        ax.text(
            0.5,
            0.5,
            "No workflow runs found in window",
            ha="center",
            va="center",
            fontsize=12,
            transform=ax.transAxes,
        )
        ax.axis("off")
    else:
        labels = [week.strftime("%m/%d") for week, _ in data]
        failure_rates = [stats["failure_rate"] * 100.0 for _, stats in data]
        totals = [stats["total"] for _, stats in data]

        bars = ax.bar(labels, failure_rates, color="#c0392b")
        ax.set_ylabel("Failure rate (%)")
        ax.set_ylim(0, 100)
        ax.set_title(f"archive-and-summarize failures per week ({timezone_label})")
        for bar, rate in zip(bars, failure_rates):
            ax.text(
                bar.get_x() + bar.get_width() / 2,
                bar.get_height() + 1,
                f"{rate:.1f}%",
                ha="center",
                va="bottom",
                fontsize=8,
            )

        ax2 = ax.twinx()
        ax2.plot(labels, totals, color="#2980b9", marker="o", linewidth=2, label="runs")
        ax2.set_ylabel("Runs")
        ax2.grid(False)

    fig.tight_layout()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output_path, dpi=160)
    plt.close(fig)


def build_slack_payload(
    *,
    repo: str,
    workflow: str,
    job_name: str,
    data: List[Tuple[dt.date, Dict[str, object]]],
    weeks: int,
) -> Dict[str, object]:
    if not data:
        text = f"*{job_name}* failure rate: no runs found for workflow `{workflow}`."
        return {"text": text}

    lines = [
        f"*{job_name} failure rate* (workflow `{workflow}`, last {weeks} weeks)",
        f"Repo: `{repo}`",
    ]
    for week, stats in data:
        failures = stats["failures"]
        total = stats["total"]
        rate = stats["failure_rate"] * 100.0
        lines.append(f"- {week.isoformat()}: {failures}/{total} ({rate:.1f}%)")

    return {"text": "\n".join(lines)}


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    token = args.token or os.getenv("GITHUB_TOKEN") or os.getenv("GH_TOKEN")
    headers = _headers(token)

    now = dt.datetime.now(dt.timezone.utc)
    window_start = now - dt.timedelta(weeks=max(args.weeks, 1))

    session = requests.Session()
    runs = fetch_runs_since(
        session=session,
        repo=args.repo,
        workflow=args.workflow,
        job_name=args.job_name,
        since=window_start,
        headers=headers,
    )
    buckets = bucket_by_week(runs)

    write_chart(
        data=buckets,
        output_path=Path(args.output_chart),
        timezone_label=args.timezone,
    )

    slack_payload = build_slack_payload(
        repo=args.repo,
        workflow=args.workflow,
        job_name=args.job_name,
        data=buckets,
        weeks=args.weeks,
    )
    Path(args.output_slack).write_text(json.dumps(slack_payload), encoding="utf-8")

    if args.output_json:
        summary = {
            "generated_at": now.isoformat(),
            "repo": args.repo,
            "workflow": args.workflow,
            "job_name": args.job_name,
            "weeks": args.weeks,
            "buckets": [
                {
                    "week_start": week.isoformat(),
                    "total_runs": stats["total"],
                    "failures": stats["failures"],
                    "failure_rate": stats["failure_rate"],
                    "run_ids": stats["run_ids"],
                }
                for week, stats in buckets
            ],
        }
        Path(args.output_json).write_text(json.dumps(summary, indent=2), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
