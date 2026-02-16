#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
import unittest
from pathlib import Path

from render_team_dispatch_from_queue import parse_tasks, select_head_task


SAMPLE_QUEUE = """# FEM4C Team Next Queue

## Aチーム（実装）
### A-100 First task
- Status: `Done`
- Goal: first done

### A-101 Current task
- Status: `In Progress`
- Goal: continue current

## Bチーム（検証）
### B-200 Upcoming task
- Status: `Todo`
- Goal: upcoming

## Cチーム（差分整理）
### C-300 Completed task
- Status: `Done`
- Goal: done
"""


class RenderTeamDispatchFromQueueTest(unittest.TestCase):
    def test_selects_in_progress_first(self) -> None:
        tasks = parse_tasks(SAMPLE_QUEUE)
        selected = select_head_task(tasks["A"])
        self.assertIsNotNone(selected)
        assert selected is not None
        self.assertEqual("A-101", selected.task_id)
        self.assertEqual("In Progress", selected.status)

    def test_falls_back_to_todo_when_no_in_progress(self) -> None:
        tasks = parse_tasks(SAMPLE_QUEUE)
        selected = select_head_task(tasks["B"])
        self.assertIsNotNone(selected)
        assert selected is not None
        self.assertEqual("B-200", selected.task_id)
        self.assertEqual("Todo", selected.status)

    def test_cli_renders_copy_paste_block(self) -> None:
        with tempfile.TemporaryDirectory() as tmpdir:
            queue_path = Path(tmpdir) / "queue.md"
            queue_path.write_text(SAMPLE_QUEUE, encoding="utf-8")
            cp = subprocess.run(
                [
                    "python",
                    "scripts/render_team_dispatch_from_queue.py",
                    "--queue",
                    str(queue_path),
                    "--team",
                    "A",
                ],
                check=True,
                text=True,
                capture_output=True,
            )
        out = cp.stdout
        self.assertIn("@A-team", out)
        self.assertIn("A-101", out)
        self.assertIn("scripts/session_timer.sh start a_team", out)


if __name__ == "__main__":
    unittest.main()
