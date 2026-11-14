#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
exec sh "$(dirname "$0")/windows98/build.sh"
