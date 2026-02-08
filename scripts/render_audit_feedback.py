#!/usr/bin/env python3
"""Render copy-paste team feedback from audit_team_sessions JSON output."""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


def parse_args(argv: list[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Render A/B/C chat feedback from audit_team_sessions --json output"
    )
    parser.add_argument("audit_json", help="Path to JSON file emitted by audit_team_sessions.py --json")
    return parser.parse_args(argv)


def load_json(path: Path) -> dict[str, object]:
    try:
        text = path.read_text(encoding="utf-8")
    except OSError as exc:
        raise SystemExit(f"ERROR: failed to read {path}: {exc}") from exc
    try:
        data = json.loads(text)
    except json.JSONDecodeError as exc:
        raise SystemExit(f"ERROR: invalid JSON in {path}: {exc}") from exc
    if not isinstance(data, dict) or "results" not in data:
        raise SystemExit("ERROR: unexpected JSON format (missing results)")
    return data


def as_list_of_dicts(value: object) -> list[dict[str, object]]:
    if not isinstance(value, list):
        return []
    out: list[dict[str, object]] = []
    for item in value:
        if isinstance(item, dict):
            out.append(item)
    return out


def to_team_tag(team: str) -> str:
    return {
        "A": "a_team",
        "B": "b_team",
        "C": "c_team",
    }.get(team, f"{team.lower()}_team")


def render_entry(entry: dict[str, object], min_elapsed: int) -> str:
    team = str(entry.get("team", "?"))
    verdict = str(entry.get("verdict", "FAIL"))
    elapsed = entry.get("elapsed_min")
    elapsed_text = "-" if elapsed is None else str(elapsed)
    reasons_raw = entry.get("reasons", [])
    reasons = [str(r) for r in reasons_raw] if isinstance(reasons_raw, list) else []
    reason_text = " / ".join(reasons) if reasons else "理由記載なし"
    tag = to_team_tag(team)

    if verdict == "PASS":
        return (
            f"@{team}-team\n"
            "受入OKです。次タスクを継続してください。\n\n"
            "[確認結果]\n"
            f"- verdict: PASS\n"
            f"- elapsed_min: {elapsed_text}\n"
            "- 引き続き `scripts/session_timer.sh` の原文を `docs/team_status.md` へ転記してください。\n"
        )

    return (
        f"@{team}-team\n"
        "今回の報告は受入不可です。同一タスクを継続して再提出してください。\n\n"
        "[監査結果]\n"
        f"- verdict: FAIL\n"
        f"- elapsed_min: {elapsed_text}（基準: >= {min_elapsed}）\n"
        f"- reasons: {reason_text}\n\n"
        "[再提出ルール]\n"
        f"- 開始: `scripts/session_timer.sh start {tag}`\n"
        "- 終了: `scripts/session_timer.sh end <session_token>`\n"
        "- `elapsed_min >= {min_elapsed}` を満たすまで同一セッションで継続\n"
        "- `sleep` 等の人工待機は禁止\n"
        "- `docs/team_status.md` には変更ファイル・実行コマンド・pass/fail を必ず記録\n"
    ).replace("{min_elapsed}", str(min_elapsed))


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    payload = load_json(Path(args.audit_json))
    results = as_list_of_dicts(payload.get("results"))
    min_elapsed = int(payload.get("threshold_min_elapsed", 30))
    if not results:
        print("ERROR: empty results", file=sys.stderr)
        return 2

    print("# Audit Feedback (copy-paste)\n")
    for entry in results:
        print(render_entry(entry, min_elapsed))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
