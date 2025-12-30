#!/usr/bin/env python3
import argparse
import subprocess
import sys
import time


def main() -> int:
    parser = argparse.ArgumentParser(description="Run a command and report elapsed time.")
    parser.add_argument("--label", default="command", help="label for timing output")
    parser.add_argument("--max-seconds", type=float, default=0.0, help="warn if elapsed exceeds this")
    parser.add_argument("command", nargs=argparse.REMAINDER, help="command to run after --")
    args = parser.parse_args()

    cmd = args.command
    if cmd and cmd[0] == "--":
        cmd = cmd[1:]
    if not cmd:
        print("No command provided", file=sys.stderr)
        return 2

    start = time.perf_counter()
    proc = subprocess.run(cmd)
    elapsed = time.perf_counter() - start
    print(f"[timing] {args.label}: {elapsed:.3f}s")
    if args.max_seconds > 0.0 and elapsed > args.max_seconds:
        print(
            f"[timing] WARN: {args.label} exceeded {args.max_seconds:.2f}s",
            file=sys.stderr,
        )
    return proc.returncode


if __name__ == "__main__":
    raise SystemExit(main())
