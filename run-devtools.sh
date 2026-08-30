#!/usr/bin/env sh
set -eu

ROOT=$(cd "$(dirname "$0")" && pwd)
ACTION="${1:-}"
BUILD_DIR="${2:-$ROOT/cmake-build-debug}"

if [ -z "$ACTION" ]; then
  echo "Usage: run-devtools.sh <format|format-fix|tidy|tidy-fix> [build_dir]" >&2
  exit 1
fi

case "$ACTION" in
  format|format-fix|tidy|tidy-fix)
    ;;
  *)
    echo "Usage: run-devtools.sh <format|format-fix|tidy|tidy-fix> [build_dir]" >&2
    exit 1
    ;;
esac

cmake --build "$BUILD_DIR" --target "$ACTION"

