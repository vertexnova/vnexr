#!/bin/bash

#==============================================================================
# VneXR Linux Build Script
#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Author:    Ajeet Singh Yadav
# Created:   February 2026
#
# Autodoc:   yes
#
# This script builds VneXR for Linux platforms
#==============================================================================

set -e

JOBS=10
ARGS=()

while [[ $# -gt 0 ]]; do
    case $1 in
        -j|--jobs)
            if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then
                JOBS="$2"
                shift 2
            else
                echo "Invalid number of jobs: $2"
                exit 1
            fi
            ;;
        -j*)
            JOBS="${1#-j}"
            if [[ ! "$JOBS" =~ ^[0-9]+$ ]]; then
                echo "Invalid number of jobs: $JOBS"
                exit 1
            fi
            shift
            ;;
        *)
            ARGS+=("$1")
            shift
            ;;
    esac
done
set -- "${ARGS[@]}"

PLATFORM="Linux"
COMPILER="gcc"

usage() {
  echo "Usage: $0 [-t <build_type>] [-a <action>] [-c <compiler>] [-l <lib_type>] [-clean] [-interactive] [-j <jobs>] [--dev] [--with-openxr] [--with-tests] [--with-examples]"
  echo "Options:"
  echo "  -t <build_type>    Debug, Release, RelWithDebInfo, MinSizeRel"
  echo "  -a <action>        configure|build|configure_and_build|test"
  echo "  -c <compiler>      gcc or clang"
  echo "  -l <lib_type>      static or shared (default: shared)"
  echo "  -clean             Clean the build directory before performing the action"
  echo "  -interactive       Run in interactive mode to choose options"
  echo "  -j <jobs>          Number of parallel jobs (default: 10)"
  echo "  --dev              Enable tests + examples (VNE_XR_DEV/TESTS/EXAMPLES=ON)"
  echo "  --with-openxr      Build OpenXR backend + Vulkan (Monado / PC VR on Linux)"
  echo "  --with-tests       Build vnexr_tests"
  echo "  --with-examples    Build example apps (03_hello_openxr when --with-openxr)"
  echo "  --no-tests         Omit tests"
  echo "  --no-examples      Omit examples"
  echo ""
  echo "OpenXR on Linux (first time):"
  echo "  ./scripts/install_linux_openxr_deps.sh"
  echo "  export VNE_VNERHI_PATH=/path/to/vnerhi   # optional local vnerhi"
  echo "  ./scripts/init_submodules.sh full"
  echo ""
  echo "Examples:"
  echo "  $0 -t Debug -a configure_and_build --dev"
  echo "  $0 -t Debug -a configure_and_build --with-openxr --with-examples"
  echo "  $0 -c clang -l static -j 32 --with-openxr --dev"
  echo "  $0 -interactive"
  exit 1
}

interactive_mode() {
  echo "=== VneXR Interactive Build Configuration ==="
  echo ""
  echo "Detected Platform: $PLATFORM"
  echo ""

  echo "Select Build Type:"
  echo "1) Debug (default)"
  echo "2) Release"
  echo "3) RelWithDebInfo"
  echo "4) MinSizeRel"
  read -p "Enter choice (1-4) [1]: " build_choice
  case $build_choice in
    2) BUILD_TYPE="Release" ;;
    3) BUILD_TYPE="RelWithDebInfo" ;;
    4) BUILD_TYPE="MinSizeRel" ;;
    *) BUILD_TYPE="Debug" ;;
  esac

  echo ""
  echo "Select Compiler:"
  echo "1) GCC (default)"
  echo "2) Clang"
  read -p "Enter choice (1-2) [1]: " compiler_choice
  case $compiler_choice in
    2) COMPILER="clang" ;;
    *) COMPILER="gcc" ;;
  esac

  echo ""
  echo "Select library type:"
  echo "1) shared (default)"
  echo "2) static"
  read -p "Enter choice (1-2) [1]: " lib_choice
  case $lib_choice in
    2) LIB_TYPE="static" ;;
    *) LIB_TYPE="shared" ;;
  esac

  echo ""
  echo "Select Action:"
  echo "1) Configure only"
  echo "2) Configure and build (default)"
  echo "3) Configure, build, and test"
  read -p "Enter choice (1-3) [2]: " action_choice
  case $action_choice in
    1) ACTION="configure" ;;
    3) ACTION="test" ;;
    *) ACTION="configure_and_build" ;;
  esac

  echo ""
  read -p "Clean build directory before starting? (y/N): " clean_choice
  [[ $clean_choice =~ ^[Yy]$ ]] && CLEAN_BUILD=true

  echo ""
  read -p "Enable tests? (Y/n): " tests_choice
  [[ $tests_choice =~ ^[Nn]$ ]] && WITH_TESTS=false || WITH_TESTS=true
  read -p "Enable examples? (Y/n): " examples_choice
  [[ $examples_choice =~ ^[Nn]$ ]] && WITH_EXAMPLES=false || WITH_EXAMPLES=true

  echo ""
  read -p "Build OpenXR backend (Vulkan + 03_hello_openxr)? (y/N): " openxr_choice
  [[ $openxr_choice =~ ^[Yy]$ ]] && WITH_OPENXR=true

  echo ""
  read -p "Proceed with this configuration? (Y/n): " proceed
  [[ $proceed =~ ^[Nn]$ ]] && { echo "Build cancelled."; exit 0; }
}

BUILD_TYPE="Debug"
ACTION="configure_and_build"
LIB_TYPE="shared"
CLEAN_BUILD=false
INTERACTIVE_MODE=false
WITH_TESTS=true
WITH_EXAMPLES=true
WITH_DEV=false
NO_TESTS=false
NO_EXAMPLES=false
WITH_OPENXR=false

while [[ $# -gt 0 ]]; do
  case $1 in
    -t|--build-type) BUILD_TYPE="$2"; shift 2 ;;
    -a|--action) ACTION="$2"; shift 2 ;;
    -c|--compiler) COMPILER="$2"; shift 2 ;;
    -l|--lib-type) LIB_TYPE="$2"; shift 2 ;;
    -clean|--clean) CLEAN_BUILD=true; shift ;;
    -interactive|--interactive) INTERACTIVE_MODE=true; shift ;;
    --dev) WITH_DEV=true; shift ;;
    --with-openxr) WITH_OPENXR=true; shift ;;
    --with-tests) WITH_TESTS=true; shift ;;
    --with-examples|--with-samples) WITH_EXAMPLES=true; shift ;;
    --no-tests) NO_TESTS=true; WITH_TESTS=false; shift ;;
    --no-examples|--no-samples) NO_EXAMPLES=true; WITH_EXAMPLES=false; shift ;;
    -h|--help) usage ;;
    *) echo "Unknown option: $1"; usage ;;
  esac
done

if [ "$WITH_DEV" = true ]; then
  WITH_TESTS=true
  WITH_EXAMPLES=true
fi
[ "$WITH_OPENXR" = true ] && WITH_EXAMPLES=true
[ "$NO_TESTS" = true ] && WITH_TESTS=false
[ "$NO_EXAMPLES" = true ] && WITH_EXAMPLES=false
[ "$INTERACTIVE_MODE" = true ] && interactive_mode

if [ "$COMPILER" = "gcc" ]; then
  COMPILER_VERSION=$(gcc --version | head -n 1 | awk '{print $4}')
elif [ "$COMPILER" = "clang" ]; then
  COMPILER_VERSION=$(clang --version | head -n 1 | awk '{print $3}')
else
  echo "Unsupported compiler: $COMPILER"
  exit 1
fi

echo "$PLATFORM :: $COMPILER-${COMPILER_VERSION}"

PROJECT_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR="$PROJECT_ROOT/build/${LIB_TYPE}/${BUILD_TYPE}/build-linux-$COMPILER-${COMPILER_VERSION}"

IPO_FLAGS=""
[ "$COMPILER" = "clang" ] && IPO_FLAGS="-DENABLE_IPO=OFF"

VNE_XR_DEV_VAL=OFF
[ "$WITH_DEV" = true ] && VNE_XR_DEV_VAL=ON
TESTS_FLAG=$( [ "$WITH_TESTS" = true ] && echo ON || echo OFF )
EXAMPLES_FLAG=$( [ "$WITH_EXAMPLES" = true ] && echo ON || echo OFF )

OPENXR_FLAGS=""
if [ "$WITH_OPENXR" = true ]; then
  if ! pkg-config --exists vulkan 2>/dev/null && ! command -v vulkaninfo >/dev/null 2>&1; then
    echo "WARNING: Vulkan dev packages may be missing. Run: ./scripts/install_linux_openxr_deps.sh"
  fi
  OPENXR_FLAGS="-DVNE_XR_WITH_OPENXR=ON -DVNE_XR_WITH_RHI=ON -DVNE_XR_BACKEND_VULKAN=ON"
fi

build_cmake_command() {
  local cxx_flags="-DCMAKE_BUILD_TYPE=$BUILD_TYPE -DVNE_XR_LIB_TYPE=$LIB_TYPE -DVNE_XR_DEV=$VNE_XR_DEV_VAL -DVNE_XR_TESTS=$TESTS_FLAG -DVNE_XR_EXAMPLES=$EXAMPLES_FLAG $OPENXR_FLAGS $IPO_FLAGS"
  if [ "$COMPILER" = "gcc" ]; then
    echo "cmake $cxx_flags -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ $PROJECT_ROOT"
  else
    echo "cmake $cxx_flags -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ $PROJECT_ROOT"
  fi
}

CONFIGURE_COMMAND=$(build_cmake_command)
BUILD_COMMAND="make -j$JOBS"
TEST_COMMAND="ctest --output-on-failure"

clean_build() { rm -rf "$BUILD_DIR"; mkdir -p "$BUILD_DIR"; cd "$BUILD_DIR" || exit; }
ensure_build_dir() { mkdir -p "$BUILD_DIR"; cd "$BUILD_DIR" || exit; }
configure_project() { echo "Configuring with command: $CONFIGURE_COMMAND"; eval $CONFIGURE_COMMAND; }
build_project() { echo "Building with $JOBS parallel jobs..."; eval $BUILD_COMMAND; }
run_tests() { eval $TEST_COMMAND; }

case $ACTION in
  configure)
    [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir
    configure_project
    ;;
  build|configure_and_build)
    [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir
    configure_project
    build_project
    ;;
  test)
    [ "$WITH_TESTS" != true ] && { echo "ERROR: Tests disabled. Use --with-tests or --dev."; exit 1; }
    [ "$CLEAN_BUILD" = true ] && clean_build || ensure_build_dir
    configure_project
    build_project
    run_tests
    ;;
  *) usage ;;
esac

echo ""
echo "=== Build completed successfully ==="
echo "Build directory: $BUILD_DIR"
echo "Tests enabled: $WITH_TESTS | Examples enabled: $WITH_EXAMPLES | OpenXR: $WITH_OPENXR"
