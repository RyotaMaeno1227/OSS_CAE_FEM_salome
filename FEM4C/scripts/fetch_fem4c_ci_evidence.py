#!/usr/bin/env python3
"""Fetch FEM4C CI run evidence from GitHub Actions API.

This script summarizes:
- latest run status/conclusion for a workflow
- outcome of "Run FEM4C regression entrypoint" step
- presence of the "chrono-tests" artifact

It can scan multiple recent runs and prefer the newest run that contains
an explicit FEM4C step result.
"""

from __future__ import annotations

import argparse
import json
import os
import time
import sys
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime, timezone
from typing import Any, Dict, List, Optional, Tuple


def build_request(url: str, token: Optional[str]) -> urllib.request.Request:
    req = urllib.request.Request(url)
    req.add_header("Accept", "application/vnd.github+json")
    req.add_header("User-Agent", "fem4c-ci-evidence-fetcher")
    if token:
        req.add_header("Authorization", f"Bearer {token}")
    return req


def get_json(url: str, token: Optional[str]) -> Dict[str, Any]:
    req = build_request(url, token)
    with urllib.request.urlopen(req, timeout=20) as response:
        return json.load(response)


def parse_repo(value: str) -> str:
    value = value.strip()
    if value.startswith("https://github.com/"):
        value = value[len("https://github.com/") :]
    return value.rstrip("/").replace(".git", "")


def find_step_outcome(jobs_payload: Dict[str, Any], step_name: str) -> str:
    for job in jobs_payload.get("jobs", []):
        for step in job.get("steps", []):
            if step.get("name") == step_name:
                return str(step.get("conclusion") or step.get("status") or "unknown")
    return "missing"


def has_artifact(artifacts_payload: Dict[str, Any], artifact_name: str) -> bool:
    for artifact in artifacts_payload.get("artifacts", []):
        if artifact.get("name") == artifact_name:
            return True
    return False


def build_runs_url(base: str, workflow: str, scan_runs: int) -> str:
    scan_runs = max(1, min(scan_runs, 100))
    quoted = urllib.parse.quote(workflow)
    return f"{base}/actions/workflows/{quoted}/runs?per_page={scan_runs}"


def safe_int(value: Optional[str]) -> Optional[int]:
    if value is None:
        return None
    try:
        return int(value)
    except (TypeError, ValueError):
        return None


def format_rate_limit_reset(reset_epoch: Optional[int]) -> str:
    if reset_epoch is None:
        return "unknown"
    return datetime.fromtimestamp(reset_epoch, timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def emit_structured_error(error_type: str, **fields: object) -> None:
    print("CI_EVIDENCE_ERROR")
    print(f"error_type={error_type}")
    for key, value in fields.items():
        print(f"{key}={value}")


def choose_run_with_step(
    base: str,
    runs: List[Dict[str, Any]],
    token: Optional[str],
    step_name: str,
) -> Tuple[Dict[str, Any], Dict[str, Any], str, int]:
    """Prefer newest run containing the target step; fallback to latest run."""
    if not runs:
        raise ValueError("no workflow runs found")

    latest_run = runs[0]
    latest_run_id = latest_run.get("id")
    latest_jobs_url = f"{base}/actions/runs/{latest_run_id}/jobs?per_page=100"
    latest_jobs_payload = get_json(latest_jobs_url, token)
    latest_step_outcome = find_step_outcome(latest_jobs_payload, step_name)

    if latest_step_outcome != "missing":
        return latest_run, latest_jobs_payload, latest_step_outcome, 1

    for idx, run in enumerate(runs[1:], start=2):
        run_id = run.get("id")
        jobs_url = f"{base}/actions/runs/{run_id}/jobs?per_page=100"
        jobs_payload = get_json(jobs_url, token)
        step_outcome = find_step_outcome(jobs_payload, step_name)
        if step_outcome != "missing":
            return run, jobs_payload, step_outcome, idx

    return latest_run, latest_jobs_payload, latest_step_outcome, 1


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--repo",
        required=True,
        help="GitHub repository (owner/repo or full https URL)",
    )
    parser.add_argument(
        "--workflow",
        default="ci.yaml",
        help="Workflow file name or workflow id (default: ci.yaml)",
    )
    parser.add_argument(
        "--step-name",
        default="Run FEM4C regression entrypoint",
        help="Step name to inspect in build job",
    )
    parser.add_argument(
        "--artifact-name",
        default="chrono-tests",
        help="Artifact name that should exist (default: chrono-tests)",
    )
    parser.add_argument(
        "--scan-runs",
        type=int,
        default=20,
        help="Number of recent runs to scan for step evidence (default: 20, max: 100)",
    )
    parser.add_argument(
        "--run-id",
        type=int,
        default=0,
        help="Optional explicit run id to inspect (skips workflow-runs scan)",
    )
    parser.add_argument(
        "--strict-acceptance",
        action="store_true",
        help="Exit non-zero when step evidence or required artifact is missing",
    )
    args = parser.parse_args()

    token = os.getenv("GITHUB_TOKEN")
    repo = parse_repo(args.repo)
    base = f"https://api.github.com/repos/{repo}"

    try:
        selected_index = 1
        if args.run_id > 0:
            run_id = int(args.run_id)
            run_payload = get_json(f"{base}/actions/runs/{run_id}", token)
            status = str(run_payload.get("status") or "unknown")
            conclusion = str(run_payload.get("conclusion") or "unknown")
            run_url = str(run_payload.get("html_url") or "")
            jobs_url = f"{base}/actions/runs/{run_id}/jobs?per_page=100"
            jobs_payload = get_json(jobs_url, token)
            step_outcome = find_step_outcome(jobs_payload, args.step_name)
            selected_index = 0
        else:
            runs_url = build_runs_url(base, args.workflow, args.scan_runs)
            runs_payload = get_json(runs_url, token)
            runs = runs_payload.get("workflow_runs", [])
            if not runs:
                print("ERROR: no workflow runs found")
                return 2

            run, jobs_payload, step_outcome, selected_index = choose_run_with_step(
                base=base,
                runs=runs,
                token=token,
                step_name=args.step_name,
            )

            run_id = run.get("id")
            status = str(run.get("status") or "unknown")
            conclusion = str(run.get("conclusion") or "unknown")
            run_url = str(run.get("html_url") or "")

        artifacts_url = f"{base}/actions/runs/{run_id}/artifacts?per_page=100"
        artifacts_payload = get_json(artifacts_url, token)
        artifact_present = has_artifact(artifacts_payload, args.artifact_name)

        step_present = step_outcome != "missing"
        acceptance_result = "pass" if (step_present and artifact_present) else "fail"

        print("CI_EVIDENCE")
        print(f"repo={repo}")
        print(f"workflow={args.workflow}")
        print(f"scan_runs={max(1, min(args.scan_runs, 100))}")
        print(f"requested_run_id={args.run_id if args.run_id > 0 else 'auto'}")
        print(f"selected_run_index={selected_index}")
        print(f"run_id={run_id}")
        print(f"run_url={run_url}")
        print(f"status={status}")
        print(f"conclusion={conclusion}")
        print(f"step_name={args.step_name}")
        print(f"step_outcome={step_outcome}")
        print(f"step_present={'yes' if step_present else 'no'}")
        print(f"artifact_name={args.artifact_name}")
        print(f"artifact_present={'yes' if artifact_present else 'no'}")
        print("acceptance_threshold=step_present==yes && artifact_present==yes")
        print(f"acceptance_result={acceptance_result}")
        print(f"fetched_at_utc={datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')}")

        if args.strict_acceptance and acceptance_result != "pass":
            return 2
        return 0
    except urllib.error.HTTPError as err:
        reset_epoch = safe_int(err.headers.get("X-RateLimit-Reset"))
        remaining = err.headers.get("X-RateLimit-Remaining", "unknown")
        limit = err.headers.get("X-RateLimit-Limit", "unknown")
        reset_utc = format_rate_limit_reset(reset_epoch)
        if err.code == 403 and remaining == "0":
            now_epoch = int(time.time())
            retry_after_sec = "unknown" if reset_epoch is None else max(0, reset_epoch - now_epoch)
            emit_structured_error(
                "rate_limit",
                error_code=err.code,
                limit=limit,
                remaining=remaining,
                reset_utc=reset_utc,
                retry_after_sec=retry_after_sec,
            )
            print(
                "ERROR: GitHub API rate limit exceeded "
                f"(limit={limit}, remaining={remaining}, reset_utc={reset_utc}, retry_after_sec={retry_after_sec})"
            )
            return 2
        emit_structured_error("http", error_code=err.code, error_reason=err.reason)
        print(f"ERROR: GitHub API HTTP {err.code}: {err.reason}")
        return 2
    except urllib.error.URLError as err:
        emit_structured_error("url", error_reason=err.reason)
        print(f"ERROR: GitHub API URL failure: {err.reason}")
        return 2
    except TimeoutError:
        emit_structured_error("timeout")
        print("ERROR: GitHub API timeout")
        return 2
    except Exception as err:  # defensive fallback
        emit_structured_error("unexpected", error_reason=err)
        print(f"ERROR: unexpected failure: {err}")
        return 2


if __name__ == "__main__":
    sys.exit(main())
