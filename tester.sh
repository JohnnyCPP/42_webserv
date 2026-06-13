#!/usr/bin/env bash

set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

HOST="127.0.0.1"
PORT="8080"        # main server   (config/default.conf, server #1)
PORT2="8081"       # second server (different content,    server #2)
VPORT="8082"       # virtual-host demo (config/vhost.conf, isolated)
DPORT="8083"       # duplicate-port demo (config/dupport.conf, isolated)
CONF="config/default.conf"
LOG_FILE="$ROOT/test.log"
SITE_FILES="$ROOT/www/web1/data/files"
TMP_UP="$(mktemp /tmp/webserv_up.XXXXXX)"
TMP_BIG="$(mktemp /tmp/webserv_big.XXXXXX)"
TMP_DL="$(mktemp /tmp/webserv_dl.XXXXXX)"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[0;33m'; NC='\033[0m'
TOTAL=0; PASS=0

echo "Webserv test report - $(date)" > "$LOG_FILE"

req() {
    local out
    out="$(curl -s --max-time 8 -w $'\n%{http_code}' "$@" 2>>"$LOG_FILE" | tr -d '\0')"
    REPLY_CODE="${out##*$'\n'}"
    REPLY_BODY="${out%$'\n'*}"
}

record() {  # name  ok(1/0)  detail
    TOTAL=$((TOTAL + 1))
    if [ "$2" = 1 ]; then
        PASS=$((PASS + 1))
        printf "${GREEN}\xe2\x9c\x93${NC} %s\n" "$1"
    else
        printf "${RED}\xe2\x9c\x97${NC} %s  ${RED}(%s)${NC}\n" "$1" "$3"
    fi
    { echo "[$([ "$2" = 1 ] && echo PASS || echo FAIL)] $1 ${3:+- $3}"; } >> "$LOG_FILE"
}

# t  desc  expected_code  body_substr(or "")  -- curl args...
t() {
    local desc="$1" exp="$2" sub="$3"; shift 3
    req "$@"
    local ok=1 why=""
    if [ "$REPLY_CODE" != "$exp" ]; then ok=0; why="code: want $exp got ${REPLY_CODE:-none}"; fi
    if [ -n "$sub" ] && ! printf '%s' "$REPLY_BODY" | grep -qF -- "$sub"; then
        ok=0; why="${why:+$why; }body missing '$sub'"
    fi
    record "$desc" "$ok" "$why"
}

# th  desc  expected_code  header_regex  -- curl args...   (checks a header line)
th() {
    local desc="$1" exp="$2" pat="$3"; shift 3
    local hdr code
    hdr="$(curl -s --max-time 8 -D - -o /dev/null "$@" 2>>"$LOG_FILE")"
    code="$(printf '%s' "$hdr" | head -1 | awk '{print $2}')"
    if [ "$code" = "$exp" ] && printf '%s' "$hdr" | grep -qiE "$pat"; then
        record "$desc" 1
    else
        record "$desc" 0 "want $exp + /$pat/, got code ${code:-none}"
    fi
}

# traw  desc  expected_code  raw_request   (malformed requests curl can't send)
traw() {
    local desc="$1" exp="$2" reqstr="$3"
    local code
    code="$(printf "$reqstr" | nc -w 3 "$HOST" "$PORT" 2>/dev/null | head -1 | awk '{print $2}')"
    [ "$code" = "$exp" ] && record "$desc" 1 || record "$desc" 0 "want $exp got ${code:-none}"
}

section() { printf "\n${YELLOW}===== %s =====${NC}\n" "$1"; }

AUX=""
boot_aux() {  # conf  port
    ./webserv "$1" >>"$LOG_FILE" 2>&1 &
    AUX=$!
    local _
    for _ in $(seq 1 20); do
        curl -s -o /dev/null --max-time 1 "http://$HOST:$2/" 2>/dev/null && break
        sleep 0.2
    done
    kill -0 "$AUX" 2>/dev/null
}
kill_aux() { [ -n "${AUX:-}" ] && kill "$AUX" 2>/dev/null; AUX=""; }

SRV=""
cleanup() {
    [ -n "$SRV" ] && kill "$SRV" 2>/dev/null
    kill_aux
    rm -f "$TMP_UP" "$TMP_BIG" "$TMP_DL"
}
trap cleanup EXIT

printf "${YELLOW}Building...${NC}\n"
if ! make -s >/dev/null 2>>"$LOG_FILE"; then
    printf "${RED}Build failed (see $LOG_FILE)${NC}\n"; exit 1
fi

printf "${YELLOW}Starting webserv on %s/%s with %s...${NC}\n" "$PORT" "$PORT2" "$CONF"
./webserv "$CONF" >>"$LOG_FILE" 2>&1 &
SRV=$!
for _ in $(seq 1 25); do
    curl -s -o /dev/null --max-time 1 "http://$HOST:$PORT/" && break
    sleep 0.2
done
if ! kill -0 "$SRV" 2>/dev/null; then
    printf "${RED}Server failed to start (port busy? check $LOG_FILE)${NC}\n"; exit 1
fi

section "A. Configuration"

t  "two servers on different ports: 8080 serves the site"   200 "WEBSERV" "http://$HOST:$PORT/"
t  "two servers on different ports: 8081 serves a listing"  200 "href="   "http://$HOST:$PORT2/"

t  "custom error page is served for 404"  404 "cannot be found" "http://$HOST:$PORT/notexist"

printf 'small body\n' > "$TMP_UP"
t  "POST body UNDER the limit is accepted (201)" 201 "" \
       -X POST -H "Content-Type: text/plain" --data-binary "@$TMP_UP" "http://$HOST:$PORT/upload/under.txt"
head -c 1500000 /dev/zero | tr '\0' 'A' > "$TMP_BIG"
t  "POST body OVER the limit -> 413"             413 "" \
       -X POST -H "Content-Type: text/plain" --data-binary "@$TMP_BIG" "http://$HOST:$PORT/upload/over.txt"

t  "route / maps to site root (index.html)"   200 "WEBSERV" "http://$HOST:$PORT/"
t  "route /data maps to the data directory"   200 "href="   "http://$HOST:$PORT/data/"

t  "directory request serves the default index file"  200 "WEBSERV" "http://$HOST:$PORT/"

t  "method allowed for the route works (DELETE 404 on missing, not 405)" 404 "" \
       -X DELETE "http://$HOST:$PORT/data/files/__nope.txt"
t  "method NOT in the route list -> 405"  405 "" "http://$HOST:$PORT/getnotallowed"

section "B. Basic checks"

t  "GET works (200)"                    200 "WEBSERV" "http://$HOST:$PORT/"
t  "POST works (upload -> 201)"         201 "" -X POST --data-binary "@$TMP_UP" "http://$HOST:$PORT/upload/post.txt"

printf 'delete me\n' > "$SITE_FILES/__deltest.txt"
t  "DELETE works on an existing file -> 204"  204 "" -X DELETE "http://$HOST:$PORT/data/files/__deltest.txt"
t  "the DELETEd file is now gone -> 404"      404 ""           "http://$HOST:$PORT/data/files/__deltest.txt"

t  "UNKNOWN method does not crash (PUT -> 405)" 405 "" -X PUT "http://$HOST:$PORT/"

UPNAME="roundtrip_$$.txt"
printf 'round-trip payload %s\n' "$$" > "$TMP_UP"
req -X POST --data-binary "@$TMP_UP" "http://$HOST:$PORT/upload/$UPNAME"
if [ "$REPLY_CODE" = "201" ]; then
    curl -s --max-time 8 -o "$TMP_DL" "http://$HOST:$PORT/data/files/$UPNAME" 2>>"$LOG_FILE"
    if cmp -s "$TMP_UP" "$TMP_DL"; then
        record "uploaded file can be downloaded again, identical" 1
    else
        record "uploaded file can be downloaded again, identical" 0 "downloaded bytes differ from upload"
    fi
else
    record "uploaded file can be downloaded again, identical" 0 "upload failed (code $REPLY_CODE)"
fi

section "C. Browser / static site"

t  "serves a full static page (index)"     200 "WEBSERV" "http://$HOST:$PORT/"
t  "serves a CSS asset"                     200 ""        "http://$HOST:$PORT/style.css"
t  "serves a binary asset (jpg)"            200 ""        "http://$HOST:$PORT/assets/42.jpg"
t  "serves the favicon"                     200 ""        "http://$HOST:$PORT/favicon.ico"

th "response carries a Content-Type header" 200 "^content-type:"   "http://$HOST:$PORT/"
th "response carries a Content-Length header" 200 "^content-length:" "http://$HOST:$PORT/"

t  "wrong URL -> 404"                        404 "" "http://$HOST:$PORT/this/does/not/exist"
t  "directory listing when autoindex on"     200 "href=" "http://$HOST:$PORT/data/"
t  "directory with autoindex OFF, no index -> 403" 403 "" "http://$HOST:$PORT/assets/"

th "redirect: /redir -> 301 Location: /data"             301 "^location:.*/data"        "http://$HOST:$PORT/redir"
th "redirect: /redirexample -> 301 Location: example.com" 301 "^location:.*example\.com" "http://$HOST:$PORT/redirexample"

B1="$(curl -s "http://$HOST:$PORT/")"; B2="$(curl -s "http://$HOST:$PORT2/")"
[ "$B1" != "$B2" ] && record "the two ports return different bodies" 1 \
                   || record "the two ports return different bodies" 0 "bodies identical"

if boot_aux config/vhost.conf "$VPORT"; then
    th "virtual host alpha.test -> the WEBSERV site" 200 "" \
       --resolve "alpha.test:$VPORT:127.0.0.1" "http://alpha.test:$VPORT/"
    t  "virtual host beta.test -> a directory listing" 200 "href=" \
       --resolve "beta.test:$VPORT:127.0.0.1" "http://beta.test:$VPORT/"
else
    record "virtual host alpha.test -> the WEBSERV site"  0 "vhost server did not start (needs listen-dedup + Host routing)"
    record "virtual host beta.test -> a directory listing" 0 "vhost server did not start (needs listen-dedup + Host routing)"
fi
kill_aux

section "D. Port issues"

./webserv config/dupport.conf >>"$LOG_FILE" 2>&1 &
DP=$!
sleep 0.6
if kill -0 "$DP" 2>/dev/null; then
    record "duplicate port is refused (server does not stay up)" 0 "server stayed running"
    kill "$DP" 2>/dev/null
else
    record "duplicate port is refused (server does not stay up)" 1
fi

section "E. CGI (.py / .php)"

t  "python CGI executes and exposes env" 200 "REQUEST_METHOD" "http://$HOST:$PORT/cgi/python/showenv.py"
t  "php CGI echoes the POST body"        200 "Content: Hello42" \
       -X POST --data "content=Hello42" "http://$HOST:$PORT/cgi/php/echo.php"
t  "CGI script that errors -> 500"       500 "" -X POST "http://$HOST:$PORT/cgi/python/error.py"
t  "CGI script not found -> 404"         404 ""         "http://$HOST:$PORT/cgi/python/__nope.py"
req --max-time 6 "http://$HOST:$PORT/cgi/python/inf.py"
t  "server still responds after a hanging CGI (no hang)" 200 "WEBSERV" "http://$HOST:$PORT/"

section "F. Robustness"

traw "malformed URI characters -> 400"    400 'GET /a>{} HTTP/1.1\r\nHost: localhost\r\n\r\n'
traw "URI not starting with '/' -> 400"   400 'GET index.html HTTP/1.1\r\nHost: localhost\r\n\r\n'
traw "garbage request line -> 400"        400 'GARBAGE NONSENSE\r\n\r\n'
printf 'GARBAGE\r\n\r\n' | nc -w 2 "$HOST" "$PORT" >/dev/null 2>&1
t   "server still serves after junk (no crash)" 200 "WEBSERV" "http://$HOST:$PORT/"

burst_ok=1
for _ in $(seq 1 40); do
    code="$(curl -s -o /dev/null -w '%{http_code}' --max-time 3 "http://$HOST:$PORT/")"
    [ "$code" = "200" ] || { burst_ok=0; break; }
done
record "40 sequential requests all answered (no hang)" "$burst_ok" "a request did not return 200"

printf "\n${YELLOW}Result: %d/%d passed${NC}\n" "$PASS" "$TOTAL"
if [ "$PASS" -eq "$TOTAL" ]; then
    printf "${GREEN}All checked requirements passed.${NC}\n"
else
    printf "${RED}%d failing — see %s.${NC}\n" \
        "$((TOTAL - PASS))" "$LOG_FILE"
fi
exit 0
