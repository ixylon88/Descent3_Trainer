#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
cd "$(dirname "$0")/.."
mkdir -p test-output/de
c++ -std=c++17 -O2 -DABI32 tests/make_fixtures.cpp -o test-output/make32
c++ -std=c++17 -O2 tests/make_fixtures.cpp -o test-output/make64
test-output/make32 test-output/fixture32
test-output/make64 test-output/fixture64
test-output/make32 test-output/fixture32-noeffects 2 0
test-output/make64 test-output/fixture64-guided 2 1 1
test-output/make32 test-output/fixture32-snapshot 2 1 0 1
test-output/make32 test-output/fixture32-v1 1
test-output/make32 test-output/fixture32-v0 0
test-output/make32 test-output/fixture32-loadout 2 1 1 1 1
test-output/make64 test-output/fixture64-loadout 2 1 1 1 1
test-output/make32 test-output/fixture32-guidebot 2 1 0 0 0 1
test-output/make64 test-output/fixture64-guidebot 2 1 0 0 0 1
# Build the same resource names that ResourceManager uses in the Windows editor.
resources=D3SaveEditor.Core.Resources.Messages
resgen D3SaveEditor.Core/Resources/Messages.resx "test-output/$resources.resources"
resgen D3SaveEditor.Core/Resources/Messages.de.resx "test-output/de/$resources.de.resources"
mcs -codepage:65001 -target:exe -out:test-output/Tests.exe \
  "-resource:test-output/$resources.resources" D3SaveEditor.Core/*.cs tests/TestRunner.cs
al -target:lib -culture:de "-embed:test-output/de/$resources.de.resources" \
  -template:test-output/Tests.exe -out:test-output/de/Tests.resources.dll
mono test-output/Tests.exe "$(pwd)/test-output"
