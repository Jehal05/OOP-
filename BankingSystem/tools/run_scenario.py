#!/usr/bin/env python3
"""Run the bank binary with a scripted stdin and return the screens it printed.

Scenario file format: one input per line, '# comment' allowed, <enter> = press Enter,
{DATE+N} = date N days from today (YYYY-MM-DD).
"""
import datetime, os, re, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

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

def run(scenario, workdir=None, color=True, extra_args=()):
    """Returns (screens, stdout, workdir). Screens are split on form-feed markers."""
    inputs = load_inputs(scenario)
    workdir = workdir or tempfile.mkdtemp(prefix="novabank_scn_")
    env = dict(os.environ, BANK_SCREEN_MARK="1", BANK_DATA_DIR="data")   # relative: receipts/ and statements/ land next to data/
    if color:
        env["FORCE_COLOR"] = "1"
    proc = subprocess.run([os.path.join(ROOT, "bank"), *extra_args], input="\n".join(inputs) + "\n", capture_output=True,
                          text=True, env=env, cwd=workdir, timeout=60)
    screens = [s for s in proc.stdout.split("\f")]
    return screens, proc.stdout, workdir

if __name__ == "__main__":
    screens, out, wd = run(sys.argv[1], color=False)
    ansi = re.compile(r"\x1b\[[0-9;]*m")
    for i, s in enumerate(screens):
        print(f"\n######## SCREEN {i} ########")
        print(ansi.sub("", s).rstrip())
    shutil.rmtree(wd, ignore_errors=True)
