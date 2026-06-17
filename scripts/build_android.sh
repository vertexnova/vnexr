#!/bin/bash

#==============================================================================
# VneXR Android Build Script
#==============================================================================

set -e
trap 'echo "ERROR: command failed at line $LINENO"; exit 1' ERR

usage() {
  echo "Usage: $0 [-t Debug|Release] [-a configure|build|configure_and_build] [-abi arm64-v8a|x86_64] [-api 24] [-l static|shared] [-clean] [-j N] [--dev] [--with-tests] [--with-examples] [--no-tests] [--no-examples]"
  echo ""
  echo "Environment: ANDROID_HOME (or ANDROID_SDK_ROOT), ANDROID_NDK_ROOT (or ANDROID_NDK_HOME)"
  exit 1
}

BUILD_TYPE="Debug"
ACTION="configure_and_build"
ABI="x86_64"
API_LEVEL="24"
LIB_TYPE="shared"
CLEAN=false
JOBS=10
WITH_DEV=true
WITH_EXAMPLES=false
WITH_TESTS=false

while [[ $# -gt 0 ]]; do
  case $1 in
    -t|--type|--build-type) BUILD_TYPE="$2"; shift 2 ;;
    -a|--action) ACTION="$2"; shift 2 ;;
    -abi|--abi) ABI="$2"; shift 2 ;;
    -api|--api) API_LEVEL="$2"; shift 2 ;;
    -l|--lib-type) LIB_TYPE="$2"; shift 2 ;;
    -clean|--clean) CLEAN=true; shift ;;
    -j|--jobs) JOBS="$2"; shift 2 ;;
    --dev) WITH_DEV=true; shift ;;
    --with-examples|--with-samples) WITH_EXAMPLES=true; shift ;;
    --with-tests) WITH_TESTS=true; shift ;;
    --no-examples|--no-samples) WITH_EXAMPLES=false; shift ;;
    --no-tests) WITH_TESTS=false; shift ;;
    -h|--help) usage ;;
    *) echo "Unknown option: $1"; usage ;;
  esac
done

[ "$WITH_DEV" = true ] && { WITH_TESTS=true; WITH_EXAMPLES=true; }

ANDROID_SDK="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}"
ANDROID_NDK="${ANDROID_NDK_ROOT:-${ANDROID_NDK_HOME:-}}"
[ -z "$ANDROID_SDK" ] && { echo "ERROR: Set ANDROID_HOME or ANDROID_SDK_ROOT"; exit 1; }
[ -z "$ANDROID_NDK" ] && { echo "ERROR: Set ANDROID_NDK_ROOT or ANDROID_NDK_HOME"; exit 1; }

PROJECT_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
NDK_REV=$(basename "$ANDROID_NDK")
BUILD_DIR="$PROJECT_ROOT/build/${LIB_TYPE}/${BUILD_TYPE}/build-android-ndk-${NDK_REV}-${ABI}"

[ "$CLEAN" = true ] && rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

VNE_XR_DEV_VAL=OFF
[ "$WITH_DEV" = true ] && VNE_XR_DEV_VAL=ON

CMAKE_ARGS=(
  "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
  "-DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake"
  "-DANDROID_ABI=$ABI"
  "-DANDROID_PLATFORM=android-$API_LEVEL"
  "-DVNE_XR_LIB_TYPE=$LIB_TYPE"
  "-DVNE_XR_CI=OFF"
  "-DVNE_XR_DEV=$VNE_XR_DEV_VAL"
  "-DVNE_XR_TESTS=$( [ "$WITH_TESTS" = true ] && echo ON || echo OFF )"
  "-DVNE_XR_EXAMPLES=$( [ "$WITH_EXAMPLES" = true ] && echo ON || echo OFF )"
  "-DVNE_TARGET_PLATFORM=Android"
  "-DVNE_XR_WITH_OPENXR=ON"
  "-DVNE_XR_BACKEND_VULKAN=ON"
)

run_configure() {
  cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" "${CMAKE_ARGS[@]}"
}

run_build() {
  cmake --build "$BUILD_DIR" --parallel "$JOBS"
}

case $ACTION in
  configure) run_configure ;;
  build) run_build ;;
  configure_and_build) run_configure; run_build ;;
  *) usage ;;
esac

echo "=== Build completed successfully ==="
echo "Build directory: $BUILD_DIR"
