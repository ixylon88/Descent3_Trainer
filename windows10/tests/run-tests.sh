#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
cd "$(dirname "$0")/.."
mkdir -p test-output
c++ -std=c++17 -O2 -DABI32 tests/make_fixtures.cpp -o test-output/make32
c++ -std=c++17 -O2 tests/make_fixtures.cpp -o test-output/make64
test-output/make32 test-output/fixture32
test-output/make64 test-output/fixture64
test-output/make32 test-output/fixture32-noeffects 2 0
test-output/make64 test-output/fixture64-guided 2 1 1
test-output/make32 test-output/fixture32-snapshot 2 1 0 1
test-output/make32 test-output/fixture32-v1 1
test-output/make32 test-output/fixture32-v0 0
mcs -target:exe -out:test-output/Tests.exe src/SaveGame.cs src/ShipLimits.cs tests/TestRunner.cs
mono test-output/Tests.exe "$(pwd)/test-output"
# Optional with a running X11 server (e.g. xvfb-run):
# mcs -target:exe -main:UiTests -out:test-output/UiTests.exe \
#   -r:System.Windows.Forms -r:System.Drawing src/*.cs tests/UiTests.cs
# mono test-output/UiTests.exe "$(pwd)/test-output/fixture32"
