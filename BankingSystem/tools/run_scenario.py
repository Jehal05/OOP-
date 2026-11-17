#!/usr/bin/env python3
"""Reads a scenario file (the typed inputs of a demo session).

Format: one input per line, '# comment' allowed, <enter> = press Enter,
{DATE+N} = date N days from today (YYYY-MM-DD).
Used by tools/valgrind_all.sh to feed the real program.
"""
import datetime, re

def load_inputs(path):
    out = []
    today = datetime.date.today()
    for raw in open(path, encoding="utf-8"):
        line = raw.rstrip("\n")
        if " #" in line:
            line = line[: line.index(" #")]
        if line.strip().startswith("#") or not line.strip():
            continue
        line = line.strip()
        line = re.sub(r"\{DATE\+(\d+)\}", lambda m: (today + datetime.timedelta(days=int(m.group(1)))).isoformat(), line)
        out.append("" if line == "<enter>" else line)
    return out
