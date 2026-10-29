"""Talk to the printer over USB serial: print what it says, send G-code commands, print the replies.

Usage: uv run --no-project --with pyserial tools/serial_cmd.py COM4 [--listen SECONDS] [CMD ...]
  --listen N   read for N seconds before sending (catches boot output if the port open resets the board)
Each command waits for "ok" (or 15 s) before the next one is sent.
"""
import sys
import time

import serial


def read_until_ok(s, timeout):
    end = time.time() + timeout
    lines = []
    while time.time() < end:
        line = s.readline().decode("utf-8", "replace").rstrip()
        if not line:
            continue
        print("  <", line)
        lines.append(line)
        if line.startswith("ok"):
            break
    return lines


def main():
    port, args = sys.argv[1], sys.argv[2:]
    listen = 2.0
    if args[:1] == ["--listen"]:
        listen, args = float(args[1]), args[2:]
    sys.stdout.reconfigure(encoding="utf-8")
    with serial.Serial(port, 115200, timeout=0.5) as s:
        end = time.time() + listen
        while time.time() < end:
            line = s.readline().decode("utf-8", "replace").rstrip()
            if line:
                print("  <", line)
        for cmd in args:
            print(">", cmd)
            s.write((cmd + "\n").encode())
            read_until_ok(s, 15)


if __name__ == "__main__":
    main()
