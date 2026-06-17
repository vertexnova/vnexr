#!/bin/bash

#==============================================================================
# VneXR macOS Build Script
#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Author:    Ajeet Singh Yadav
# Created:   February 2026
#
# Autodoc:   yes
#
# This script builds VneXR for macOS with optional Xcode project generation.
#==============================================================================

set -e
trap 'echo "ERROR: command failed at line $LINENO"; exit 1' ERR

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

PLATFORM="Darwin"
COMPILER="clang"

usage() {
    echo "Usage: $0 [-t <build_type>] [-a <action>] [-l <lib_type>] [-clean] [-interactive] [-j <jobs>] [-xcode] [-xcode-only] [--dev] [--with-tests] [--with-examples] [--library-only]"
    echo "Options:"
    echo "  -t <build_type>    Debug|Release|RelWithDebInfo|MinSizeRel"
    echo "  -a <action>        configure|build|configure_and_build|test|xcode|xcode_build"
    echo "  -l <lib_type>      static|shared (default: shared)"
    echo "  -clean             Clean build and xcode directories first"
    echo "  -interactive       Interactive mode"
    echo "  -j <jobs>          Parallel jobs (default: 10)"
    echo "  -xcode             Also generate a dedicated Xcode project directory"
    echo "  -xcode-only        Only generate the dedicated Xcode project directory"
    echo "  --dev              Enable tests + examples (VNE_XR_DEV/TESTS/EXAMPLES=ON)"
    echo "  --with-tests       Build vnexr_tests"
    echo "  --with-examples    Build example apps (01_hello_xr); alias: --with-samples"
    echo "  --no-tests         Omit tests"
    echo "  --no-examples      Omit examples; alias: --no-samples"
    echo "  --library-only     Library-only Xcode project (no tests or examples)"
    echo ""
    echo "Examples/tests in Xcode:"
    echo "  -xcode / -xcode-only enables tests + examples by default (use --library-only to skip)."
    echo "  Command-line builds omit them unless --dev, --with-tests, or --with-examples is set."
    exit 1
}

BUILD_TYPE="Debug"
ACTION="configure_and_build"
LIB_TYPE="shared"
CLEAN=false
GENERATE_XCODE=false
XCODE_ONLY=false
INTERACTIVE_MODE=false
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
        -interactive|--interactive) INTERACTIVE_MODE=true; shift ;;
        -xcode|--xcode) GENERATE_XCODE=true; shift ;;
        -xcode-only|--xcode-only) XCODE_ONLY=true; ACTION="xcode"; shift ;;
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

if [ "$LIBRARY_ONLY" = true ]; then
    WITH_TESTS=false
    WITH_EXAMPLES=false
elif [ "$WITH_DEV" = true ]; then
    WITH_TESTS=true
    WITH_EXAMPLES=true
elif [ "$GENERATE_XCODE" = true ] || [ "$XCODE_ONLY" = true ] || [ "$ACTION" = "xcode" ]; then
    [ "$NO_TESTS" != true ] && WITH_TESTS=true
    [ "$NO_EXAMPLES" != true ] && WITH_EXAMPLES=true
fi

[ "$NO_TESTS" = true ] && WITH_TESTS=false
[ "$NO_EXAMPLES" = true ] && WITH_EXAMPLES=false

if [ "$INTERACTIVE_MODE" = true ]; then
    echo "=== VneXR Interactive Build Configuration (macOS) ==="
    read -p "Build type [Debug]: " bt
    [ -n "$bt" ] && BUILD_TYPE="$bt"
    read -p "Action [configure_and_build]: " act
    [ -n "$act" ] && ACTION="$act"
    read -p "Enable examples? (y/N): " samp
    [[ "$samp" =~ ^[Yy]$ ]] && WITH_EXAMPLES=true
    read -p "Enable tests? (y/N): " tst
    [[ "$tst" =~ ^[Yy]$ ]] && WITH_TESTS=true
fi

COMPILER_VERSION=$(clang --version | awk 'NR==1 {print $4}' | sed 's/(.*)//')
[ "$COMPILER_VERSION" = "version" ] && COMPILER_VERSION=$(clang --version | awk 'NR==1 {print $3}')

echo "$PLATFORM :: $COMPILER-${COMPILER_VERSION}"

PROJECT_ROOT=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR="$PROJECT_ROOT/build/${LIB_TYPE}/${BUILD_TYPE}/build-macos-$COMPILER-${COMPILER_VERSION}"
XCODE_DIR="$PROJECT_ROOT/build/${LIB_TYPE}/${BUILD_TYPE}/xcode-macos-$COMPILER-${COMPILER_VERSION}"

if [ "$CLEAN" = true ]; then
    rm -rf "$BUILD_DIR" "$XCODE_DIR"
fi
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
    "-DCMAKE_C_COMPILER=clang"
    "-DCMAKE_CXX_COMPILER=clang++"
    "-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15"
    "-DCMAKE_CXX_STANDARD=20"
    "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
    "-DCMAKE_CXX_EXTENSIONS=OFF"
)

run_cmake_configure() {
    local build_path="$1"
    local generator="$2"
    echo "Configuring CMake ($generator) in: $build_path"
    cd "$build_path"
    if [ "$generator" = "Xcode" ]; then
        cmake -G Xcode "${CMAKE_ARGS[@]}" "$PROJECT_ROOT"
    else
        cmake "${CMAKE_ARGS[@]}" "$PROJECT_ROOT"
    fi
    cd - > /dev/null
}

run_make_build() {
    echo "Building with make in: $1"
    make -C "$1" -j"$JOBS"
}

run_xcode_build() {
    local xcode_path="$1"
    cd "$xcode_path"
    local xcodeproj_file
    shopt -s nullglob
    xcodeproj_file=( *.xcodeproj )
    shopt -u nullglob
    xcodeproj_file="${xcodeproj_file[0]}"
    [ -z "$xcodeproj_file" ] && { echo "ERROR: No Xcode project in $xcode_path"; exit 1; }
    xcodebuild -project "$xcodeproj_file" -scheme ALL_BUILD -configuration "$BUILD_TYPE" -jobs "$JOBS"
    cd - > /dev/null
}

echo "=== VneXR macOS Build Script ==="
echo "Build Type: $BUILD_TYPE | Lib Type: $LIB_TYPE | Action: $ACTION"
echo "Tests: $WITH_TESTS | Examples: $WITH_EXAMPLES | Jobs: $JOBS"
echo ""

case $ACTION in
    configure) run_cmake_configure "$BUILD_DIR" "Unix Makefiles" ;;
    build|configure_and_build)
        run_cmake_configure "$BUILD_DIR" "Unix Makefiles"
        run_make_build "$BUILD_DIR"
        [ "$GENERATE_XCODE" = true ] && run_cmake_configure "$XCODE_DIR" "Xcode"
        ;;
    test)
        [ "$WITH_TESTS" != true ] && { echo "ERROR: Tests disabled. Use --with-tests or --dev."; exit 1; }
        run_cmake_configure "$BUILD_DIR" "Unix Makefiles"
        run_make_build "$BUILD_DIR"
        ctest --test-dir "$BUILD_DIR" --output-on-failure
        ;;
    xcode_build)
        run_cmake_configure "$BUILD_DIR" "Xcode"
        run_xcode_build "$BUILD_DIR"
        ;;
    xcode|xcode-only) run_cmake_configure "$XCODE_DIR" "Xcode" ;;
    *) usage ;;
esac

echo ""
echo "=== Build completed successfully ==="
if [ "$ACTION" = "xcode" ] || [ "$ACTION" = "xcode-only" ]; then
    echo "Xcode project: $XCODE_DIR"
    echo "Open: open $XCODE_DIR/*.xcodeproj"
else
    echo "Build directory: $BUILD_DIR"
    [ "$GENERATE_XCODE" = true ] && echo "Xcode project: $XCODE_DIR"
fi
