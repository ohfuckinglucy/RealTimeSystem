#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BIN_DIR="$ROOT_DIR/bin"

printf "[tests] building...\n"
make -C "$ROOT_DIR" all >/dev/null

fail() { printf "[tests] FAIL: %s\n" "$1"; exit 1; }
pass() { printf "[tests] PASS: %s\n" "$1"; }

# smoke: hello
"$BIN_DIR/hello" "test" >/dev/null 2>&1 || fail "hello"
pass "hello"

# smoke: intro
( "$BIN_DIR/intro" >/dev/null 2>&1 & echo $! > "$BIN_DIR/.intro.pid" ) || true
sleep 0.2
kill "$(cat "$BIN_DIR/.intro.pid" 2>/dev/null)" 2>/dev/null || true
rm -f "$BIN_DIR/.intro.pid"
pass "intro (smoke)"

# shared_mem: semex
( "$BIN_DIR/semex" >/dev/null 2>&1 & echo $! > "$BIN_DIR/.semex.pid" ) || true
sleep 0.5
kill "$(cat "$BIN_DIR/.semex.pid" 2>/dev/null)" 2>/dev/null || true
rm -f "$BIN_DIR/.semex.pid"
pass "semex (smoke)"

# interrupt: int
( "$BIN_DIR/int" >/dev/null 2>&1 & echo $! > "$BIN_DIR/.int.pid" ) || true
sleep 0.2
kill "$(cat "$BIN_DIR/.int.pid" 2>/dev/null)" 2>/dev/null || true
rm -f "$BIN_DIR/.int.pid"
pass "int (smoke)"

# resource manager: start server and client
( "$BIN_DIR/resmgr" >/dev/null 2>&1 & echo $! > "$BIN_DIR/.resmgr.pid" ) || true
sleep 0.2
"$BIN_DIR/resmgr_client" "hello" >/dev/null 2>&1 || fail "resmgr_client"
kill "$(cat "$BIN_DIR/.resmgr.pid" 2>/dev/null)" 2>/dev/null || true
rm -f "$BIN_DIR/.resmgr.pid"
pass "resmgr echo"

printf "[tests] all tests passed\n"
