#!/bin/bash

#==============================================================================
# VneXR iOS Build Script
#==============================================================================

set -e
trap 'echo "ERROR: command failed at line $LINENO"; exit 1' ERR

JOBS=10
ARGS=()
while [[ $# -gt 0 ]]; do
    case $1 in
        -j|--jobs) [[ -n "$2" && "$2" =~ ^[0-9]+$ ]] && { JOBS="$2"; shift 2; } || { echo "Invalid jobs: $2"; exit 1; } ;;
        -j*) JOBS="${1#-j}"; [[ "$JOBS" =~ ^[0-9]+$ ]] || exit 1; shift ;;
        *) ARGS+=("$1"); shift ;;
    esac
done
set -- "${ARGS[@]}"

usage() {
  echo "Usage: $0 [-t <build_type>] [-a <action>] [-l <lib_type>] [-clean] [-j <jobs>] [-xcode] [-xcode-only] [-simulator] [-device] [-deployment-target <ver>] [--dev] [--with-tests] [--with-examples] [--library-only]"
  exit 1
}

BUILD_TYPE="Debug"
ACTION="configure_and_build"
LIB_TYPE="static"
CLEAN=false
GENERATE_XCODE=false
XCODE_ONLY=false
TARGET="simulator"
IOS_DEPLOYMENT_TARGET="15.0"
WITH_TESTS=false
WITH_EXAMPLES=false
WITH_DEV=false
LIBRARY_ONLY=false
NO_TESTS=false
NO_EXAMPLES=false

while [[ $# -gt 0 ]]; do
  case $1 in
    -t|--build-type) BUILD_TYPE="$2"; shift 2 ;;
    -a|--action) ACTION="$2"; shift 2 ;;
    -l|--lib-type) LIB_TYPE="$2"; shift 2 ;;
    -clean|--clean) CLEAN=true; shift ;;
    -xcode|--xcode) GENERATE_XCODE=true; shift ;;
    -xcode-only|--xcode-only) XCODE_ONLY=true; ACTION="xcode"; shift ;;
    -simulator) TARGET="simulator"; shift ;;
    -device) TARGET="device"; shift ;;
    -deployment-target) IOS_DEPLOYMENT_TARGET="$2"; shift 2 ;;
    --dev) WITH_DEV=true; shift ;;
    --with-tests) WITH_TESTS=true; shift ;;
    --with-examples|--with-samples) WITH_EXAMPLES=true; shift ;;
    --no-tests) NO_TESTS=true; WITH_TESTS=false; shift ;;
    --no-examples|--no-samples) NO_EXAMPLES=true; WITH_EXAMPLES=false; shift ;;
    --library-only) LIBRARY_ONLY=true; WITH_TESTS=false; WITH_EXAMPLES=false; shift ;;
    -h|--help) usage ;;
    *) echo "Unknown option: $1"; usage ;;
  esac
done

[ "$LIBRARY_ONLY" = true ] && { WITH_TESTS=false; WITH_EXAMPLES=false; }
[ "$WITH_DEV" = true ] && { WITH_TESTS=true; WITH_EXAMPLES=true; }
if [ "$GENERATE_XCODE" = true ] || [ "$XCODE_ONLY" = true ] || [ "$ACTION" = "xcode" ]; then
  [ "$NO_TESTS" != true ] && [ "$LIBRARY_ONLY" != true ] && WITH_TESTS=true
  [ "$NO_EXAMPLES" != true ] && [ "$LIBRARY_ONLY" != true ] && WITH_EXAMPLES=true
fi
[ "$NO_TESTS" = true ] && WITH_TESTS=false
[ "$NO_EXAMPLES" = true ] && WITH_EXAMPLES=false

command -v xcodebuild >/dev/null || { echo "ERROR: Xcode not found"; exit 1; }

PROJECT_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
COMPILER_VERSION=$(clang --version | awk 'NR==1 {print $4}' | sed 's/(.*)//')
BUILD_DIR="$PROJECT_ROOT/build/${LIB_TYPE}/${BUILD_TYPE}/build-ios-clang-${COMPILER_VERSION}"
XCODE_DIR="$PROJECT_ROOT/build/${LIB_TYPE}/${BUILD_TYPE}/xcode-ios-clang-${COMPILER_VERSION}"

[ "$CLEAN" = true ] && rm -rf "$BUILD_DIR" "$XCODE_DIR"
mkdir -p "$BUILD_DIR" "$XCODE_DIR"

VNE_XR_DEV_VAL=OFF
[ "$WITH_DEV" = true ] && VNE_XR_DEV_VAL=ON

CMAKE_ARGS=(
  "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
  "-DVNE_XR_LIB_TYPE=$LIB_TYPE"
  "-DVNE_XR_DEV=$VNE_XR_DEV_VAL"
  "-DVNE_XR_CI=OFF"
  "-DVNE_XR_TESTS=$( [ "$WITH_TESTS" = true ] && echo ON || echo OFF )"
  "-DVNE_XR_EXAMPLES=$( [ "$WITH_EXAMPLES" = true ] && echo ON || echo OFF )"
  "-DVNE_TARGET_PLATFORM=iOS"
  "-DCMAKE_SYSTEM_NAME=iOS"
  "-DCMAKE_OSX_DEPLOYMENT_TARGET=$IOS_DEPLOYMENT_TARGET"
  "-DCMAKE_C_COMPILER=clang"
  "-DCMAKE_CXX_COMPILER=clang++"
)

if [ "$TARGET" = "device" ]; then
  CMAKE_ARGS+=("-DCMAKE_OSX_SYSROOT=iphoneos" "-DCMAKE_OSX_ARCHITECTURES=arm64")
else
  CMAKE_ARGS+=("-DCMAKE_OSX_SYSROOT=iphonesimulator" "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64")
fi

run_cmake_configure() {
  cd "$1"
  cmake -G Xcode "${CMAKE_ARGS[@]}" "$PROJECT_ROOT"
  cd - >/dev/null
}

run_xcode_build() {
  cd "$1"
  local sdk=iphonesimulator
  [ "$TARGET" = "device" ] && sdk=iphoneos
  local proj=( *.xcodeproj )
  xcodebuild -project "${proj[0]}" -scheme ALL_BUILD -configuration "$BUILD_TYPE" -sdk "$sdk" -jobs "$JOBS"
  cd - >/dev/null
}

case $ACTION in
  configure) run_cmake_configure "$BUILD_DIR" ;;
  build|configure_and_build)
    run_cmake_configure "$BUILD_DIR"
    run_xcode_build "$BUILD_DIR"
    [ "$GENERATE_XCODE" = true ] && run_cmake_configure "$XCODE_DIR"
    ;;
  xcode|xcode-only) run_cmake_configure "$XCODE_DIR" ;;
  xcode_build)
    run_cmake_configure "$BUILD_DIR"
    run_xcode_build "$BUILD_DIR"
    ;;
  test)
    [ "$WITH_TESTS" != true ] && { echo "ERROR: Tests disabled"; exit 1; }
    run_cmake_configure "$BUILD_DIR"
    run_xcode_build "$BUILD_DIR"
    ctest --test-dir "$BUILD_DIR" -C "$BUILD_TYPE" --output-on-failure
    ;;
  *) usage ;;
esac

echo "=== Build completed successfully ==="
