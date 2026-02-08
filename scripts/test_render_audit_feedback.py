#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import tempfile
import unittest
from pathlib import Path


class RenderAuditFeedbackTest(unittest.TestCase):
    def test_render_fail_and_pass_entries(self):
        payload = {
            "threshold_min_elapsed": 30,
            "results": [
                {
                    "team": "A",
                    "verdict": "FAIL",
                    "elapsed_min": 19,
                    "reasons": ["elapsed_min<30"],
                },
                {
                    "team": "B",
                    "verdict": "PASS",
                    "elapsed_min": 33,
                    "reasons": [],
                },
            ],
        }
        with tempfile.TemporaryDirectory() as tmpdir:
            path = Path(tmpdir) / "audit.json"
            path.write_text(json.dumps(payload, ensure_ascii=False), encoding="utf-8")
            cp = subprocess.run(
                ["python", "scripts/render_audit_feedback.py", str(path)],
                capture_output=True,
                text=True,
                check=True,
            )
        out = cp.stdout
        self.assertIn("@A-team", out)
        self.assertIn("受入不可", out)
        self.assertIn("elapsed_min: 19", out)
        self.assertIn("@B-team", out)
        self.assertIn("受入OK", out)


if __name__ == "__main__":
    unittest.main()
