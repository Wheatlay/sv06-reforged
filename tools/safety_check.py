"""Safety gate: our config must never be hotter, faster or stronger than Sovol's stock V2.0.0_A, and the built
firmware must fit the GD32's fast flash and start above the bootloader.

Usage (from the project root):
  uv run --no-project tools/safety_check.py            # config checks only
  uv run --no-project tools/safety_check.py --bin      # also check the built firmware .bin
Exit code 1 on any failure.
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from port_config import parse, first  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
OURS = ROOT / "firmware/Marlin"
SOVOL = ROOT / "sovol-stock/Marlin"
ENV = "GD32F103RET6_sovol_maple"
MAX_BIN = 256 * 1024 - 28 * 1024  # first 256 KB run at full speed on the GD32, minus the 28 KB bootloader
APP_ADDRESS = "0x08007000"

# (name, rule): "eq" same value, "le" ours <= sovol, "ge" ours >= sovol, "on" must be enabled,
# "user:N" must equal the value the user tuned on the printer.
# Lists like { 500, 500, 10, 30 } are compared element by element.
RULES = {
    "Configuration.h": [
        ("TEMP_SENSOR_0", "eq"), ("TEMP_SENSOR_BED", "eq"),
        ("HEATER_0_MAXTEMP", "le"), ("BED_MAXTEMP", "le"), ("HEATER_0_MINTEMP", "ge"), ("BED_MINTEMP", "ge"),
        ("THERMAL_PROTECTION_HOTENDS", "on"), ("THERMAL_PROTECTION_BED", "on"),
        ("DEFAULT_AXIS_STEPS_PER_UNIT", "eq"),
        ("DEFAULT_MAX_FEEDRATE", "le"), ("DEFAULT_MAX_ACCELERATION", "le"),
        ("DEFAULT_ACCELERATION", "le"), ("DEFAULT_RETRACT_ACCELERATION", "le"), ("DEFAULT_TRAVEL_ACCELERATION", "le"),
        ("X_BED_SIZE", "eq"), ("Y_BED_SIZE", "eq"), ("Z_MAX_POS", "eq"), ("X_MIN_POS", "eq"), ("Y_MIN_POS", "eq"),
        ("INVERT_X_DIR", "eq"), ("INVERT_Y_DIR", "eq"), ("INVERT_Z_DIR", "eq"), ("INVERT_E0_DIR", "eq"),
        ("NOZZLE_TO_PROBE_OFFSET", "eq"), ("MOTHERBOARD", "eq"),
    ],
    "Configuration_adv.h": [
        ("USE_WATCHDOG", "on"),  # Stricter than Sovol (they have it off); EEPROM save feeds it, see eeprom_bl24cxx.cpp
        ("HOTEND0_PULLUP_RESISTOR_OHMS", "eq"), ("HOTEND0_RESISTANCE_25C_OHMS", "eq"), ("HOTEND0_BETA", "eq"),
        ("THERMAL_PROTECTION_PERIOD", "le"), ("THERMAL_PROTECTION_HYSTERESIS", "le"),
        ("THERMAL_PROTECTION_BED_PERIOD", "le"), ("THERMAL_PROTECTION_BED_HYSTERESIS", "le"),
        ("WATCH_TEMP_PERIOD", "le"), ("WATCH_TEMP_INCREASE", "ge"),
        ("WATCH_BED_TEMP_PERIOD", "le"), ("WATCH_BED_TEMP_INCREASE", "ge"),
        ("X_CURRENT", "le"), ("Y_CURRENT", "le"), ("Z_CURRENT", "le"), ("E0_CURRENT", "le"),
        ("X_RSENSE", "eq"), ("Y_RSENSE", "eq"), ("Z_RSENSE", "eq"), ("E0_RSENSE", "eq"),
        ("X_STALL_SENSITIVITY", "eq"), ("Y_STALL_SENSITIVITY", "eq"),
        ("PROBING_MARGIN_LEFT", "ge"), ("PROBING_MARGIN_RIGHT", "ge"),
        ("PROBING_MARGIN_FRONT", "ge"), ("PROBING_MARGIN_BACK", "ge"),
    ],
}


def numbers(val):
    try:
        return [float(x) for x in re.findall(r"-?\d+(?:\.\d+)?", val)] if re.fullmatch(r"[\s{},.\d-]+", val) else None
    except ValueError:
        return None


def check_config():
    fails = []
    for fname, rules in RULES.items():
        ours, _ = parse(OURS / fname)
        sovol, _ = parse(SOVOL / fname)
        for name, rule in rules:
            o, s = first(ours, name), first(sovol, name)
            if o is None or s is None:
                fails.append(f"{name}: missing ({'ours' if o is None else 'sovol'})")
                continue
            if rule == "on":
                ok = o[0]
            elif rule.startswith("user:"):
                ok = o[0] and numbers(o[1]) == [float(rule[5:])]
            elif not o[0] or not s[0]:
                ok = o[0] == s[0]
            elif rule == "eq":
                no, ns = numbers(o[1]), numbers(s[1])
                ok = no == ns if no is not None and ns is not None else o[1].replace(" ", "") == s[1].replace(" ", "")
            else:
                no, ns = numbers(o[1]), numbers(s[1])
                if no is None or ns is None or len(no) != len(ns):
                    ok = False
                else:
                    ok = all(a <= b for a, b in zip(no, ns)) if rule == "le" else all(a >= b for a, b in zip(no, ns))
            status = "ok  " if ok else "FAIL"
            print(f"{status} {rule:2} {name:34} ours {o[1] if o[0] else '(off)':22} sovol {s[1] if s[0] else '(off)'}")
            if not ok:
                fails.append(name)
    return fails


def check_bin():
    fails = []
    ini = (ROOT / "firmware/ini/stm32f1-maple.ini").read_text(encoding="utf-8")
    section = ini.split(f"[env:{ENV}]", 1)[1].split("\n[", 1)[0]
    for needle in (f"board_build.address  = {APP_ADDRESS}", "board_build.ldscript = sovol.ld"):
        ok = needle in section
        print(f"{'ok  ' if ok else 'FAIL'} env {ENV}: {needle}")
        if not ok:
            fails.append(needle)
    bins = sorted((ROOT / "firmware/.pio/build" / ENV).glob("*.bin"), key=lambda p: p.stat().st_mtime)
    if not bins:
        return fails + ["no .bin found, build first"]
    b = bins[-1]
    size = b.stat().st_size
    ok = size <= MAX_BIN
    print(f"{'ok  ' if ok else 'FAIL'} size {b.name}: {size} bytes, limit {MAX_BIN}, margin {MAX_BIN - size}")
    if not ok:
        fails.append("size")
    return fails


def main():
    fails = check_config()
    if "--bin" in sys.argv:
        fails += check_bin()
    print("\nSAFETY CHECK " + ("PASSED" if not fails else f"FAILED: {', '.join(fails)}"))
    sys.exit(1 if fails else 0)


if __name__ == "__main__":
    main()
