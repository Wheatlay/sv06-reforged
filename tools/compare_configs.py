"""Compare option values between two configs for the names an example changes vs. its default.

Usage: uv run --no-project tools/compare_configs.py <default.h> <example.h> <other.h>
Prints names where <example.h> and <other.h> (e.g. Sovol stock) disagree.
"""
import sys
sys.path.insert(0, __import__("os").path.dirname(__file__))
from port_config import changes, parse, first

default, example, other = sys.argv[1:4]
o, _ = parse(other)
for name, en, val, kind in changes(default, example):
    if kind == "removed-in-example":
        continue
    so = first(o, name)
    sval = "-- not present" if so is None else f"{'ON ' if so[0] else 'off'} {so[1]}"
    if so is None or (so[0], so[1]) != (en, val):
        print(f"{name:36} official: {'ON ' if en else 'off'} {val:28} | sovol: {sval}")
