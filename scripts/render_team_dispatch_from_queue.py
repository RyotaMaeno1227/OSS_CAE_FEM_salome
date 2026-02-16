#!/usr/bin/env python3
"""Render copy-paste A/B/C dispatch messages from docs/fem4c_team_next_queue.md."""
from __future__ import annotations

import argparse
import re
from dataclasses import dataclass
from pathlib import Path

TEAM_LABEL = {
    "A": ("A-team", "a_team"),
    "B": ("B-team", "b_team"),
    "C": ("C-team", "c_team"),
}

TEAM_HEADER_RE = re.compile(r"^##\s+([ABC])チーム")
TASK_HEADER_RE = re.compile(r"^###\s+([ABC]-\d+)\s+(.+)$")
STATUS_RE = re.compile(r"^- Status:\s+`([^`]+)`")
GOAL_RE = re.compile(r"^- Goal:\s*(.+)$")


@dataclass
class Task:
    team: str
    task_id: str
    title: str
    status: str = ""
    goal: str = ""


def parse_args(argv: list[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Render chat instructions from fem4c_team_next_queue.md"
    )
    parser.add_argument(
        "--queue",
        default="docs/fem4c_team_next_queue.md",
        help="Path to next queue markdown",
    )
    parser.add_argument(
        "--team",
        choices=("A", "B", "C", "all"),
        default="all",
        help="Render only one team or all teams",
    )
    return parser.parse_args(argv)


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except OSError as exc:
        raise SystemExit(f"ERROR: failed to read {path}: {exc}") from exc


def parse_tasks(markdown: str) -> dict[str, list[Task]]:
    tasks_by_team: dict[str, list[Task]] = {"A": [], "B": [], "C": []}
    current_team = ""
    current_task: Task | None = None

    def flush_task() -> None:
        nonlocal current_task
        if current_task is None:
            return
        tasks_by_team[current_task.team].append(current_task)
        current_task = None

    for raw_line in markdown.splitlines():
        line = raw_line.rstrip()
        team_match = TEAM_HEADER_RE.match(line)
        if team_match:
            flush_task()
            current_team = team_match.group(1)
            continue

        task_match = TASK_HEADER_RE.match(line)
        if task_match and current_team:
            flush_task()
            current_task = Task(
                team=current_team,
                task_id=task_match.group(1),
                title=task_match.group(2).strip(),
            )
            continue

        if current_task is None:
            continue

        status_match = STATUS_RE.match(line)
        if status_match:
            current_task.status = status_match.group(1).strip()
            continue

        goal_match = GOAL_RE.match(line)
        if goal_match and not current_task.goal:
            current_task.goal = goal_match.group(1).strip()

    flush_task()
    return tasks_by_team


def select_head_task(tasks: list[Task]) -> Task | None:
    if not tasks:
        return None
    for preferred in ("in progress", "todo"):
        for task in tasks:
            if task.status.strip().lower() == preferred:
                return task
    for task in tasks:
        if task.status.strip().lower() != "done":
            return task
    return tasks[0]


def render_team_message(team: str, task: Task | None) -> str:
    mention, timer_tag = TEAM_LABEL[team]
    if task is None:
        task_line = "- 先頭タスクを取得できませんでした。`docs/fem4c_team_next_queue.md` を確認して補完してください。"
        goal_line = "- Goal: (未取得)"
    else:
        task_line = f"- {task.task_id}: {task.title}（Status: {task.status or 'N/A'}）"
        goal_line = f"- Goal: {task.goal or '(未記載)'}"

    return (
        f"@{mention}\n"
        "作業を継続してください（30分以上、推奨30-45分の連続実行）。\n\n"
        "[今回の先頭タスク]\n"
        f"{task_line}\n"
        f"{goal_line}\n\n"
        "[参照先]\n"
        "- docs/abc_team_chat_handoff.md（Section 0）\n"
        "- docs/fem4c_team_next_queue.md\n"
        "- docs/team_runbook.md\n\n"
        "[必須ルール]\n"
        f"- 開始: scripts/session_timer.sh start {timer_tag}\n"
        "- 報告前: bash scripts/session_timer_guard.sh <session_token> 30\n"
        "- 終了: scripts/session_timer.sh end <session_token>\n"
        "- 長時間反復ソーク/人工待機は禁止\n"
        "- 実装差分を1件以上作り、結果を docs/team_status.md / docs/session_continuity_log.md に記録\n"
    )


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    tasks_by_team = parse_tasks(read_text(Path(args.queue)))
    teams = ["A", "B", "C"] if args.team == "all" else [args.team]

    print("# Dispatch Messages (copy-paste)\n")
    for idx, team in enumerate(teams):
        task = select_head_task(tasks_by_team.get(team, []))
        print(render_team_message(team, task))
        if idx != len(teams) - 1:
            print()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
