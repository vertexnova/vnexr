#!/bin/bash

#==============================================================================
# VneXR WebAssembly / Emscripten Build Script
#==============================================================================

set -e
trap 'echo "ERROR: command failed at line $LINENO"; exit 1' ERR

PORT=8080
JOBS=10
ARGS=()

for arg in "$@"; do
  if [ "$arg" = "--kill" ]; then
    pids=$(lsof -ti tcp:$PORT 2>/dev/null || true)
    [ -n "$pids" ] && kill $pids
    exit 0
  fi
done

while [[ $# -gt 0 ]]; do
  case $1 in
    -j|--jobs) [[ -n "$2" && "$2" =~ ^[0-9]+$ ]] && { JOBS="$2"; shift 2; } || exit 1 ;;
    -j*) JOBS="${1#-j}"; shift ;;
    *) ARGS+=("$1"); shift ;;
  esac
done
set -- "${ARGS[@]}"

usage() {
  echo "Usage: $0 [-t <build_type>] [-a configure|build|configure_and_build|test|serve] [-l static|shared] [-clean] [-j N] [--with-examples|--no-examples] [--with-tests|--no-tests] [--serve] [--kill]"
  exit 1
}

BUILD_TYPE="Debug"
ACTION="configure_and_build"
LIB_TYPE="static"
CLEAN=false
WITH_EXAMPLES=true
WITH_TESTS=false
SERVE=false

while [[ $# -gt 0 ]]; do
  case $1 in
    -t|--build-type) BUILD_TYPE="$2"; shift 2 ;;
    -a|--action) ACTION="$2"; shift 2 ;;
    -l|--lib-type) LIB_TYPE="$2"; shift 2 ;;
    -clean|--clean) CLEAN=true; shift ;;
    --with-examples|--with-samples) WITH_EXAMPLES=true; shift ;;
    --no-examples|--no-samples) WITH_EXAMPLES=false; shift ;;
    --with-tests) WITH_TESTS=true; shift ;;
    --no-tests) WITH_TESTS=false; shift ;;
    --serve) SERVE=true; shift ;;
    -h|--help) usage ;;
    *) echo "Unknown option: $1"; usage ;;
  esac
done

command -v emcc >/dev/null || { echo "ERROR: emcc not found. source emsdk/emsdk_env.sh"; exit 1; }
command -v emcmake >/dev/null || { echo "ERROR: emcmake not found"; exit 1; }

PROJECT_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR="$PROJECT_ROOT/build/wasm/${LIB_TYPE}/${BUILD_TYPE}"
SERVE_DIR="$BUILD_DIR/bin"

CMAKE_ARGS=(
  "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
  "-DVNE_XR_LIB_TYPE=$LIB_TYPE"
  "-DVNE_XR_DEV=OFF"
  "-DVNE_XR_TESTS=$( [ "$WITH_TESTS" = true ] && echo ON || echo OFF )"
  "-DVNE_XR_EXAMPLES=$( [ "$WITH_EXAMPLES" = true ] && echo ON || echo OFF )"
  "-DVNE_TARGET_PLATFORM=Web"
)

if command -v ninja >/dev/null 2>&1; then
  GENERATOR="Ninja"
else
  GENERATOR="Unix Makefiles"
fi

run_configure() {
  mkdir -p "$BUILD_DIR"
  emcmake cmake -G "$GENERATOR" -S "$PROJECT_ROOT" -B "$BUILD_DIR" "${CMAKE_ARGS[@]}"
}

run_build() {
  emmake cmake --build "$BUILD_DIR" --parallel "$JOBS"
}

run_ctest() {
  ctest --test-dir "$BUILD_DIR" --output-on-failure --timeout 300
}

run_serve() {
  [ -d "$SERVE_DIR" ] || { echo "ERROR: $SERVE_DIR not found"; exit 1; }
  python3 -m http.server "$PORT" --directory "$SERVE_DIR"
}

[ "$CLEAN" = true ] && rm -rf "$BUILD_DIR"

case $ACTION in
  configure) run_configure ;;
  build) run_configure; run_build ;;
  configure_and_build) run_configure; run_build; [ "$WITH_TESTS" = true ] && run_ctest ;;
  test) run_ctest ;;
  serve) run_serve; exit 0 ;;
  *) usage ;;
esac

echo "=== Build completed successfully ==="
echo "Artifacts: $SERVE_DIR/"
[ "$SERVE" = true ] && run_serve
