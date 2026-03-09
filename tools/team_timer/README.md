## Team Timer Module

Canonical timer module for this repository.

Use this module for all team-run timing operations:

- `python3 tools/team_timer/team_timer.py start <team_tag>`
- `python3 tools/team_timer/team_timer.py declare <session_token> <primary_task> <secondary_task> [plan_note]`
- `python3 tools/team_timer/team_timer.py progress <session_token> <current_task> <work_kind> [progress_note]`
- `python3 tools/team_timer/team_timer.py guard <session_token> <min_elapsed_minutes>`
- `python3 tools/team_timer/team_timer.py end <session_token>`

The module emits the same `SESSION_TIMER_*` markers used by the current audit
stack, but stores live state under a new root:

- `/tmp/highperformanceFEM_team_timer`

Legacy `scripts/session_timer*.sh` commands are compatibility shims and are no
longer the canonical interface.
