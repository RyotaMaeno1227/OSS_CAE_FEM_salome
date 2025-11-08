#!/usr/bin/env python3
"""
Compose coupled endurance notification payloads for Slack/email.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any, Dict, List, Optional


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
    parser.add_argument(
        "--summary-validation-report",
        help="Optional Markdown summary of schema validation results to embed.",
    )
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


def _load_text(path: Optional[str]) -> Optional[str]:
    if not path:
        return None
    text_path = Path(path)
    if not text_path.exists():
        return None
    return text_path.read_text(encoding="utf-8")


def _format_metric(name: str, value: Any) -> str:
    if isinstance(value, float):
        return f"{value:.3e}" if abs(value) >= 1000 or value == 0 else f"{value:.3f}"
    return str(value)


def _plan_excerpt(plan_markdown: Optional[str], max_lines: int = 12) -> Optional[str]:
    if not plan_markdown:
        return None
    lines: List[str] = []
    for line in plan_markdown.splitlines():
        if line.strip():
            lines.append(line.rstrip())
        if len(lines) == max_lines:
            break
    if not lines:
        return None
    if len(lines) == max_lines:
        lines.append("…")
    return "\n".join(lines)


def _status_tokens(status: str) -> tuple[str, str]:
    normalized = status.strip().upper()
    if normalized == "SUCCESS":
        return ":white_check_mark:", "clear"
    if normalized in {"FAILED", "FAILURE"}:
        return ":x:", "failure"
    if normalized == "CANCELLED":
        return ":warning:", "cancelled"
    return ":information_source:", normalized.lower()


def build_slack_payload(
    *,
    summary: Optional[Dict[str, Any]],
    plan_markdown: Optional[str],
    summary_validation: Optional[str],
    run_id: str,
    run_url: str,
    status: str,
    artifact_name: str,
) -> Dict[str, Any]:
    icon, status_label = _status_tokens(status)

    key_fields = []
    if summary:
        samples = summary.get("samples")
        version = summary.get("version")
        max_condition = summary.get("max_condition")
        warn_ratio = summary.get("warn_ratio")
        rank_ratio = summary.get("rank_ratio")
        duration = summary.get("duration")
        if version is not None:
            key_fields.append({"title": "Summary version", "value": str(version), "short": True})
        if samples is not None:
            key_fields.append({"title": "Samples", "value": str(samples), "short": True})
        if isinstance(max_condition, (int, float)):
            key_fields.append({"title": "Max condition", "value": f"{max_condition:.3e}", "short": True})
        if isinstance(warn_ratio, (int, float)):
            key_fields.append({"title": "Warning ratio", "value": f"{warn_ratio:.3%}", "short": True})
        if isinstance(rank_ratio, (int, float)):
            key_fields.append({"title": "Rank ratio", "value": f"{rank_ratio:.3%}", "short": True})
        if isinstance(duration, (int, float)):
            key_fields.append({"title": "Duration [s]", "value": f"{duration:.3f}", "short": True})

    plan_excerpt = _plan_excerpt(plan_markdown)
    attachments = [
        {
            "title": "Run details",
            "title_link": run_url,
            "text": (
                f"*Status:* {status_label}\n"
                f"*Artifact:* `{artifact_name}`\n"
                f"*Tags:* `COUPLED` `ENDURANCE`"
            ),
            "fields": key_fields,
            "footer": f"Run ID: {run_id}",
            "mrkdwn_in": ["text"],
        }
    ]

    if plan_excerpt:
        attachments.append(
            {
                "title": "Plan overview (excerpt)",
                "text": plan_excerpt,
                "mrkdwn_in": ["text"],
            }
        )

    if summary:
        attachments.append(
            {
                "title": "latest.summary.json (excerpt)",
                "text": json.dumps(summary, indent=2)[:4000],
                "mrkdwn_in": ["text"],
            }
        )
    if summary_validation:
        attachments.append(
            {
                "title": "Schema validation",
                "text": summary_validation[:4000],
                "mrkdwn_in": ["text"],
            }
        )

    attachments.append(
        {
            "title": "Reproduction steps",
            "text": "\n".join(
                [
                    "1. Download artifact via `tools/fetch_endurance_artifact.py`.",
                    "2. Execute the repro command inside the artifact README.",
                    "3. Compare KPI deltas against appendix thresholds before re-running CI.",
                ]
            ),
            "mrkdwn_in": ["text"],
        }
    )

    return {
        "text": f"{icon} [Coupled Endurance] {status.upper()} - Run {run_id}",
        "attachments": attachments,
    }


def build_email_body(
    *,
    summary: Optional[Dict[str, Any]],
    plan_markdown: Optional[str],
    summary_validation: Optional[str],
    run_id: str,
    run_url: str,
    status: str,
    artifact_name: str,
) -> str:
    icon, status_label = _status_tokens(status)
    lines = [
        f"[Coupled Endurance] {icon} status: {status_label.upper()}",
        f"Run URL: {run_url}",
        f"Artifact: {artifact_name}",
        "",
        "Key metrics:",
    ]

    summary_keys = ["version", "samples", "max_condition", "warn_ratio", "rank_ratio", "duration"]
    for key in summary_keys:
        if summary and key in summary:
            lines.append(f"  - {key}: {_format_metric(key, summary[key])}")

    if summary:
        lines.append("")
        lines.append("Full JSON is attached in the artifact (`latest.summary.json`).")
        lines.append("")

    lines.append("Reproduction steps:")
    lines.append("  1. `python tools/fetch_endurance_artifact.py <run-id> --output-dir repro/latest`")
    lines.append("  2. Run the generated repro command inside the downloaded folder.")
    lines.append("  3. Share findings in #chrono-constraints using the Appendix C template.")
    lines.append("")

    if plan_markdown:
        excerpt = _plan_excerpt(plan_markdown, max_lines=20)
        if excerpt:
            lines.append("Plan overview excerpt:")
            lines.append(excerpt)
            lines.append("")

    if summary_validation:
        lines.append("Schema validation:")
        lines.append(summary_validation)
        lines.append("")

    lines.append("--\nAutomated notification generated by compose_endurance_notification.py")
    return "\n".join(lines)


def main() -> int:
    args = parse_args()
    summary = _load_json(args.summary_json)
    plan_md = _load_plan_markdown(args.plan_markdown)
    summary_validation = _load_text(args.summary_validation_report)

    slack_payload = build_slack_payload(
        summary=summary,
        plan_markdown=plan_md,
        summary_validation=summary_validation,
        run_id=args.run_id,
        run_url=args.run_url,
        status=args.status,
        artifact_name=args.artifact_name,
    )
    email_body = build_email_body(
        summary=summary,
        plan_markdown=plan_md,
        summary_validation=summary_validation,
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
