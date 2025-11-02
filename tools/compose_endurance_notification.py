#!/usr/bin/env python3
"""
Compose coupled endurance notification payloads for Slack/email.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any, Dict, Optional


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Compose coupled endurance notification payloads.")
    parser.add_argument("--summary-json", required=False, help="Path to latest.summary.json (optional).")
    parser.add_argument("--plan-markdown", required=False, help="Path to plan markdown (optional).")
    parser.add_argument("--run-id", required=True, help="Workflow run ID.")
    parser.add_argument("--run-url", required=True, help="Workflow run URL.")
    parser.add_argument("--status", required=True, help="Job status string (e.g. FAILURE).")
    parser.add_argument("--artifact-name", required=True, help="Artifact name for reference.")
    parser.add_argument("--output-slack", required=True, help="File path to write Slack JSON payload.")
    parser.add_argument("--output-email", required=True, help="File path to write email text.")
    return parser.parse_args()


def _load_json(path: Optional[str]) -> Optional[Dict[str, Any]]:
    if not path:
        return None
    json_path = Path(path)
    if not json_path.exists():
        return None
    try:
        return json.loads(json_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return {"error": "Failed to parse summary JSON"}


def _load_plan_markdown(path: Optional[str]) -> Optional[str]:
    if not path:
        return None
    md_path = Path(path)
    if not md_path.exists():
        return None
    return md_path.read_text(encoding="utf-8")


def _format_metric(name: str, value: Any) -> str:
    if isinstance(value, float):
        return f"{value:.3e}" if abs(value) >= 1000 or value == 0 else f"{value:.3f}"
    return str(value)


def build_slack_payload(
    *,
    summary: Optional[Dict[str, Any]],
    plan_markdown: Optional[str],
    run_id: str,
    run_url: str,
    status: str,
    artifact_name: str,
) -> Dict[str, Any]:
    fields = []
    if summary:
        samples = summary.get("samples")
        version = summary.get("version")
        max_condition = summary.get("max_condition")
        warn_ratio = summary.get("warn_ratio")
        rank_ratio = summary.get("rank_ratio")
        duration = summary.get("duration")
        if version is not None:
            fields.append({"title": "Summary version", "value": str(version), "short": True})
        if samples is not None:
            fields.append({"title": "Samples", "value": str(samples), "short": True})
        if isinstance(max_condition, (int, float)):
            fields.append({"title": "Max condition", "value": f"{max_condition:.3e}", "short": True})
        if isinstance(warn_ratio, (int, float)):
            fields.append({"title": "Warning ratio", "value": f"{warn_ratio:.3%}", "short": True})
        if isinstance(rank_ratio, (int, float)):
            fields.append({"title": "Rank ratio", "value": f"{rank_ratio:.3%}", "short": True})
        if isinstance(duration, (int, float)):
            fields.append({"title": "Duration [s]", "value": f"{duration:.3f}", "short": True})

    attachments = [
        {
            "title": "Run details",
            "title_link": run_url,
            "fields": fields,
            "footer": f"Artifact: {artifact_name}",
        }
    ]

    if summary:
        attachments.append(
            {
                "title": "latest.summary.json (excerpt)",
                "text": json.dumps(summary, indent=2)[:4000],
                "mrkdwn_in": ["text"],
            }
        )

    if plan_markdown:
        attachments.append(
            {
                "title": "Plan overview",
                "text": plan_markdown[:4000],
                "mrkdwn_in": ["text"],
            }
        )

    return {
        "text": f"[Coupled Endurance] {status.upper()} - Run {run_id}",
        "attachments": attachments,
    }


def build_email_body(
    *,
    summary: Optional[Dict[str, Any]],
    plan_markdown: Optional[str],
    run_id: str,
    run_url: str,
    status: str,
    artifact_name: str,
) -> str:
    lines = [
        f"Coupled Endurance job status: {status.upper()}",
        f"Run: {run_url}",
        f"Artifact: {artifact_name}",
        "",
    ]

    if summary:
        lines.append("Summary metrics:")
        for key in ["version", "samples", "max_condition", "warn_ratio", "rank_ratio", "duration"]:
            if key in summary:
                lines.append(f"  - {key}: {_format_metric(key, summary[key])}")
        lines.append("")

    lines.append("Reproduction steps:")
    lines.append("  1. Download artifact using tools/fetch_endurance_artifact.py (see run comment).")
    lines.append("  2. Execute the generated reproduction command.")
    lines.append("")

    if plan_markdown:
        lines.append("Plan overview:")
        lines.append(plan_markdown)
        lines.append("")

    lines.append("--\nAutomated notification generated by compose_endurance_notification.py")
    return "\n".join(lines)


def main() -> int:
    args = parse_args()
    summary = _load_json(args.summary_json)
    plan_md = _load_plan_markdown(args.plan_markdown)

    slack_payload = build_slack_payload(
        summary=summary,
        plan_markdown=plan_md,
        run_id=args.run_id,
        run_url=args.run_url,
        status=args.status,
        artifact_name=args.artifact_name,
    )
    email_body = build_email_body(
        summary=summary,
        plan_markdown=plan_md,
        run_id=args.run_id,
        run_url=args.run_url,
        status=args.status,
        artifact_name=args.artifact_name,
    )

    Path(args.output_slack).write_text(json.dumps(slack_payload), encoding="utf-8")
    Path(args.output_email).write_text(email_body, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
