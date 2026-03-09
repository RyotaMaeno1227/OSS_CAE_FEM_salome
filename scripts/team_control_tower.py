#!/usr/bin/env python3
"""Summarize live team activity and latest acceptance status."""
from __future__ import annotations

import argparse
import json
import os
import time
from dataclasses import asdict, dataclass
from pathlib import Path
import re

from audit_team_sessions import collect_latest_audits, read_text

TEAM_ORDER = ["A", "B", "C", "D", "E"]
TEAM_TAG = {
    "A": "a_team",
    "B": "b_team",
    "C": "c_team",
    "D": "d_team",
    "E": "e_team",
}
DEFAULT_NO_GUARD_GRACE_MINUTES = 12
DEFAULT_STALE_HEARTBEAT_MINUTES = 12
DEFAULT_PLAN_GRACE_MINUTES = 10

TEAM_HEADER_RE = re.compile(r"^##\s+(?:\d+\.\s+)?((?:PM|[ABCDE]))チーム")
TASK_HEADER_RE = re.compile(r"^###\s+((?:PM|[ABCDE])-\d+)(?:\s+.*)?$")
STATUS_RE = re.compile(r"^- Status:\s+`([^`]+)`")
GOAL_RE = re.compile(r"^- Goal:\s*(.+)$")


@dataclass
class Task:
    team: str
    task_id: str
    status: str = ""
    goal: str = ""


@dataclass
class TeamSnapshot:
    team: str
    runtime_state: str
    elapsed_min_now: int | None
    session_token: str | None
    declared_primary_task: str | None
    declared_secondary_task: str | None
    plan_utc: str | None
    progress_count: int | None
    latest_progress_elapsed_min: int | None
    queue_task_id: str | None
    queue_task_status: str | None
    queue_task_goal: str | None
    latest_entry: str
    latest_verdict: str | None
    latest_reasons: list[str]
    next_action: str


def parse_args(argv: list[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Render current team runtime/acceptance snapshot")
    parser.add_argument("--team-status", default="docs/team_status.md")
    parser.add_argument("--queue", default="docs/fem4c_team_next_queue.md")
    parser.add_argument(
        "--state-root",
        default=os.environ.get("TEAM_TIMER_STATE_ROOT", "/tmp/highperformanceFEM_team_timer"),
    )
    parser.add_argument("--min-elapsed", type=int, default=60)
    parser.add_argument("--max-elapsed", type=int, default=90)
    parser.add_argument("--no-guard-grace-minutes", type=int, default=DEFAULT_NO_GUARD_GRACE_MINUTES)
    parser.add_argument("--stale-heartbeat-minutes", type=int, default=DEFAULT_STALE_HEARTBEAT_MINUTES)
    parser.add_argument("--plan-grace-minutes", type=int, default=DEFAULT_PLAN_GRACE_MINUTES)
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--write", help="Write rendered output to the specified file")
    return parser.parse_args(argv)


def parse_queue_tasks(markdown: str) -> dict[str, list[Task]]:
    tasks_by_team: dict[str, list[Task]] = {team: [] for team in TEAM_ORDER}
    current_team: str | None = None
    current_task: Task | None = None

    def flush() -> None:
        nonlocal current_task
        if current_task is None:
            return
        if current_task.team in tasks_by_team:
            tasks_by_team[current_task.team].append(current_task)
        current_task = None

    for raw_line in markdown.splitlines():
        line = raw_line.rstrip()
        team_match = TEAM_HEADER_RE.match(line)
        if team_match:
            flush()
            current_team = team_match.group(1)
            continue

        task_match = TASK_HEADER_RE.match(line)
        if task_match:
            flush()
            task_id = task_match.group(1)
            team = task_id.split("-", 1)[0]
            current_task = Task(team=team, task_id=task_id)
            continue

        if current_task is None or current_team is None:
            continue

        status_match = STATUS_RE.match(line)
        if status_match:
            current_task.status = status_match.group(1).strip()
            continue

        goal_match = GOAL_RE.match(line)
        if goal_match and not current_task.goal:
            current_task.goal = goal_match.group(1).strip()
            continue

    flush()
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
    return None


def parse_env_file(path: Path) -> dict[str, str]:
    data: dict[str, str] = {}
    if not path.is_file():
        return data
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        if "=" not in raw_line:
            continue
        key, value = raw_line.split("=", 1)
        data[key.strip()] = value.strip()
    return data


def load_active_states(state_root: Path) -> dict[str, dict[str, str]]:
    active_dir = state_root / "active"
    if not active_dir.is_dir():
        return {}
    states: dict[str, dict[str, str]] = {}
    for team in TEAM_ORDER:
        env_path = active_dir / f"{TEAM_TAG[team]}.env"
        payload = parse_env_file(env_path)
        if payload:
            states[team] = payload
    return states


def build_next_action(
    team: str,
    runtime_state: str,
    active_elapsed: int | None,
    task: Task | None,
    latest_verdict: str | None,
    latest_reasons: list[str],
) -> str:
    task_label = task.task_id if task else "queue未取得"

    if runtime_state == "STALE_NO_GUARD":
        if task is None:
            return (
                "start だけ記録されて guard が無い短時間 stale session。"
                " queue は進めず、新規 session_token で同一スコープを再開する。"
            )
        return (
            f"{task_label} は start のみで停止した短時間 stale session。"
            " queue は進めず、新規 session_token で同一タスクを再開する。"
        )
    if runtime_state == "STALE_BEFORE_60":
        if task is None:
            return (
                "60分未満で停止した stale session。"
                " queue は進めず、新規 session_token で同一スコープをやり直す。"
            )
        return (
            f"{task_label} は 60分未満で停止した stale session。"
            " queue は進めず、新規 session_token で同一タスクをやり直す。"
        )
    if runtime_state == "STALE_AFTER_60":
        if task is None:
            return (
                "60分以上進んだ stale session。"
                " 終了報告が回収できるなら回収し、無理なら同一スコープで再開する。"
            )
        return (
            f"{task_label} は 60分以上進んだ stale session。"
            " 終了報告が回収できるなら回収し、無理なら同一タスクを再開する。"
        )
    if runtime_state == "ACTIVE_UNCONFIRMED":
        if task is None:
            return (
                "active token は残っているが guard heartbeat が無く実稼働を確認できない。"
                " 継続中なら guard を記録し、停止済みなら end/report を回収する。"
            )
        return (
            f"{task_label} は active token のみで実稼働未確認。"
            " 継続中なら guard を記録し、停止済みなら end/report を回収する。"
        )
    if runtime_state == "PLAN_MISSING":
        if task is None:
            return (
                "start 後 10 分を超えても SESSION_TIMER_DECLARE が無い。"
                " 継続するなら primary/secondary task を宣言し、停止済みなら stale 扱いで再開する。"
            )
        return (
            f"{task_label} は start 後 10 分以内の task 宣言が不足。"
            " `scripts/session_timer_declare.sh <token> <primary> <secondary>` を記録するか、"
            " 停止済みなら新規 session_token で同一タスクをやり直す。"
        )
    if runtime_state == "PROGRESS_MISSING":
        if task is None:
            return (
                "宣言後の実装進捗 heartbeat が不足。"
                " `scripts/session_timer_progress.sh <token> <current_task> <work_kind> [note]` を記録して作業継続を可視化する。"
            )
        return (
            f"{task_label} は SESSION_TIMER_PROGRESS が不足。"
            " 20分以内に1回、40分台でもう1回 progress を残して guard60 まで継続する。"
        )
    if runtime_state == "RUNNING":
        if task is None:
            return "current session を継続。終了時に `Auto-Next` を queue へ追記して再開点を固定する。"
        return f"{task_label} を継続。guard60 未達のため終了報告しない。"
    if runtime_state == "READY_TO_WRAP":
        if task is None:
            return "current session を継続。完了時に `Auto-Next` を queue へ追記し、guard60/pass を確認して終了報告する。"
        return f"{task_label} を継続。完了していれば guard60/pass を確認して終了報告へ進む。"
    if runtime_state == "OVERRUN":
        if task is None:
            return "90分超過。ここで区切って終了報告し、次セッションのため `Auto-Next` を queue へ追加する。"
        return (
            f"{task_label} は 90分超過。ここで区切って終了報告し、次セッションで再開する。"
        )
    if latest_verdict == "PASS":
        if task is None:
            return "次回開始前に `Auto-Next` を queue へ追加してから `作業してください` を送る。"
        return f"次回は `作業してください` のみでよい。先頭タスク {task_label} へ進む。"
    if latest_reasons:
        if task is None:
            return (
                "新規 session_token で再開する前に `Auto-Next` を queue へ追加する。"
                f" まず不受理理由を解消する: {', '.join(latest_reasons)}"
            )
        return (
            f"{task_label} を新規 session_token で再開する。"
            f" まず不受理理由を解消する: {', '.join(latest_reasons)}"
        )
    return f"{task_label} を新規 session_token で開始する。"


def build_snapshots(
    team_status_markdown: str,
    queue_markdown: str,
    state_root: Path,
    min_elapsed: int,
    max_elapsed: int,
    no_guard_grace_minutes: int,
    stale_heartbeat_minutes: int,
    plan_grace_minutes: int,
) -> list[TeamSnapshot]:
    active_states = load_active_states(state_root)
    queue_tasks = parse_queue_tasks(queue_markdown)
    audits = {
        audit.team: audit
        for audit in collect_latest_audits(team_status_markdown, TEAM_ORDER)
    }

    now_epoch = int(time.time())
    snapshots: list[TeamSnapshot] = []
    for team in TEAM_ORDER:
        task = select_head_task(queue_tasks.get(team, []))
        audit = audits[team]
        latest_reasons = audit.failure_reasons(
            min_elapsed=min_elapsed,
            max_elapsed=max_elapsed,
            require_evidence=True,
            require_impl_changes=False,
            max_consecutive_same_command=1,
        )
        latest_verdict = audit.verdict(
            min_elapsed=min_elapsed,
            max_elapsed=max_elapsed,
            require_evidence=True,
            require_impl_changes=False,
            max_consecutive_same_command=1,
        )

        active = active_states.get(team)
        if active:
            start_epoch = int(active.get("start_epoch", "0") or "0")
            elapsed_min_now = max(0, (now_epoch - start_epoch) // 60)
            last_seen_epoch = int(active.get("last_seen_epoch", str(start_epoch)) or start_epoch)
            last_guard_epoch_raw = active.get("last_guard_epoch")
            last_guard_epoch = int(last_guard_epoch_raw) if last_guard_epoch_raw and last_guard_epoch_raw.isdigit() else None
            last_guard_result = active.get("last_guard_result", "")
            plan_epoch_raw = active.get("plan_epoch")
            plan_epoch = int(plan_epoch_raw) if plan_epoch_raw and plan_epoch_raw.isdigit() else None
            progress_count_raw = active.get("progress_count")
            progress_count = int(progress_count_raw) if progress_count_raw and progress_count_raw.isdigit() else 0
            latest_progress_elapsed_raw = active.get("last_progress_elapsed_min")
            latest_progress_elapsed_min = (
                int(latest_progress_elapsed_raw)
                if latest_progress_elapsed_raw and latest_progress_elapsed_raw.isdigit()
                else None
            )
            declared_primary_task = active.get("declared_primary_task")
            declared_secondary_task = active.get("declared_secondary_task")
            plan_utc = active.get("plan_utc")
            elapsed_min_at_last_seen = max(0, (last_seen_epoch - start_epoch) // 60)
            no_guard_timeout = no_guard_grace_minutes * 60
            stale_timeout = stale_heartbeat_minutes * 60
            plan_timeout = plan_grace_minutes * 60
            no_guard_stale = last_guard_epoch is None and (now_epoch - start_epoch) >= no_guard_timeout
            heartbeat_stale = last_guard_epoch is not None and (now_epoch - last_seen_epoch) > stale_timeout
            plan_missing = plan_epoch is None and (now_epoch - start_epoch) >= plan_timeout
            if no_guard_stale:
                runtime_state = "STALE_NO_GUARD"
            elif heartbeat_stale and elapsed_min_at_last_seen < min_elapsed:
                runtime_state = "STALE_BEFORE_60"
            elif heartbeat_stale:
                runtime_state = "STALE_AFTER_60"
            elif plan_missing:
                runtime_state = "PLAN_MISSING"
            elif elapsed_min_now >= 20 and progress_count <= 0:
                runtime_state = "PROGRESS_MISSING"
            elif last_guard_result == "block" and elapsed_min_at_last_seen < min_elapsed:
                runtime_state = "ACTIVE_UNCONFIRMED"
            elif elapsed_min_now < min_elapsed:
                runtime_state = "RUNNING"
            elif max_elapsed > 0 and elapsed_min_now > max_elapsed:
                runtime_state = "OVERRUN"
            else:
                runtime_state = "READY_TO_WRAP"
            session_token = active.get("session_token")
        else:
            elapsed_min_now = None
            session_token = None
            declared_primary_task = None
            declared_secondary_task = None
            plan_utc = None
            progress_count = None
            latest_progress_elapsed_min = None
            runtime_state = "READY_NEXT" if latest_verdict == "PASS" else "NEEDS_REWORK"

        snapshots.append(
            TeamSnapshot(
                team=team,
                runtime_state=runtime_state,
                elapsed_min_now=elapsed_min_now,
                session_token=session_token,
                declared_primary_task=declared_primary_task,
                declared_secondary_task=declared_secondary_task,
                plan_utc=plan_utc,
                progress_count=progress_count,
                latest_progress_elapsed_min=latest_progress_elapsed_min,
                queue_task_id=task.task_id if task else None,
                queue_task_status=task.status if task else None,
                queue_task_goal=task.goal if task else None,
                latest_entry=audit.title,
                latest_verdict=latest_verdict,
                latest_reasons=latest_reasons,
                next_action=build_next_action(
                    team,
                    runtime_state,
                    elapsed_min_now,
                    task,
                    latest_verdict,
                    latest_reasons,
                ),
            )
        )
    return snapshots


def render_markdown(
    snapshots: list[TeamSnapshot],
    min_elapsed: int,
    max_elapsed: int,
) -> str:
    generated_at = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
    active = [s.team for s in snapshots if s.runtime_state in {"RUNNING", "READY_TO_WRAP", "OVERRUN"}]
    active_unconfirmed = [s.team for s in snapshots if s.runtime_state == "ACTIVE_UNCONFIRMED"]
    plan_missing = [s.team for s in snapshots if s.runtime_state == "PLAN_MISSING"]
    progress_missing = [s.team for s in snapshots if s.runtime_state == "PROGRESS_MISSING"]
    stale = [s.team for s in snapshots if s.runtime_state in {"STALE_NO_GUARD", "STALE_BEFORE_60", "STALE_AFTER_60"}]
    ready_next = [s.team for s in snapshots if s.runtime_state == "READY_NEXT"]
    needs_rework = [s.team for s in snapshots if s.runtime_state == "NEEDS_REWORK"]

    lines = [
        "# Team Control Tower Snapshot",
        "",
        f"- generated_at_utc: `{generated_at}`",
        f"- target_window: `{min_elapsed} <= elapsed_min <= {max_elapsed}`",
        f"- active_teams: `{','.join(active) if active else '-'}`",
        f"- active_unconfirmed: `{','.join(active_unconfirmed) if active_unconfirmed else '-'}`",
        f"- plan_missing: `{','.join(plan_missing) if plan_missing else '-'}`",
        f"- progress_missing: `{','.join(progress_missing) if progress_missing else '-'}`",
        f"- stale_sessions: `{','.join(stale) if stale else '-'}`",
        f"- ready_next: `{','.join(ready_next) if ready_next else '-'}`",
        f"- needs_rework: `{','.join(needs_rework) if needs_rework else '-'}`",
        "",
    ]

    for s in snapshots:
        lines.extend(
            [
                f"## {s.team}チーム",
                f"- runtime_state: `{s.runtime_state}`",
                f"- elapsed_min_now: `{s.elapsed_min_now if s.elapsed_min_now is not None else '-'}`",
                f"- session_token: `{s.session_token if s.session_token else '-'}`",
                f"- declared_plan: `{s.declared_primary_task if s.declared_primary_task else '-'} -> {s.declared_secondary_task if s.declared_secondary_task else '-'}`",
                f"- plan_utc: `{s.plan_utc if s.plan_utc else '-'}`",
                f"- progress: `{s.progress_count if s.progress_count is not None else '-'} / {s.latest_progress_elapsed_min if s.latest_progress_elapsed_min is not None else '-'}`",
                f"- queue_head: `{s.queue_task_id if s.queue_task_id else '-'} / {s.queue_task_status if s.queue_task_status else '-'}`",
                f"- queue_goal: {s.queue_task_goal if s.queue_task_goal else '-'}",
                f"- latest_entry: {s.latest_entry}",
                f"- latest_verdict: `{s.latest_verdict if s.latest_verdict else '-'}`",
                f"- latest_reasons: {', '.join(s.latest_reasons) if s.latest_reasons else '-'}",
                f"- next_action: {s.next_action}",
                "",
            ]
        )
    return "\n".join(lines).rstrip() + "\n"


def render_json(
    snapshots: list[TeamSnapshot],
    min_elapsed: int,
    max_elapsed: int,
) -> str:
    payload = {
        "generated_at_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "target_window": {
            "min_elapsed": min_elapsed,
            "max_elapsed": max_elapsed,
        },
        "results": [asdict(snapshot) for snapshot in snapshots],
    }
    return json.dumps(payload, ensure_ascii=False, indent=2)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    team_status_text = read_text(Path(args.team_status))
    queue_text = read_text(Path(args.queue))
    snapshots = build_snapshots(
        team_status_text,
        queue_text,
        Path(args.state_root),
        args.min_elapsed,
        args.max_elapsed,
        args.no_guard_grace_minutes,
        args.stale_heartbeat_minutes,
        args.plan_grace_minutes,
    )
    rendered = (
        render_json(snapshots, args.min_elapsed, args.max_elapsed)
        if args.json
        else render_markdown(snapshots, args.min_elapsed, args.max_elapsed)
    )
    if args.write:
        Path(args.write).write_text(rendered, encoding="utf-8")
    print(rendered, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
