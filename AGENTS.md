# Repository Agent Rules

## Default Workflow
- User to Codex: `確認してください`
- User to team chats: `作業してください`
- When the user says `確認してください`, first inspect:
  - `docs/team_status.md`
  - `docs/session_continuity_log.md`
  - `docs/fem4c_team_next_queue.md`
  - `python3 tools/team_timer/team_control_tower.py`
  - `python3 tools/team_timer/audit_team_sessions.py`

## Team Run Contract
- Default team session length is `60-90` minutes.
- A run is acceptable only if all of the following exist in `docs/team_status.md`:
  - `SESSION_TIMER_START`
  - `SESSION_TIMER_DECLARE`
  - two `SESSION_TIMER_PROGRESS` entries
  - `SESSION_TIMER_GUARD` with `guard60=pass`
  - `SESSION_TIMER_END`
- Use `SESSION_TIMER_END elapsed_min` as the source of truth, not IDE UI time.
- Ignore verbal completion claims without formal timer evidence.
- Canonical timer commands are:
  - `python3 tools/team_timer/team_timer.py start <team_tag>`
  - `python3 tools/team_timer/team_timer.py declare <session_token> <primary_task> <secondary_task> [plan_note]`
  - `python3 tools/team_timer/team_timer.py progress <session_token> <current_task> <work_kind> [progress_note]`
  - `python3 tools/team_timer/team_timer.py guard <session_token> <min_elapsed_minutes>`
  - `python3 tools/team_timer/team_timer.py end <session_token>`
- Legacy `scripts/session_timer*.sh` wrappers are compatibility shims only and are not the primary interface.

## Mid-Run Behavior
- During an active run, teams should only reply for:
  - blockers
  - destructive conflicts
  - data-loss risk
- Routine progress belongs in `SESSION_TIMER_PROGRESS`, not chat.
- If a run stops before `guard60=pass`, treat it as stale and rerun the same task with a new session token.

## Queue Source of Truth
- The current restart point lives in `docs/fem4c_team_next_queue.md`.
- If a task is accepted, update the next restart point there before telling teams to continue.
- If a team has no next task, PM must define one before sending `作業してください`.

## E-Team Special Case
- E-team is not accepted unless the formal log contains an explicit `SESSION_TIMER_END` block.
- If E-team writes a rich completion note but misses `SESSION_TIMER_END`, treat it as rerun-required.
