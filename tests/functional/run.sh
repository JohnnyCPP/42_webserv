#!/usr/bin/env bash
#
# Functional (black-box) test suite for webserv.
#
# It builds the server, launches it with tests/functional/test.conf, and drives
# it over real TCP with curl / raw sockets, asserting on the actual responses.
# This mirrors how the 42 evaluation grades the project.
#
# Tests are split in two groups:
#
#   ACTIVE   - things that already work today (reception layer + crash safety).
#              These determine the exit code: a single failure fails the suite.
#
#   PENDING  - eval behaviours that require Phase 3 routing
#              (WebServer::processClientRequest). They are run for visibility
#              but NEVER affect the exit code. As Phase 3 lands, promote each
#              one from pending() to a hard assertion.
#
# Usage:  ./tests/functional/run.sh
# Run from the repository root, or from anywhere (it cd's to the repo root).

set -u

# ---------------------------------------------------------------------------
# Locate repo root (this file lives in tests/functional/)
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${ROOT_DIR}"

CONF="tests/functional/test.conf"
HOST="127.0.0.1"
PORT_A=18080      # must match test.conf server 1
PORT_B=19090      # must match test.conf server 2
BIN="./webserv"
LOG="$(mktemp /tmp/webserv_test.XXXXXX.log)"
SERVER_PID=""

# ---------------------------------------------------------------------------
# Pretty output / counters
# ---------------------------------------------------------------------------
if [ -t 1 ]; then
    GREEN='\033[0;32m'; RED='\033[0;31m'; YELLOW='\033[0;33m'
    CYAN='\033[0;36m'; BOLD='\033[1m'; RESET='\033[0m'
else
    GREEN=''; RED=''; YELLOW=''; CYAN=''; BOLD=''; RESET=''
fi

PASS=0; FAIL=0; PEND=0
FAILED_NAMES=()

pass()    { PASS=$((PASS+1)); printf "  ${GREEN}PASS${RESET}  %s\n" "$1"; }
fail()    { FAIL=$((FAIL+1)); FAILED_NAMES+=("$1"); printf "  ${RED}FAIL${RESET}  %s\n" "$1"; [ $# -gt 1 ] && printf "        ${RED}%s${RESET}\n" "$2"; }
pending() { PEND=$((PEND+1)); printf "  ${YELLOW}PEND${RESET}  %s\n" "$1"; [ $# -gt 1 ] && printf "        ${YELLOW}%s${RESET}\n" "$2"; }
section() { printf "\n${BOLD}${CYAN}== %s ==${RESET}\n" "$1"; }

# assert_eq <expected> <actual> <name>
assert_eq() {
    if [ "$1" = "$2" ]; then pass "$3"; else fail "$3" "expected '$1', got '$2'"; fi
}
# assert_contains <haystack> <needle> <name>
assert_contains() {
    case "$1" in
        *"$2"*) pass "$3" ;;
        *)      fail "$3" "expected to contain '$2'" ;;
    esac
}

# ---------------------------------------------------------------------------
# HTTP helpers
# ---------------------------------------------------------------------------
# http_status <method> <url> [curl-extra-args...]
http_status() {
    local method="$1"; local url="$2"; shift 2
    curl -s -o /dev/null -w "%{http_code}" --max-time 5 -X "$method" "$@" "$url" 2>/dev/null
}
# http_headers <method> <url> [extra...]
http_headers() { local m="$1" u="$2"; shift 2; curl -s -D - -o /dev/null --max-time 5 -X "$m" "$@" "$u" 2>/dev/null; }
# http_body <url> [extra...]
http_body()    { local u="$1"; shift; curl -s --max-time 5 "$@" "$u" 2>/dev/null; }

# raw_send <port> <data>  -- send bytes over a raw socket, print whatever comes back
raw_send() {
    local port="$1" data="$2"
    exec 3<>"/dev/tcp/${HOST}/${port}" 2>/dev/null || return 1
    printf '%b' "$data" >&3
    timeout 2 cat <&3
    exec 3>&- 3<&- 2>/dev/null
}

port_open() { (exec 3<>"/dev/tcp/${HOST}/$1") 2>/dev/null && { exec 3>&- 3<&-; return 0; } || return 1; }

wait_for_port() {
    local port="$1" tries=50
    while [ $tries -gt 0 ]; do
        port_open "$port" && return 0
        sleep 0.1; tries=$((tries-1))
    done
    return 1
}

server_alive() { [ -n "$SERVER_PID" ] && kill -0 "$SERVER_PID" 2>/dev/null; }

start_server() {
    "$BIN" "$CONF" >"$LOG" 2>&1 &
    SERVER_PID=$!
}

stop_server() {
    [ -n "$SERVER_PID" ] && kill "$SERVER_PID" 2>/dev/null
    [ -n "$SERVER_PID" ] && wait "$SERVER_PID" 2>/dev/null
    SERVER_PID=""
}

cleanup() {
    stop_server
    rm -f "$LOG"
}
trap cleanup EXIT INT TERM

# ===========================================================================
# 0. BUILD
# ===========================================================================
section "Build"
if make re >/tmp/webserv_build.log 2>&1 && [ -x "$BIN" ]; then
    pass "make re builds $BIN with -Wall -Wextra -Werror -std=c++98"
else
    fail "build failed (see /tmp/webserv_build.log)"
    printf "\n${RED}Cannot continue without a binary.${RESET}\n"
    exit 1
fi

# Guard: make sure our test ports are free before we start.
if port_open "$PORT_A" || port_open "$PORT_B"; then
    printf "${RED}Test ports %s/%s already in use. Aborting.${RESET}\n" "$PORT_A" "$PORT_B"
    exit 1
fi

# ===========================================================================
# 1. STARTUP / CONFIG
# ===========================================================================
section "Startup & configuration"
start_server
if wait_for_port "$PORT_A"; then
    pass "server starts and listens on $PORT_A (server block 1)"
else
    fail "server did not start listening on $PORT_A"
    cat "$LOG"
    exit 1
fi
if port_open "$PORT_B"; then
    pass "server listens on $PORT_B (server block 2 / second port)"
else
    fail "server not listening on $PORT_B"
fi
server_alive && pass "process stays alive after startup" || fail "process died after startup"

# ===========================================================================
# 2. BASIC METHODS  (eval: GET/POST/DELETE work, UNKNOWN must not crash)
# ===========================================================================
section "Basic methods (no-crash + valid response)"

st=$(http_status GET "http://${HOST}:${PORT_A}/")
[ -n "$st" ] && pass "GET / returns an HTTP status ($st)" || fail "GET / returned nothing (timeout/hang)"

st=$(http_status POST "http://${HOST}:${PORT_A}/upload" --data "hello=world")
[ -n "$st" ] && pass "POST /upload returns an HTTP status ($st)" || fail "POST returned nothing"

st=$(http_status DELETE "http://${HOST}:${PORT_A}/api/data.json")
[ -n "$st" ] && pass "DELETE /api returns an HTTP status ($st)" || fail "DELETE returned nothing"

# Unknown method must not crash the server.
http_status BREW "http://${HOST}:${PORT_A}/" >/dev/null
server_alive && pass "UNKNOWN method (BREW) does not crash the server" || fail "server crashed on unknown method"

# ===========================================================================
# 3. RESPONSE WELL-FORMEDNESS
# ===========================================================================
section "Response format"
hdrs=$(http_headers GET "http://${HOST}:${PORT_A}/")
assert_contains "$hdrs" "HTTP/1.1" "response has an HTTP/1.1 status line"
assert_contains "$hdrs" "Content-Length:" "response carries a Content-Length header"

# Body length must equal advertised Content-Length.
clen=$(printf '%s' "$hdrs" | tr -d '\r' | awk -F': ' 'tolower($1)=="content-length"{print $2}')
blen=$(http_body "http://${HOST}:${PORT_A}/" | wc -c | tr -d ' ')
if [ -n "$clen" ]; then
    assert_eq "$clen" "$blen" "Content-Length ($clen) matches body byte count ($blen)"
else
    fail "no Content-Length to compare against body"
fi

# ===========================================================================
# 4. CRASH / RESILIENCE  (malformed + partial input must not take it down)
# ===========================================================================
section "Crash resilience"

raw_send "$PORT_A" "GARBAGE WITHOUT CRLF" >/dev/null 2>&1
server_alive && pass "garbage with no CRLF does not crash the server" || fail "server crashed on garbage input"

raw_send "$PORT_A" "GET / HTTP/1.1\r\nHost: x\r\n" >/dev/null 2>&1   # headers never terminated
server_alive && pass "incomplete request (no blank line) does not crash" || fail "server crashed on partial request"

# open a connection and close it immediately without sending anything
(exec 3<>"/dev/tcp/${HOST}/${PORT_A}"; exec 3>&- 3<&-) 2>/dev/null
server_alive && pass "connect-then-close with no data does not crash" || fail "server crashed on empty connection"

# server must still serve normal traffic afterwards
st=$(http_status GET "http://${HOST}:${PORT_A}/")
[ -n "$st" ] && pass "server still responds after malformed traffic ($st)" || fail "server stopped responding after malformed traffic"

# ===========================================================================
# 5. LOAD / NO HANGING CONNECTIONS  (light, scriptable proxy for siege)
# ===========================================================================
section "Sequential load (no hang)"
ok=0
for i in $(seq 1 100); do
    s=$(http_status GET "http://${HOST}:${PORT_A}/")
    [ -n "$s" ] && ok=$((ok+1))
done
assert_eq "100" "$ok" "100 sequential GET requests all answered (no hanging connection)"
server_alive && pass "process still alive after load" || fail "process died under load"

# A handful of parallel clients should all get served.
pids=""; tmpd=$(mktemp -d)
for i in $(seq 1 10); do
    ( http_status GET "http://${HOST}:${PORT_A}/" > "$tmpd/$i" ) &
    pids="$pids $!"
done
wait $pids 2>/dev/null
par_ok=$(grep -l . "$tmpd"/* 2>/dev/null | wc -l | tr -d ' ')
assert_eq "10" "$par_ok" "10 parallel GET requests all answered"
rm -rf "$tmpd"

# ===========================================================================
# 6. CLI / ARG HANDLING
# ===========================================================================
section "CLI argument handling"
stop_server
"$BIN" too many args >/dev/null 2>&1
rc=$?
assert_eq "1" "$rc" "too many arguments exits with failure (no crash)"

# ===========================================================================
# 7. PORT REUSE  (eval: same port twice must not work)
# ===========================================================================
section "Port reuse"
start_server
wait_for_port "$PORT_A" || true
"$BIN" "$CONF" >/tmp/webserv_dup.log 2>&1 &
dup_pid=$!
sleep 1
if kill -0 "$dup_pid" 2>/dev/null; then
    fail "a second instance on the same ports kept running (should fail to bind)"
    kill "$dup_pid" 2>/dev/null; wait "$dup_pid" 2>/dev/null
else
    if grep -qi "bind" /tmp/webserv_dup.log; then
        pass "second instance on the same ports exits cleanly with a bind error"
    else
        pass "second instance on the same ports exits (no crash)"
    fi
fi
rm -f /tmp/webserv_dup.log

# ===========================================================================
# 8. PENDING  -- require Phase 3 routing. Informational only.
# ===========================================================================
section "PENDING (require Phase 3: WebServer::processClientRequest)"
# These run against the live server but never fail the suite. Promote each to a
# hard assertion (assert_eq / assert_contains) as the routing is implemented.

st=$(http_status GET "http://${HOST}:${PORT_A}/index.html")
body=$(http_body "http://${HOST}:${PORT_A}/index.html")
case "$body" in
    *"Hello from webserv"*) pending "static file serving: GET /index.html should return the file, not the stub page";;
    *"Webserv Demo"*)       pass    "static file serving: GET /index.html returns the real file" ;;
    *)                      pending "static file serving: GET /index.html (status $st)" ;;
esac

st=$(http_status GET "http://${HOST}:${PORT_A}/does-not-exist")
[ "$st" = "404" ] && pass "wrong URL returns 404" || pending "wrong URL should return 404 (got $st)"

st=$(http_status DELETE "http://${HOST}:${PORT_A}/")    # only GET allowed on /
[ "$st" = "405" ] && pass "disallowed method returns 405" || pending "DELETE on GET-only route should return 405 (got $st)"

allow=$(http_headers DELETE "http://${HOST}:${PORT_A}/" | tr -d '\r' | awk -F': ' 'tolower($1)=="allow"{print $2}')
[ -n "$allow" ] && pass "405 carries an Allow header ($allow)" || pending "405 should carry an Allow header listing permitted methods"

big=$(head -c 2000000 /dev/zero | tr '\0' 'a')         # 2 MB > 1 MB limit
st=$(http_status POST "http://${HOST}:${PORT_A}/upload" --data-binary "$big")
[ "$st" = "413" ] && pass "body over client_max_body_size returns 413" || pending "oversized body should return 413 (got $st)"

st=$(http_status GET "http://${HOST}:${PORT_A}/old-stuff")
[ "$st" = "301" ] && pass "configured route returns 301 redirect" || pending "/old-stuff should return 301 (got $st)"
loc=$(http_headers GET "http://${HOST}:${PORT_A}/old-stuff" | tr -d '\r' | awk -F': ' 'tolower($1)=="location"{print $2}')
[ -n "$loc" ] && pass "redirect carries a Location header ($loc)" || pending "301 should carry a Location header"

body=$(http_body "http://${HOST}:${PORT_A}/public/")
case "$body" in
    *"<a href"*|*"Index of"*) pass "autoindex lists directory contents" ;;
    *) pending "autoindex on /public/ should list directory contents" ;;
esac

# upload then retrieve
token="probe_$$_$RANDOM"
http_status POST "http://${HOST}:${PORT_A}/upload" -H "Content-Type: text/plain" --data "$token" >/dev/null
back=$(http_body "http://${HOST}:${PORT_A}/upload/")
case "$back" in
    *"$token"*) pass "uploaded file can be retrieved back" ;;
    *) pending "upload + retrieve round-trip (upload a file and GET it back)" ;;
esac

# custom error page body (config maps 404 -> /errors/404.html)
body=$(http_body "http://${HOST}:${PORT_A}/definitely-missing")
case "$body" in
    *"<html"*) pending "custom error page: 404 should serve the configured /errors/404.html once it exists" ;;
    *) pending "custom error page wiring" ;;
esac

# virtual host routing by server_name
st=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 \
        --resolve "static.webserv:${PORT_B}:127.0.0.1" \
        "http://static.webserv:${PORT_B}/" 2>/dev/null)
[ -n "$st" ] && pending "virtual-host routing by server_name (got $st; verify it serves the static.webserv root)"

# ===========================================================================
# SUMMARY
# ===========================================================================
stop_server
printf "\n${BOLD}==================== SUMMARY ====================${RESET}\n"
printf "  ${GREEN}PASS:    %d${RESET}\n" "$PASS"
printf "  ${RED}FAIL:    %d${RESET}\n" "$FAIL"
printf "  ${YELLOW}PENDING: %d${RESET} (Phase 3 — see tests/FUTURE_TESTS.md)\n" "$PEND"
if [ "$FAIL" -gt 0 ]; then
    printf "\n${RED}Failed:${RESET}\n"
    for n in "${FAILED_NAMES[@]}"; do printf "    - %s\n" "$n"; done
    exit 1
fi
printf "\n${GREEN}All active tests passed.${RESET}\n"
exit 0
