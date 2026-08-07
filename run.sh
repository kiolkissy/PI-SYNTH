#!/bin/sh
# Convenience launcher: builds the engine, starts it, and starts the web UI
# dev server. Ctrl+C stops both.
set -e

cd "$(dirname "$0")"

cmake --build build -j"$(nproc)"
sudo setcap 'cap_sys_nice=eip' build/synth

cleanup() {
    kill "$UI_PID" 2>/dev/null
}
trap cleanup EXIT

(cd ui && npm run dev) &
UI_PID=$!

./build/synth
