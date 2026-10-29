#!/bin/bash

#==============================================================================
# VneXR Windows Build Script
# Shares build/<lib>/<config>/build-windows-cl-<cl-version> with build_windows.py.
#==============================================================================

set -e

JOBS=10
ARGS=()
while [[ $# -gt 0 ]]; do
    case $1 in
        -j|--jobs) [[ -n "$2" && "$2" =~ ^[0-9]+$ ]] && { JOBS="$2"; shift 2; } || { echo "Invalid jobs: $2"; exit 1; } ;;
        -j*) JOBS="${1#-j}"; [[ "$JOBS" =~ ^[0-9]+$ ]] || { echo "Invalid jobs: $JOBS"; exit 1; }; shift ;;
        *) ARGS+=("$1"); shift ;;
    esac
done
set -- "${ARGS[@]}"

usage() {
  echo "Usage: $0 [-t <build_type>] [-a <action>] [-l <lib_type>] [-clean] [-j <jobs>] [--dev] [--with-openxr] [--with-tests] [--with-examples] [--sample <name>] [--runtime-json <path>]"
  echo "  --dev            Enable tests + examples (VNE_XR_DEV/TESTS/EXAMPLES=ON)"
  echo "  --with-openxr    Build the OpenXR backend + Vulkan (implies samples)"
  echo "  --with-tests     Build vnexr_tests"
  echo "  --with-examples  Build example apps; alias: --with-samples"
  echo "  --no-tests       Omit tests"
  echo "  --no-examples    Omit examples; alias: --no-samples"
  echo "  --sample         Sample name for -a run (vnexr_samples --list)"
  echo "  --runtime-json   OpenXR runtime manifest for -a run (XR_RUNTIME_JSON)"
  echo "  -a run           Configure, build, and launch vnexr_samples"
  exit 1
}

BUILD_TYPE="Debug"
ACTION="configure_and_build"
LIB_TYPE="shared"
CLEAN_BUILD=false
WITH_TESTS=false
WITH_EXAMPLES=false
WITH_DEV=false
WITH_OPENXR=false
NO_TESTS=false
NO_EXAMPLES=false
SAMPLE=""
RUNTIME_JSON=""

while [[ $# -gt 0 ]]; do
  case $1 in
    -t|--build-type) BUILD_TYPE="$2"; shift 2 ;;
    -a|--action) ACTION="$2"; shift 2 ;;
    -l|--lib-type) LIB_TYPE="$2"; shift 2 ;;
    -clean|--clean) CLEAN_BUILD=true; shift ;;
    --dev) WITH_DEV=true; shift ;;
    --with-openxr) WITH_OPENXR=true; shift ;;
    --with-tests) WITH_TESTS=true; shift ;;
    --with-examples|--with-samples) WITH_EXAMPLES=true; shift ;;
    --no-tests) NO_TESTS=true; WITH_TESTS=false; shift ;;
    --no-examples|--no-samples) NO_EXAMPLES=true; WITH_EXAMPLES=false; shift ;;
    --sample) SAMPLE="$2"; shift 2 ;;
    --runtime-json) RUNTIME_JSON="$2"; shift 2 ;;
    -h|--help) usage ;;
    *) echo "Unknown option: $1"; usage ;;
  esac
done

# Make the runtime manifest absolute (and Windows-style) now: later steps change directory, and the launcher is a
# native Windows program that reads XR_RUNTIME_JSON.
if [ -n "$RUNTIME_JSON" ] && command -v cygpath >/dev/null 2>&1; then
  RUNTIME_JSON="$(cygpath -aw "$RUNTIME_JSON")"
fi

[ "$WITH_DEV" = true ] && { WITH_TESTS=true; WITH_EXAMPLES=true; }
[ "$NO_TESTS" = true ] && WITH_TESTS=false
[ "$NO_EXAMPLES" = true ] && WITH_EXAMPLES=false
if [ "$WITH_OPENXR" = true ] || [ "$ACTION" = "run" ]; then
  WITH_EXAMPLES=true
fi
if [ "$WITH_OPENXR" = true ] && [ -z "${VULKAN_SDK:-}" ]; then
  echo "Warning: VULKAN_SDK is not set; install the LunarG Vulkan SDK if CMake cannot find Vulkan."
fi

command -v cl >/dev/null || { echo "Error: run from a Visual Studio Developer Command Prompt"; exit 1; }
command -v cmake >/dev/null || { echo "Error: CMake not found"; exit 1; }

COMPILER_VERSION="unknown"
if COMPILER_OUTPUT=$(cl 2>&1); then
  COMPILER_VERSION=$(printf '%s\n' "$COMPILER_OUTPUT" | grep -oE 'Version [0-9]+\.[0-9]+\.[0-9]+' | head -1 | awk '{print $2}')
  [ -n "$COMPILER_VERSION" ] || COMPILER_VERSION="unknown"
fi

PROJECT_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR="$PROJECT_ROOT/build/${LIB_TYPE}/${BUILD_TYPE}/build-windows-cl-${COMPILER_VERSION}"

VNE_XR_DEV_VAL=OFF
[ "$WITH_DEV" = true ] && VNE_XR_DEV_VAL=ON
TESTS_FLAG=$( [ "$WITH_TESTS" = true ] && echo ON || echo OFF )
EXAMPLES_FLAG=$( [ "$WITH_EXAMPLES" = true ] && echo ON || echo OFF )
# Always pass the switch: an omitted flag would otherwise keep a cached ON from an earlier configure.
OPENXR_FLAGS=(-DVNE_XR_WITH_OPENXR=OFF)
if [ "$WITH_OPENXR" = true ]; then
  OPENXR_FLAGS=(-DVNE_XR_WITH_OPENXR=ON -DVNE_XR_WITH_RHI=ON -DVNE_XR_BACKEND_VULKAN=ON)
fi

build_cmake_command() {
  cmake -G "Visual Studio 17 2022" -A x64 \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DVNE_XR_LIB_TYPE="$LIB_TYPE" \
    -DCMAKE_C_COMPILER=cl \
    -DCMAKE_CXX_COMPILER=cl \
    -DVNE_XR_DEV="$VNE_XR_DEV_VAL" \
    -DVNE_XR_TESTS="$TESTS_FLAG" \
    -DVNE_XR_EXAMPLES="$EXAMPLES_FLAG" \
    "${OPENXR_FLAGS[@]}" \
    "$PROJECT_ROOT"
}

BUILD_COMMAND="cmake --build . --config $BUILD_TYPE --parallel $JOBS"
TEST_COMMAND="ctest -C $BUILD_TYPE --output-on-failure"

clean_build() { rm -rf "$BUILD_DIR"; mkdir -p "$BUILD_DIR"; cd "$BUILD_DIR" || exit; }
ensure_build_dir() { mkdir -p "$BUILD_DIR"; cd "$BUILD_DIR" || exit; }

prepend_runtime_path() {
  local dir="$1"
  [ -d "$dir" ] || return 0
  if command -v cygpath >/dev/null 2>&1; then
    export PATH="$(cygpath -u "$dir"):$PATH"
  else
    export PATH="$dir;$PATH"
  fi
}

run_sample() {
  local launcher=""
  local samples_dir="$BUILD_DIR/bin/samples"
  if [[ -f "$samples_dir/$BUILD_TYPE/vnexr_samples.exe" ]]; then
    launcher="$samples_dir/$BUILD_TYPE/vnexr_samples.exe"
  elif [[ -f "$samples_dir/vnexr_samples.exe" ]]; then
    launcher="$samples_dir/vnexr_samples.exe"
  else
    echo "Error: vnexr_samples.exe not found under $samples_dir"
    exit 1
  fi
  local prepended=()
  if [[ -d "$BUILD_DIR/bin/$BUILD_TYPE" ]]; then
    prepend_runtime_path "$BUILD_DIR/bin/$BUILD_TYPE"
    prepended+=("$BUILD_DIR/bin/$BUILD_TYPE")
  fi
  if [[ -n "${VULKAN_SDK:-}" && -d "$VULKAN_SDK/Bin" ]]; then
    prepend_runtime_path "$VULKAN_SDK/Bin"
    prepended+=("$VULKAN_SDK/Bin")
  fi
  if [[ ${#prepended[@]} -gt 0 ]]; then
    echo "PATH prepended: ${prepended[*]}"
  fi
  if [[ -n "$RUNTIME_JSON" ]]; then
    export XR_RUNTIME_JSON="$RUNTIME_JSON"
    echo "XR_RUNTIME_JSON=$RUNTIME_JSON"
  fi
  local cmd=("$launcher")
  if [[ -n "$SAMPLE" ]]; then
    cmd+=(--sample "$SAMPLE")
  fi
  echo "Running: ${cmd[*]}"
  (cd "$(dirname "$launcher")" && "${cmd[@]}")
}

echo "Windows :: cl-${COMPILER_VERSION} ($BUILD_TYPE, $LIB_TYPE)"

case $ACTION in
  configure) [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir; build_cmake_command ;;
  build|configure_and_build)
    [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir
    build_cmake_command
    eval $BUILD_COMMAND
    ;;
  test)
    [ "$WITH_TESTS" != true ] && { echo "ERROR: Tests disabled. Use --with-tests or --dev."; exit 1; }
    [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir
    build_cmake_command
    eval $BUILD_COMMAND
    eval $TEST_COMMAND
    ;;
  run)
    [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir
    build_cmake_command
    eval $BUILD_COMMAND
    run_sample
    ;;
  *) usage ;;
esac

echo ""
echo "=== Build completed successfully ==="
echo "Build directory: $BUILD_DIR"
echo "Tests enabled: $WITH_TESTS | Examples enabled: $WITH_EXAMPLES | OpenXR: $WITH_OPENXR"
