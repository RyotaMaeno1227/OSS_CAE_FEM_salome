#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import sys
import tempfile
from datetime import datetime, timezone
from pathlib import Path

STATE_ROOT = Path(os.environ.get("TEAM_TIMER_STATE_ROOT", "/tmp/highperformanceFEM_team_timer"))
ACTIVE_DIR = STATE_ROOT / "active"
LAST_DIR = STATE_ROOT / "last"


def utc_now() -> tuple[str, int]:
    now = datetime.now(timezone.utc)
    return now.strftime("%Y-%m-%dT%H:%M:%SZ"), int(now.timestamp())


def read_env(path: Path) -> dict[str, str]:
    data: dict[str, str] = {}
    if not path.is_file():
        return data
    for raw in path.read_text(encoding="utf-8").splitlines():
        if "=" not in raw:
            continue
        key, value = raw.split("=", 1)
        data[key] = value
    return data


def write_env(path: Path, data: dict[str, str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile("w", delete=False, encoding="utf-8", dir=str(path.parent)) as tmp:
        for key, value in data.items():
            tmp.write(f"{key}={value}\n")
        tmp_path = Path(tmp.name)
    tmp_path.replace(path)


def upsert_env(path: Path, updates: dict[str, str]) -> dict[str, str]:
    data = read_env(path)
    data.update(updates)
    write_env(path, data)
    return data


def require_token(token_path: str) -> Path:
    token = Path(token_path)
    if not token.is_file():
        raise SystemExit(f"ERROR: token file not found: {token_path}")
    return token


def require_numeric(value: str, field: str) -> int:
    try:
        return int(value)
    except Exception as exc:
        raise SystemExit(f"ERROR: {field} is not numeric: {value}") from exc


def cmd_start(team_tag: str) -> int:
    if not team_tag:
        raise SystemExit("ERROR: team_tag is required for start")
    now_utc, now_epoch = utc_now()
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    token = Path(f"/tmp/{team_tag}_session_{stamp}_{os.getpid()}.token")
    token_data = {
        "team_tag": team_tag,
        "start_utc": now_utc,
        "start_epoch": str(now_epoch),
    }
    write_env(token, token_data)
    ACTIVE_DIR.mkdir(parents=True, exist_ok=True)
    LAST_DIR.mkdir(parents=True, exist_ok=True)
    write_env(
        ACTIVE_DIR / f"{team_tag}.env",
        {
            "status": "active",
            "session_token": str(token),
            "team_tag": team_tag,
            "start_utc": now_utc,
            "start_epoch": str(now_epoch),
            "last_seen_utc": now_utc,
            "last_seen_epoch": str(now_epoch),
            "last_seen_source": "start",
        },
    )
    print("SESSION_TIMER_START")
    print(f"session_token={token}")
    for key, value in token_data.items():
        print(f"{key}={value}")
    return 0


def cmd_declare(token_path: str, primary_task: str, secondary_task: str, plan_note: str) -> int:
    if not primary_task or not secondary_task:
        raise SystemExit("ERROR: declare requires <session_token_path> <primary_task> <secondary_task>")
    if primary_task == "-" or secondary_task == "-":
        raise SystemExit("ERROR: primary_task / secondary_task must be explicit task ids")
    token = require_token(token_path)
    token_data = read_env(token)
    team_tag = token_data.get("team_tag", "")
    start_utc = token_data.get("start_utc", "")
    start_epoch = token_data.get("start_epoch", "")
    if not team_tag or not start_utc or not start_epoch:
        raise SystemExit(f"ERROR: token file is malformed: {token}")
    plan_utc, plan_epoch = utc_now()
    updates = {
        "declared_primary_task": primary_task,
        "declared_secondary_task": secondary_task,
        "plan_utc": plan_utc,
        "plan_epoch": str(plan_epoch),
        "plan_note": plan_note,
    }
    upsert_env(token, updates)
    upsert_env(
        ACTIVE_DIR / f"{team_tag}.env",
        {
            "status": "active",
            "session_token": str(token),
            "team_tag": team_tag,
            "start_utc": start_utc,
            "start_epoch": start_epoch,
            "last_seen_utc": plan_utc,
            "last_seen_epoch": str(plan_epoch),
            "last_seen_source": "declare",
            **updates,
        },
    )
    print("SESSION_TIMER_DECLARE")
    print(f"session_token={token}")
    print(f"team_tag={team_tag}")
    print(f"primary_task={primary_task}")
    print(f"secondary_task={secondary_task}")
    print(f"plan_utc={plan_utc}")
    print(f"plan_epoch={plan_epoch}")
    print(f"plan_note={plan_note}")
    return 0


def cmd_progress(token_path: str, current_task: str, work_kind: str, progress_note: str) -> int:
    if not current_task or not work_kind:
        raise SystemExit("ERROR: progress requires <session_token_path> <current_task> <work_kind>")
    token = require_token(token_path)
    token_data = read_env(token)
    team_tag = token_data.get("team_tag", "")
    start_utc = token_data.get("start_utc", "")
    start_epoch = require_numeric(token_data.get("start_epoch", ""), "start_epoch")
    if not team_tag or not start_utc:
        raise SystemExit(f"ERROR: token file is malformed: {token}")
    progress_utc, progress_epoch = utc_now()
    elapsed_sec = max(0, progress_epoch - start_epoch)
    elapsed_min = elapsed_sec // 60
    current_count = int(token_data.get("progress_count", "0") or "0")
    progress_count = current_count + 1
    updates = {
        "progress_count": str(progress_count),
        "last_progress_task": current_task,
        "last_progress_kind": work_kind,
        "last_progress_note": progress_note,
        "last_progress_utc": progress_utc,
        "last_progress_epoch": str(progress_epoch),
        "last_progress_elapsed_min": str(elapsed_min),
    }
    upsert_env(token, updates)
    upsert_env(
        ACTIVE_DIR / f"{team_tag}.env",
        {
            "status": "active",
            "session_token": str(token),
            "team_tag": team_tag,
            "start_utc": start_utc,
            "start_epoch": str(start_epoch),
            "last_seen_utc": progress_utc,
            "last_seen_epoch": str(progress_epoch),
            "last_seen_source": "progress",
            **updates,
        },
    )
    print("SESSION_TIMER_PROGRESS")
    print(f"session_token={token}")
    print(f"team_tag={team_tag}")
    print(f"current_task={current_task}")
    print(f"work_kind={work_kind}")
    print(f"progress_note={progress_note}")
    print(f"progress_utc={progress_utc}")
    print(f"progress_epoch={progress_epoch}")
    print(f"elapsed_min={elapsed_min}")
    print(f"progress_count={progress_count}")
    return 0


def cmd_guard(token_path: str, min_elapsed: int) -> int:
    token = require_token(token_path)
    token_data = read_env(token)
    team_tag = token_data.get("team_tag", "")
    start_utc = token_data.get("start_utc", "")
    start_epoch = require_numeric(token_data.get("start_epoch", ""), "start_epoch")
    if not team_tag or not start_utc:
        raise SystemExit(f"ERROR: token file is malformed: {token}")
    now_utc, now_epoch = utc_now()
    elapsed_sec = max(0, now_epoch - start_epoch)
    elapsed_min = elapsed_sec // 60
    guard_result = "pass" if elapsed_min >= min_elapsed else "block"
    active_updates = {
        "status": "active",
        "session_token": str(token),
        "team_tag": team_tag,
        "start_utc": start_utc,
        "start_epoch": str(start_epoch),
        "last_seen_utc": now_utc,
        "last_seen_epoch": str(now_epoch),
        "last_seen_source": "guard",
        "last_guard_utc": now_utc,
        "last_guard_epoch": str(now_epoch),
        "last_guard_min_required": str(min_elapsed),
        "last_guard_result": guard_result,
        "progress_count": token_data.get("progress_count", ""),
        "last_progress_task": token_data.get("last_progress_task", ""),
        "last_progress_kind": token_data.get("last_progress_kind", ""),
        "last_progress_note": token_data.get("last_progress_note", ""),
        "last_progress_utc": token_data.get("last_progress_utc", ""),
        "last_progress_epoch": token_data.get("last_progress_epoch", ""),
        "last_progress_elapsed_min": token_data.get("last_progress_elapsed_min", ""),
    }
    upsert_env(ACTIVE_DIR / f"{team_tag}.env", active_updates)
    print("SESSION_TIMER_GUARD")
    print(f"session_token={token}")
    print(f"team_tag={team_tag}")
    print(f"start_utc={start_utc}")
    print(f"now_utc={now_utc}")
    print(f"start_epoch={start_epoch}")
    print(f"now_epoch={now_epoch}")
    print(f"elapsed_sec={elapsed_sec}")
    print(f"elapsed_min={elapsed_min}")
    print(f"min_required={min_elapsed}")
    print(f"guard_result={guard_result}")
    return 0 if guard_result == "pass" else 1


def cmd_end(token_path: str) -> int:
    token = require_token(token_path)
    token_data = read_env(token)
    team_tag = token_data.get("team_tag", "")
    start_utc = token_data.get("start_utc", "")
    start_epoch = require_numeric(token_data.get("start_epoch", ""), "start_epoch")
    if not team_tag or not start_utc:
        raise SystemExit(f"ERROR: token file is malformed: {token}")
    end_utc, end_epoch = utc_now()
    elapsed_sec = max(0, end_epoch - start_epoch)
    elapsed_min = elapsed_sec // 60
    last_lines = {
        "status": "ended",
        "session_token": str(token),
        "team_tag": team_tag,
        "start_utc": start_utc,
        "end_utc": end_utc,
        "start_epoch": str(start_epoch),
        "end_epoch": str(end_epoch),
        "elapsed_sec": str(elapsed_sec),
        "elapsed_min": str(elapsed_min),
    }
    for key in (
        "declared_primary_task",
        "declared_secondary_task",
        "plan_utc",
        "plan_epoch",
        "plan_note",
        "progress_count",
        "last_progress_task",
        "last_progress_kind",
        "last_progress_note",
        "last_progress_utc",
        "last_progress_epoch",
        "last_progress_elapsed_min",
    ):
        value = token_data.get(key, "")
        if value:
            last_lines[key] = value
    write_env(LAST_DIR / f"{team_tag}.env", last_lines)
    active_path = ACTIVE_DIR / f"{team_tag}.env"
    active_data = read_env(active_path)
    if active_data.get("session_token") in ("", str(token)):
        try:
            active_path.unlink()
        except FileNotFoundError:
            pass
    print("SESSION_TIMER_END")
    print(f"session_token={token}")
    print(f"team_tag={team_tag}")
    print(f"start_utc={start_utc}")
    print(f"end_utc={end_utc}")
    print(f"start_epoch={start_epoch}")
    print(f"end_epoch={end_epoch}")
    print(f"elapsed_sec={elapsed_sec}")
    print(f"elapsed_min={elapsed_min}")
    for key in (
        "progress_count",
        "last_progress_task",
        "last_progress_kind",
        "last_progress_note",
        "last_progress_utc",
        "last_progress_epoch",
        "last_progress_elapsed_min",
    ):
        value = token_data.get(key, "")
        if value:
            print(f"{key}={value}")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Canonical repository team timer")
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("start")
    p.add_argument("team_tag")

    p = sub.add_parser("declare")
    p.add_argument("session_token")
    p.add_argument("primary_task")
    p.add_argument("secondary_task")
    p.add_argument("plan_note", nargs="?", default="")

    p = sub.add_parser("progress")
    p.add_argument("session_token")
    p.add_argument("current_task")
    p.add_argument("work_kind")
    p.add_argument("progress_note", nargs="?", default="")

    p = sub.add_parser("guard")
    p.add_argument("session_token")
    p.add_argument("min_elapsed", nargs="?", type=int, default=30)

    p = sub.add_parser("end")
    p.add_argument("session_token")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.command == "start":
        return cmd_start(args.team_tag)
    if args.command == "declare":
        return cmd_declare(args.session_token, args.primary_task, args.secondary_task, args.plan_note)
    if args.command == "progress":
        return cmd_progress(args.session_token, args.current_task, args.work_kind, args.progress_note)
    if args.command == "guard":
        return cmd_guard(args.session_token, args.min_elapsed)
    if args.command == "end":
        return cmd_end(args.session_token)
    raise SystemExit(2)


if __name__ == "__main__":
    sys.exit(main())
