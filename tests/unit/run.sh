#!/usr/bin/env bash
#
# Compiles and runs the C++ unit tests against the real source files, using the
# same standard/flags as the project Makefile.
#
# Usage:  ./tests/unit/run.sh

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${ROOT_DIR}"

CXX="${CXX:-c++}"
CXXFLAGS="-Wall -Wextra -Werror -std=c++98 -g3"
INC="-I include -I tests/unit"
BIN_DIR="$(mktemp -d /tmp/webserv_unit.XXXXXX)"
trap 'rm -rf "$BIN_DIR"' EXIT

rc=0

build_and_run() {
    local name="$1"; shift          # remaining args: source files
    local out="${BIN_DIR}/${name}"
    printf "\n\033[0;36mCompiling\033[0m %s\n" "$name"
    if ! $CXX $CXXFLAGS $INC "$@" -o "$out"; then
        printf "\033[0;31mCompilation failed: %s\033[0m\n" "$name"
        rc=1
        return
    fi
    "$out" || rc=1
}

build_and_run http_response \
    tests/unit/test_http_response.cpp \
    src/http/HttpResponse.cpp

build_and_run client_parser \
    tests/unit/test_client_parser.cpp \
    src/client/Client.cpp

if [ "$rc" -eq 0 ]; then
    printf "\n\033[0;32mAll unit tests passed.\033[0m\n"
else
    printf "\n\033[0;31mSome unit tests failed.\033[0m\n"
fi
exit $rc
