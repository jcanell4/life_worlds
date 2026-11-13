#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

# Host-specific tree so a rebuild on one machine does not wipe the other
# (macOS and Linux each keep their own CMake cache).
case "$(uname -s)" in
  Darwin) BD=build/darwin ;;
  Linux) BD=build/linux ;;
  *) BD="build/$(uname -s | tr '[:upper:]' '[:lower:]')" ;;
esac

rm -rf "$BD"

cmake -S "$ROOT" -B "$BD" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build "$BD" --target life_worlds
ln -sf "$BD/compile_commands.json" compile_commands.json
