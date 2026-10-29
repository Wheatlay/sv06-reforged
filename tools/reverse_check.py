"""List options enabled in a reference config (e.g. Sovol stock) whose state/value differs in ours.

Usage: uv run --no-project tools/reverse_check.py <reference.h> <ours.h>
"""
import sys
sys.path.insert(0, __import__("os").path.dirname(__file__))
from port_config import parse, first

ref, _ = parse(sys.argv[1])
ours, _ = parse(sys.argv[2])
for name, occ in ref.items():
    en, val, _ = occ[0]
    if not en or name.endswith("_H_VERSION") or name.startswith("_"):
        continue
    o = first(ours, name)
    if o is None:
        print(f"{name:36} sovol: {val:30} | ours: -- not present")
    elif (o[0], o[1]) != (en, val):
        print(f"{name:36} sovol: {val:30} | ours: {'ON ' if o[0] else 'off'} {o[1]}")
