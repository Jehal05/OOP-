#!/usr/bin/env python3
"""Fail if any rendered screen line is wider than 80 columns (the UI promises 80-column readability)."""
import re, sys, os, glob
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run_scenario
ansi = re.compile(r"\x1b\[[0-9;]*m")
bad = 0
for scn in sorted(glob.glob(os.path.join(run_scenario.ROOT, "scenarios", "*.txt"))):
    screens, out, wd = run_scenario.run(scn, color=False)
    for i, s in enumerate(screens):
        for line in ansi.sub("", s).split("\n"):
            # prompt lines echo user input and can legitimately be long; only layout lines matter
            if line.startswith(" ▸") or line.startswith(" >"):
                continue
            if len(line) > 80:
                bad += 1
                print(f"{os.path.basename(scn)} screen {i}: {len(line)} cols: {line[:90]}")
print("width check:", "OK" if bad == 0 else f"{bad} wide line(s)")
sys.exit(1 if bad else 0)
