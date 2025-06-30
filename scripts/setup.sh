#!/usr/bin/env bash

set -euo pipefail

# Reavix Development Environment Setup
# Features:
# - Cross-platform support (LInux/macOs/WSL)
# - Sanitizers enabled (ASan,UBSan)
# - Linters installed
# - Debug symbols

REAVIX_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.."&&pwd)"
ZIG_VERSION="0.11.0"

echo "Setting up reavix development environment..."

# Install Zig
if ! command -v zig > /dev/null; then
    echo "Installing Zig... ${ZIG_VERSION}"
    case "$(uname -s)" in
        Linux*)
            curl -L "https://ziglang.org/builds/zig-linux-x86_64-${ZIG_VERSION}.tar.xz" | tar -xJ -C /tmp
            export PATH="/tmp/zig-linux-x86_64-${ZIG_VERSION}:$PATH"
            ;;
        Darwin*)
            curl -L "https://ziglang.org/builds/zig-macos-x86_64-${ZIG_VERSION}.tar.xz" | tar -xJ -C /tmp
            export PATH="/tmp/zig-macos-x86_64-${ZIG_VERSION}:$PATH"
            ;;
        *)
            echo "Unsupported OS"
            exit 1
            ;;
    esac
fi

#Install clang-tidy
if ! commanf -v clang-tidy > /dev/null; then
    if[["$OSTYPE" == "linux-gnu"*]]; then
        sudo apt-get install -y clang-tidy-15
    elif[["$OSTYPE" == "darwin"*]]; then
        brew install llvm
    fi
fi

#compile with sanitizers
echo "Building core with sanitizers"
zig cc \
    -Wall -Wextra -Werror \
    -fsanitize=address,undefined \
    -fno-omit-frame-pointer \
    -g \
    -Isrc/include \
    src/core/*.c src/sandbox/*.c \
    -o "$REAVIX_ROOT/bin/reavix-core"

echo "Setup complete! Run './bin/reavix-core to start"