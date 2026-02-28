#!/usr/bin/env python3
"""Simple mock server to inspect Coupled Endurance webhook payloads locally."""

from __future__ import annotations

import argparse
import http.server
import json
import logging
from pathlib import Path
from typing import Optional


class MockWebhookHandler(http.server.BaseHTTPRequestHandler):
    log_dir: Path = Path("mock_webhook_logs")

    def _write_response(self, code: int, message: str) -> None:
        self.send_response(code)
        self.send_header("Content-Type", "text/plain; charset=utf-8")
        self.end_headers()
        self.wfile.write(message.encode("utf-8"))

    def do_POST(self) -> None:  # noqa: N802
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length) if length else b""
        try:
            payload = json.loads(body.decode("utf-8")) if body else None
        except json.JSONDecodeError:
            payload = {"raw": body.decode("utf-8", errors="replace")}

        self.log_message("Received payload: %s", json.dumps(payload, indent=2))
        filename = MockWebhookHandler.log_dir / "webhook_payload.json"
        filename.parent.mkdir(parents=True, exist_ok=True)
        filename.write_text(json.dumps(payload, indent=2), encoding="utf-8")
        self._write_response(200, "ok\n")

    def log_message(self, format: str, *args: object) -> None:  # noqa: A003
        logging.info("%s - %s", self.address_string(), format % args)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Mock webhook server for Coupled Endurance alerts.")
    parser.add_argument("--host", default="127.0.0.1", help="Bind address (default: %(default)s).")
    parser.add_argument("--port", type=int, default=9000, help="Port to listen on (default: %(default)s).")
    parser.add_argument(
        "--log-dir",
        default="mock_webhook_logs",
        help="Directory to store received payloads (default: %(default)s).",
    )
    return parser.parse_args()


def run_server(host: str, port: int, log_dir: str) -> None:
    MockWebhookHandler.log_dir = Path(log_dir)
    server = http.server.HTTPServer((host, port), MockWebhookHandler)
    logging.info("Mock webhook server listening on %s:%d", host, port)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        logging.info("Stopping server...")
        server.shutdown()


def main() -> int:
    args = parse_args()
    logging.basicConfig(level=logging.INFO, format="[%(asctime)s] %(message)s")
    run_server(args.host, args.port, args.log_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
