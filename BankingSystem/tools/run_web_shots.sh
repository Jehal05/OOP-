#!/usr/bin/env bash
# Starts a throw-away server with fresh demo data, replays the browser tour, stops the server.
set -e
cd "$(dirname "$0")/.."
D=$(mktemp -d)
BANK_DATA_DIR="$D" ./bankweb --port 8099 >/dev/null 2>&1 &
PID=$!
trap 'kill $PID 2>/dev/null; rm -rf "$D"' EXIT
sleep 1.2
node tools/web_shots.js http://localhost:8099
