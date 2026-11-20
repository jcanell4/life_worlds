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

# LibTorch (C51 / DQN). Prefer an explicit CMAKE_PREFIX_PATH; otherwise use the
# common CPU install documented in the READMEs.
if [[ -z "${CMAKE_PREFIX_PATH:-}" ]]; then
  _torch_default="${HOME}/.local/libtorch-cpu/libtorch"
  if [[ -d "${_torch_default}" ]]; then
    export CMAKE_PREFIX_PATH="${_torch_default}"
  fi
elif [[ -d "${HOME}/.local/libtorch-cpu/libtorch" ]]; then
  export CMAKE_PREFIX_PATH="${HOME}/.local/libtorch-cpu/libtorch:${CMAKE_PREFIX_PATH}"
fi

cmake -S "$ROOT" -B "$BD" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build "$BD" --target life_worlds
ln -sf "$BD/compile_commands.json" compile_commands.json
