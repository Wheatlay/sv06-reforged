"""Port an example Marlin config onto another Marlin version's default config.

Takes the options an example config changes relative to its own default (e.g. official SV-06 vs. bugfix default)
and applies those changes to a target default config (e.g. Marlin 2.1.2.9). Options the target doesn't know are
reported, never silently dropped.

Usage:
  uv run --no-project tools/port_config.py diff  <default.h> <example.h>
  uv run --no-project tools/port_config.py apply <default.h> <example.h> <target.h> <out.h>
  uv run --no-project tools/port_config.py show  <config.h> NAME [NAME...]
  uv run --no-project tools/port_config.py set   <config.h> NAME=value NAME -NAME ...
"""
import re
import sys
from pathlib import Path

DEFINE = re.compile(r"^(?P<indent>\s*)(?P<off>//\s*)?#define\s+(?P<name>[A-Za-z_]\w*)(?P<rest>.*)$")


def strip_comment(rest):
    """Value part of a #define line, without the trailing // comment."""
    out, i, depth_q = [], 0, None
    while i < len(rest):
        c = rest[i]
        if depth_q:
            out.append(c)
            if c == "\\" and i + 1 < len(rest):
                out.append(rest[i + 1]); i += 1
            elif c == depth_q:
                depth_q = None
        elif c in "\"'":
            depth_q = c; out.append(c)
        elif rest.startswith("//", i):
            break
        else:
            out.append(c)
        i += 1
    return "".join(out).strip()


def parse(path):
    """name -> list of (enabled, value, line_index); a name can appear more than once (inside #if branches)."""
    opts = {}
    lines = Path(path).read_text(encoding="utf-8", errors="replace").splitlines()
    for i, line in enumerate(lines):
        m = DEFINE.match(line)
        if not m:
            continue
        # Skip commented examples inside block comments ("* #define ...") - they don't match DEFINE anyway.
        opts.setdefault(m["name"], []).append((m["off"] is None, strip_comment(m["rest"]), i))
    return opts, lines


def first(opts, name):
    return opts[name][0] if name in opts else None


def changes(default, example):
    """Options whose enabled-state or value differs, as (name, enabled, value) from the example."""
    d, _ = parse(default)
    e, _ = parse(example)
    out = []
    for name, occ in e.items():
        en, val, _ = occ[0]
        dd = first(d, name)
        if dd is None:
            out.append((name, en, val, "new"))
        elif (dd[0], dd[1]) != (en, val):
            out.append((name, en, val, "changed"))
    for name in d:
        if name not in e:
            out.append((name, None, None, "removed-in-example"))
    return out


def apply(default, example, target, out_path):
    t_opts, t_lines = parse(target)
    report = []
    for name, en, val, kind in changes(default, example):
        if kind == "removed-in-example":
            continue
        if name not in t_opts:
            if en:
                report.append(f"MISSING in target (enabled in example): {name} {val}")
            continue
        if len(t_opts[name]) > 1:
            report.append(f"MULTIPLE definitions in target, edited first only, check by hand: {name}")
        _, _, idx = t_opts[name][0]
        m = DEFINE.match(t_lines[idx])
        comment = m["rest"][len(m["rest"].rstrip()) - len(m["rest"].rstrip()):]
        tail = ""
        cpos = m["rest"].find("//")
        if cpos != -1 and strip_comment(m["rest"]) == strip_comment(m["rest"][:cpos]):
            tail = "  " + m["rest"][cpos:].strip()
        value = f" {val}" if val else ""
        t_lines[idx] = f"{m['indent']}{'' if en else '//'}#define {name}{value}{tail}"
        report.append(f"{'SET ' if en else 'OFF '} {name}{value}")
    Path(out_path).write_text("\n".join(t_lines) + "\n", encoding="utf-8")
    return report


def set_options(path, specs):
    """Apply 'NAME=value', 'NAME' (enable as-is) or '-NAME' (disable) to the first definition of each name."""
    opts, lines = parse(path)
    for spec in specs:
        off = spec.startswith("-")
        name, _, val = spec.lstrip("-").partition("=")
        if name not in opts:
            raise SystemExit(f"{name}: not in {path}")
        en0, val0, idx = opts[name][0]
        m = DEFINE.match(lines[idx])
        cpos = m["rest"].find("//")
        tail = "  " + m["rest"][cpos:].strip() if cpos != -1 and strip_comment(m["rest"]) == strip_comment(m["rest"][:cpos]) else ""
        v = val if val else val0
        lines[idx] = f"{m['indent']}{'//' if off else ''}#define {name}{' ' + v if v else ''}{tail}"
        print(f"{'OFF' if off else 'SET'} {name} {v}   (was {'ON' if en0 else 'off'} {val0})")
    Path(path).write_text("\n".join(lines) + "\n", encoding="utf-8")


def main():
    cmd, *args = sys.argv[1:]
    if cmd == "set":
        return set_options(args[0], args[1:])
    if cmd == "diff":
        for name, en, val, kind in changes(*args):
            if kind != "removed-in-example":
                print(f"{kind:8} {'ON ' if en else 'off'} {name} {val}")
    elif cmd == "apply":
        for line in apply(*args):
            print(line)
    elif cmd == "show":
        opts, _ = parse(args[0])
        for name in args[1:]:
            for en, val, idx in opts.get(name, []):
                print(f"{name:32} {'ON ' if en else 'off'} {val}   (line {idx + 1})")
            if name not in opts:
                print(f"{name:32} -- not present")


if __name__ == "__main__":
    main()
