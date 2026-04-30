#!/usr/bin/env bash
# Convenience entry point: configure, build, run ctest.

set -euo pipefail

PRESET="${1:-linux-release}"

cmake --preset "$PRESET"
cmake --build --preset "$PRESET" -j
ctest --preset "$PRESET" --output-on-failure
