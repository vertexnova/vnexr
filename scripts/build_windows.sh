#!/bin/bash

#==============================================================================
# VneXR Windows Build Script
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
  echo "Usage: $0 [-t <build_type>] [-a <action>] [-l <lib_type>] [-clean] [-j <jobs>] [--dev] [--with-tests] [--with-examples]"
  echo "  --dev            Enable tests + examples (VNE_XR_DEV/TESTS/EXAMPLES=ON)"
  echo "  --with-tests     Build vnexr_tests"
  echo "  --with-examples  Build example apps; alias: --with-samples"
  echo "  --no-tests       Omit tests"
  echo "  --no-examples    Omit examples; alias: --no-samples"
  exit 1
}

BUILD_TYPE="Debug"
ACTION="configure_and_build"
LIB_TYPE="shared"
CLEAN_BUILD=false
WITH_TESTS=false
WITH_EXAMPLES=false
WITH_DEV=false
NO_TESTS=false
NO_EXAMPLES=false

while [[ $# -gt 0 ]]; do
  case $1 in
    -t|--build-type) BUILD_TYPE="$2"; shift 2 ;;
    -a|--action) ACTION="$2"; shift 2 ;;
    -l|--lib-type) LIB_TYPE="$2"; shift 2 ;;
    -clean|--clean) CLEAN_BUILD=true; shift ;;
    --dev) WITH_DEV=true; shift ;;
    --with-tests) WITH_TESTS=true; shift ;;
    --with-examples|--with-samples) WITH_EXAMPLES=true; shift ;;
    --no-tests) NO_TESTS=true; WITH_TESTS=false; shift ;;
    --no-examples|--no-samples) NO_EXAMPLES=true; WITH_EXAMPLES=false; shift ;;
    -h|--help) usage ;;
    *) echo "Unknown option: $1"; usage ;;
  esac
done

[ "$WITH_DEV" = true ] && { WITH_TESTS=true; WITH_EXAMPLES=true; }
[ "$NO_TESTS" = true ] && WITH_TESTS=false
[ "$NO_EXAMPLES" = true ] && WITH_EXAMPLES=false

command -v cl >/dev/null || { echo "Error: run from a Visual Studio Developer Command Prompt"; exit 1; }
command -v cmake >/dev/null || { echo "Error: CMake not found"; exit 1; }

COMPILER_VERSION="unknown"
if COMPILER_OUTPUT=$(cl 2>&1 | grep -i "version" | head -1); then
  COMPILER_VERSION=$(echo "$COMPILER_OUTPUT" | awk '{print $3}' | sed 's/[^0-9.]//g')
fi

PROJECT_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR="$PROJECT_ROOT/build/${LIB_TYPE}/${BUILD_TYPE}/build-windows-cl-${COMPILER_VERSION}"

VNE_XR_DEV_VAL=OFF
[ "$WITH_DEV" = true ] && VNE_XR_DEV_VAL=ON
TESTS_FLAG=$( [ "$WITH_TESTS" = true ] && echo ON || echo OFF )
EXAMPLES_FLAG=$( [ "$WITH_EXAMPLES" = true ] && echo ON || echo OFF )

build_cmake_command() {
  cmake -G "Visual Studio 17 2022" -A x64 \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DVNE_XR_LIB_TYPE="$LIB_TYPE" \
    -DCMAKE_C_COMPILER=cl \
    -DCMAKE_CXX_COMPILER=cl \
    -DVNE_XR_DEV="$VNE_XR_DEV_VAL" \
    -DVNE_XR_TESTS="$TESTS_FLAG" \
    -DVNE_XR_EXAMPLES="$EXAMPLES_FLAG" \
    -DVNE_XR_WITH_OPENXR=ON \
    -DVNE_XR_BACKEND_VULKAN=ON \
    "$PROJECT_ROOT"
}

BUILD_COMMAND="cmake --build . --config $BUILD_TYPE --parallel $JOBS"
TEST_COMMAND="ctest -C $BUILD_TYPE --output-on-failure"

clean_build() { rm -rf "$BUILD_DIR"; mkdir -p "$BUILD_DIR"; cd "$BUILD_DIR" || exit; }
ensure_build_dir() { mkdir -p "$BUILD_DIR"; cd "$BUILD_DIR" || exit; }

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
  *) usage ;;
esac

echo ""
echo "=== Build completed successfully ==="
echo "Build directory: $BUILD_DIR"
echo "Tests enabled: $WITH_TESTS | Examples enabled: $WITH_EXAMPLES"
