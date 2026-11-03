#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
cd "$(dirname "$0")/.."
mkdir -p test-output
c++ -std=c++17 -O2 tests/make_historical_fixtures.cpp -o test-output/make-fixtures
test-output/make-fixtures test-output/fixture98
test-output/make-fixtures test-output/fixture98-noeffects 2 0
test-output/make-fixtures test-output/fixture98-guided 2 1 1
test-output/make-fixtures test-output/fixture98-snapshot 2 1 0 1
test-output/make-fixtures test-output/fixture98-v1 1
test-output/make-fixtures test-output/fixture98-v0 0
cc -std=c99 -D_POSIX_C_SOURCE=200809L -DD3_TESTING -O1 -g \
  -Wall -Wextra -Wno-misleading-indentation -Wno-format-truncation \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  src/d3core.c src/d3files.c src/sha256.c tests/test_core.c -o test-output/test-core
# LeakSanitizer needs access to /proc task metadata and can be unavailable in
# isolated build environments; address and undefined-behavior checks stay on.
ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  test-output/test-core "$(pwd)/test-output"
python3 tests/pe_audit.py D3EDIT98.EXE
