#!/usr/bin/env python3
"""
Compose coupled endurance notification payloads for Slack/email.
"""

from __future__ import annotations

import argparse
import json
from html import escape
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
        "--output-email-html",
        help="Optional HTML file for the email body (simple <pre> template).",
    )
    parser.add_argument(
        "--summary-validation-report",
        help="Optional Markdown summary of schema validation results to embed.",
    )
    parser.add_argument(
        "--diagnostics-report",
        help="Optional diagnostics_log_report console capture to include.",
    )
    parser.add_argument(
        "--diagnostics-log",
        help="Optional diagnostics Markdown (e.g. latest.diagnostics.md) to embed.",
    )
    parser.add_argument(
        "--summary-validation-json",
        help="Optional JSON file describing schema validation status.",
    )
    parser.add_argument(
        "--plan-lint-json",
        help="Optional JSON file describing lint_endurance_plan results.",
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


def _collapsible_block(title: str, body: str) -> str:
    return f"<details><summary>{title}</summary>\n\n{body.strip()}\n\n</details>"


def _format_json_block(title: str, payload: Dict[str, Any]) -> str:
    snippet = json.dumps(payload, indent=2)
    return _collapsible_block(title, snippet[:4000])


def _extract_validation_status(markdown: Optional[str]) -> Optional[str]:
    if not markdown:
        return None
    for line in markdown.splitlines():
        if line.strip().lower().startswith("- status"):
            return line.split(":", 1)[-1].strip().strip("*")
    return None


def _extract_status_from_json(payload: Optional[Dict[str, Any]]) -> Optional[str]:
    if not payload:
        return None
    status = payload.get("status")
    if isinstance(status, str):
        return status
    return None


def _format_status_line(payload: Optional[Dict[str, Any]]) -> Optional[str]:
    if not payload:
        return None
    status = payload.get("status")
    if not isinstance(status, str):
        return None
    message = payload.get("message") or payload.get("skip_reason")
    if isinstance(message, str) and message:
        return f"{status} ({message})"
    return status


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
    summary_validation_json: Optional[Dict[str, Any]],
    plan_lint_json: Optional[Dict[str, Any]],
    diagnostics_report: Optional[str],
    diagnostics_markdown: Optional[str],
    run_id: str,
    run_url: str,
    status: str,
    artifact_name: str,
) -> Dict[str, Any]:
    icon, status_label = _status_tokens(status)
    validation_status = (
        _format_status_line(summary_validation_json)
        or _extract_validation_status(summary_validation)
        or _extract_status_from_json(summary_validation_json)
    )
    plan_lint_status = _format_status_line(plan_lint_json) or _extract_status_from_json(plan_lint_json)

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

    caption_lines = [
        f"*Status:* {status_label}",
        f"*Artifact:* `{artifact_name}`",
        "*Tags:* `COUPLED` `ENDURANCE`",
    ]
    if validation_status:
        caption_lines.append(f"*Schema validation:* {validation_status}")
    if plan_lint_status:
        caption_lines.append(f"*Plan lint:* {plan_lint_status}")

    plan_excerpt = _plan_excerpt(plan_markdown)
    attachments = [
        {
            "title": "Run details",
            "title_link": run_url,
            "text": "\n".join(caption_lines),
            "fields": key_fields,
            "footer": f"Run ID: {run_id}",
            "mrkdwn_in": ["text"],
        }
    ]

    if plan_excerpt:
        attachments.append(
            {
                "title": "Plan overview (excerpt)",
                "text": _collapsible_block("Plan overview", plan_excerpt),
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
                "text": _collapsible_block("Validation markdown", summary_validation[:4000]),
                "mrkdwn_in": ["text"],
            }
        )
    if summary_validation_json:
        attachments.append(
            {
                "title": "Schema validation (JSON)",
                "text": _format_json_block("validation.json", summary_validation_json),
                "mrkdwn_in": ["text"],
            }
        )
    if plan_lint_json:
        attachments.append(
            {
                "title": "Plan lint report",
                "text": _format_json_block("plan_lint.json", plan_lint_json),
                "mrkdwn_in": ["text"],
            }
        )
    if diagnostics_report:
        attachments.append(
            {
                "title": "Diagnostics console (truncated)",
                "text": _collapsible_block(
                    "diagnostic_log_report output", diagnostics_report[:3500]
                ),
                "mrkdwn_in": ["text"],
            }
        )
    if diagnostics_markdown:
        attachments.append(
            {
                "title": "Diagnostics markdown",
                "text": _collapsible_block("latest.diagnostics.md", diagnostics_markdown[:3500]),
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
    summary_validation_json: Optional[Dict[str, Any]],
    plan_lint_json: Optional[Dict[str, Any]],
    diagnostics_report: Optional[str],
    diagnostics_markdown: Optional[str],
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
    if summary_validation_json:
        lines.append("Schema validation (JSON):")
        lines.append(json.dumps(summary_validation_json, indent=2))
        lines.append("")

    if plan_lint_json:
        lines.append("Plan lint report:")
        lines.append(json.dumps(plan_lint_json, indent=2))
        lines.append("")

    if diagnostics_report:
        lines.append("Diagnostics console excerpt:")
        lines.append(diagnostics_report)
        lines.append("")
    if diagnostics_markdown:
        lines.append("Diagnostics markdown excerpt:")
        lines.append(diagnostics_markdown)
        lines.append("")

    lines.append("--\nAutomated notification generated by compose_endurance_notification.py")
    return "\n".join(lines)


def main() -> int:
    args = parse_args()
    summary = _load_json(args.summary_json)
    plan_md = _load_plan_markdown(args.plan_markdown)
    summary_validation = _load_text(args.summary_validation_report)
    summary_validation_json = _load_json(args.summary_validation_json)
    plan_lint_json = _load_json(args.plan_lint_json)
    diagnostics_report = _load_text(args.diagnostics_report)
    diagnostics_markdown = _load_text(args.diagnostics_log)

    slack_payload = build_slack_payload(
        summary=summary,
        plan_markdown=plan_md,
        summary_validation=summary_validation,
        summary_validation_json=summary_validation_json,
        plan_lint_json=plan_lint_json,
        diagnostics_report=diagnostics_report,
        diagnostics_markdown=diagnostics_markdown,
        run_id=args.run_id,
        run_url=args.run_url,
        status=args.status,
        artifact_name=args.artifact_name,
    )
    email_body = build_email_body(
        summary=summary,
        plan_markdown=plan_md,
        summary_validation=summary_validation,
        summary_validation_json=summary_validation_json,
        plan_lint_json=plan_lint_json,
        diagnostics_report=diagnostics_report,
        diagnostics_markdown=diagnostics_markdown,
        run_id=args.run_id,
        run_url=args.run_url,
        status=args.status,
        artifact_name=args.artifact_name,
    )

    Path(args.output_slack).write_text(json.dumps(slack_payload), encoding="utf-8")
    Path(args.output_email).write_text(email_body, encoding="utf-8")
    if args.output_email_html:
        Path(args.output_email_html).write_text(_render_html_email(email_body), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
def _render_html_email(text: str) -> str:
    escaped = escape(text)
    return """<html>
  <head>
    <meta charset="utf-8"/>
    <style>
      body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; line-height: 1.4; padding: 1.5rem; background: #f8f9fb; color: #1f2a35; }
      pre { background: #1e1e1e; color: #f5f5f5; padding: 1rem; border-radius: 8px; overflow-x: auto; white-space: pre-wrap; font-family: "Fira Code", Menlo, Consolas, monospace; }
      a { color: #0366d6; }
    </style>
  </head>
  <body>
    <pre>""" + escaped + "</pre>\n  </body>\n</html>"
