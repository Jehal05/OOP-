#!/usr/bin/env bash
# Runs every scripted scenario and the integration tests under Valgrind; fails on any leak or memory error.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
make -s all >/dev/null
make -s build/test_bank build/test_repository >/dev/null
for scn in scenarios/*.txt; do
  work="$(mktemp -d)"
  python3 - "$scn" > "$work/input.txt" <<'PY'
import sys, os
sys.path.insert(0, "tools")
import run_scenario
print("\n".join(run_scenario.load_inputs(sys.argv[1])))
PY
  echo "valgrind: $scn"
  ( cd "$work" && BANK_DATA_DIR=data valgrind --leak-check=full --errors-for-leak-kinds=all --error-exitcode=9 -q \
      "$ROOT/bank" --no-color < input.txt > /dev/null )
  rm -rf "$work"
done
for t in build/test_bank build/test_repository build/test_account; do
  echo "valgrind: $t"; valgrind --leak-check=full --errors-for-leak-kinds=all --error-exitcode=9 -q ./$t > /dev/null
done
echo "VALGRIND CLEAN: no leaks, no memory errors"
