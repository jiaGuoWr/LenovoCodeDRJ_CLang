#!/usr/bin/env bash
#
# Cloud Agent install script for the Lenovo Tidy monorepo.
#
# Prepares a Linux (Ubuntu 24.04) development environment for the three
# components that build natively here:
#   * LenovoTidyChecks  - C++ clang-tidy plugin + driver (CMake/Ninja + LLVM 18)
#   * LenovoTidyLsp      - Rust tower-lsp server (cargo)
#   * LenovoTidyVscode   - TypeScript VS Code extension (npm)
#
# LenovoTidyVs2022 (C#/.NET Framework VSIX) and windows-build/*.ps1 are
# Windows-only and are intentionally not built here.
#
# The script is idempotent: re-running it refreshes dependencies and performs
# incremental builds without side effects.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

# LLVM 18 tools (FileCheck) and pip's --user bin dir are not on the default
# PATH; add them so CMake can discover lit/FileCheck for the regression suite.
export PATH="$HOME/.local/bin:/usr/lib/llvm-18/bin:$PATH"

echo "==> Installing system packages (LLVM 18 dev toolchain + build tools)"
export DEBIAN_FRONTEND=noninteractive
sudo apt-get update
# Ubuntu 24.04 (noble) ships LLVM/Clang 18 (matching the required major
# version) directly in its universe/main repos, so no external apt.llvm.org
# source is needed. libstdc++-14-dev + libzstd-dev satisfy clang++'s default
# GCC toolchain selection and LLVM's exported zstd link target respectively.
sudo apt-get install -y --no-install-recommends \
  build-essential ninja-build \
  libstdc++-14-dev libzstd-dev \
  llvm-18-dev libclang-18-dev libclang-cpp18-dev \
  clang-18 clang-tools-18 clang-tidy-18 clang-format-18 \
  python3 python3-pip

echo "==> Installing lit (LLVM regression-test runner)"
pip3 install --user --break-system-packages lit

echo "==> Ensuring a stable Rust toolchain (Cargo.lock needs edition2024 support)"
if command -v rustup >/dev/null 2>&1; then
  rustup toolchain install stable --profile minimal >/dev/null
  rustup default stable
fi

echo "==> Building LenovoTidyLsp (Rust, release)"
cargo build --release --manifest-path "$REPO_ROOT/LenovoTidyLsp/Cargo.toml"

echo "==> Installing + compiling LenovoTidyVscode (TypeScript)"
(
  cd "$REPO_ROOT/LenovoTidyVscode"
  npm ci
  npm run compile
)

echo "==> Configuring + building LenovoTidyChecks (C++ clang-tidy plugin + driver)"
(
  cd "$REPO_ROOT/LenovoTidyChecks"
  cmake --preset linux-release
  cmake --build --preset linux-release -j
)

echo "==> install.sh complete"
