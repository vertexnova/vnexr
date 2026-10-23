#!/usr/bin/env bash

#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Author: Ajeet Singh Yadav
# Created: September 2026
#==============================================================================

# vnexr script helpers
#
# Shared helpers for the scripts in scripts/. Source it, never execute it:
#
#   source "$(dirname "${BASH_SOURCE[0]}")/lib/common.sh"
#
# Must stay bash 3.2 compatible (macOS /bin/bash): no associative arrays,
# mapfile/readarray, or ${var,,} case conversion.
#
# Helpers that can fail call die() directly. Never call them inside $(...):
# die would only leave the subshell.

if [[ -n "${VNE_COMMON_SH_LOADED:-}" ]]; then
    return 0
fi
VNE_COMMON_SH_LOADED=1

SCRIPTS_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPTS_DIR}/.." && pwd)"

#------------------------------------------------------------------------------
# Logging: info/ok go to stdout, warn/error to stderr. Colors only on a TTY
# (the CMake shader targets capture this output into build logs).
#------------------------------------------------------------------------------
_VNE_BLUE='' _VNE_GREEN='' _VNE_OUT_RESET=''
_VNE_YELLOW='' _VNE_RED='' _VNE_ERR_RESET=''
if [[ -z "${NO_COLOR:-}" ]]; then
    if [[ -t 1 ]]; then
        _VNE_BLUE=$'\033[0;34m' _VNE_GREEN=$'\033[0;32m' _VNE_OUT_RESET=$'\033[0m'
    fi
    if [[ -t 2 ]]; then
        _VNE_YELLOW=$'\033[1;33m' _VNE_RED=$'\033[0;31m' _VNE_ERR_RESET=$'\033[0m'
    fi
fi

log_info() { printf '%s[INFO]%s %s\n' "${_VNE_BLUE}" "${_VNE_OUT_RESET}" "$*"; }
log_ok() { printf '%s[OK]%s %s\n' "${_VNE_GREEN}" "${_VNE_OUT_RESET}" "$*"; }
log_warn() { printf '%s[WARN]%s %s\n' "${_VNE_YELLOW}" "${_VNE_ERR_RESET}" "$*" >&2; }
log_error() { printf '%s[ERROR]%s %s\n' "${_VNE_RED}" "${_VNE_ERR_RESET}" "$*" >&2; }
die() {
    log_error "$@"
    exit 1
}

# enable_error_trap: report the failing command and line, then exit 1.
# Used by the build scripts, whose steps are long external commands.
enable_error_trap() {
    trap '_vne_on_error $? $LINENO' ERR
}

_vne_on_error() {
    log_error "Command failed (exit $1) at line $2: ${BASH_COMMAND}"
    exit 1
}

#------------------------------------------------------------------------------
# Option parsing
#------------------------------------------------------------------------------

# require_value <option> <value>: an option's value must be present and must
# not look like another option. Calls the script's usage() otherwise.
require_value() {
    if [[ -z "${2:-}" || "${2:0:1}" == "-" ]]; then
        log_error "Option '$1' requires a value"
        usage
    fi
}

# set_jobs <value>: validate a -j value and store it in JOBS.
set_jobs() {
    if [[ ! "$1" =~ ^[0-9]+$ ]]; then
        die "Invalid number of jobs: $1"
    fi
    JOBS="$1"
}

# on_off <true|false>: print the CMake spelling of a script boolean.
on_off() {
    if [[ "$1" == true ]]; then
        echo ON
    else
        echo OFF
    fi
}

validate_lib_type() {
    if [[ "$1" != static && "$1" != shared ]]; then
        die "Invalid --lib-type: $1 (expected static or shared)"
    fi
}

# validate_deployment_target <version> <os-name> <examples>
validate_deployment_target() {
    if [[ ! "$1" =~ ^[0-9]+\.[0-9]+$ ]]; then
        log_error "Invalid $2 deployment target format: $1"
        die "Expected format: X.Y (e.g., $3)"
    fi
}

# resolve_tests_and_samples <library_only> <dev> <ide_project> <no_tests> <no_samples>
# Sets WITH_TESTS / WITH_SAMPLES. Precedence: --library-only, then --dev, then an
# IDE project (Xcode) turning both on; --no-tests / --no-samples always win.
resolve_tests_and_samples() {
    if [[ "$1" == true ]]; then
        WITH_TESTS=false
        WITH_SAMPLES=false
    elif [[ "$2" == true || "$3" == true ]]; then
        WITH_TESTS=true
        WITH_SAMPLES=true
    fi
    if [[ "$4" == true ]]; then
        WITH_TESTS=false
    fi
    if [[ "$5" == true ]]; then
        WITH_SAMPLES=false
    fi
}

#------------------------------------------------------------------------------
# Interactive prompts. Closed stdin aborts instead of silently using defaults.
#------------------------------------------------------------------------------

# ask_yes_no <question> <Y|N>: succeeds on yes; empty or other input takes the default.
ask_yes_no() {
    local hint="y/N" reply
    if [[ "$2" == Y ]]; then
        hint="Y/n"
    fi
    if ! read -r -p "$1 (${hint}): " reply; then
        die "No input (stdin closed)"
    fi
    case "$reply" in
        [Yy]) return 0 ;;
        [Nn]) return 1 ;;
    esac
    [[ "$2" == Y ]]
}

# ask_menu <title> <default-index> <item>...: numbered menu; stores the chosen
# 1-based index in MENU_CHOICE (the default for empty or invalid input).
ask_menu() {
    local title="$1" default="$2"
    shift 2
    local count=$# index=1 item reply
    echo ""
    echo "$title"
    for item in "$@"; do
        echo "${index}) ${item}"
        index=$((index + 1))
    done
    if ! read -r -p "Enter choice (1-${count}) [${default}]: " reply; then
        die "No input (stdin closed)"
    fi
    MENU_CHOICE="$default"
    # ^[1-9]: a leading zero would make (( )) parse the reply as octal.
    if [[ "$reply" =~ ^[1-9][0-9]*$ ]] && ((reply <= count)); then
        MENU_CHOICE="$reply"
    fi
}

ask_build_type() {
    ask_menu "Select Build Type:" 1 \
        "Debug (default) - Development with debug symbols" \
        "Release - Optimized for production" \
        "RelWithDebInfo - Release with debug info" \
        "MinSizeRel - Minimum size release"
    case "$MENU_CHOICE" in
        2) BUILD_TYPE="Release" ;;
        3) BUILD_TYPE="RelWithDebInfo" ;;
        4) BUILD_TYPE="MinSizeRel" ;;
        *) BUILD_TYPE="Debug" ;;
    esac
}

ask_lib_type() {
    ask_menu "Select library type:" 1 \
        "shared (default) - shared library" \
        "static - static library"
    case "$MENU_CHOICE" in
        2) LIB_TYPE="static" ;;
        *) LIB_TYPE="shared" ;;
    esac
}

ask_action() {
    ask_menu "Select Action:" 2 \
        "Configure only" \
        "Configure and build (default)" \
        "Configure, build, and test"
    case "$MENU_CHOICE" in
        1) ACTION="configure" ;;
        3) ACTION="test" ;;
        *) ACTION="configure_and_build" ;;
    esac
}

#------------------------------------------------------------------------------
# Toolchain
#------------------------------------------------------------------------------

# require_command <command> <message>
require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        die "$2"
    fi
}

# clang_version: "17.0.0" from any clang banner (Apple, Homebrew, Ubuntu, upstream).
clang_version() {
    clang --version | sed -nE '1s/.*version ([0-9][0-9.]*).*/\1/p'
}

# gcc_version: 4th field of the banner, e.g. "gcc (Ubuntu 13.2.0-23ubuntu4) 13.2.0".
gcc_version() {
    gcc --version | awk 'NR==1 {print $4}'
}

# note_vendored_dawn: the WebGPU backend's CMakeLists falls back to FetchContent at
# the pinned release tag (VNE_RHI_DAWN_GIT_TAG) when deps/external/dawn is absent.
# Say so rather than fail: the first fetch is slow.
note_vendored_dawn() {
    if [[ ! -d "${PROJECT_ROOT}/deps/external/dawn" ]]; then
        echo "NOTE: no vendored Dawn at ${PROJECT_ROOT}/deps/external/dawn"
        echo "      CMake will fetch Dawn at the pinned tag; the first configure takes a while."
        echo "      To use a local tree instead, pass -DVNE_RHI_DAWN_DIR=/path/to/dawn."
    fi
}

# check_xcode_sdks <os-name> <device-sdk> <simulator-sdk>
check_xcode_sdks() {
    local os_name="$1" device_sdk="$2" simulator_sdk="$3" sdks
    echo "=== Checking ${os_name} Development Environment ==="
    require_command xcodebuild "Xcode not found. Please install Xcode from the App Store."
    echo "Found: $(xcodebuild -version | sed -n 1p)"
    # Read the list first: grep -q on a pipe can SIGPIPE xcodebuild under pipefail.
    sdks="$(xcodebuild -showsdks)"
    if ! grep -q "$device_sdk" <<<"$sdks"; then
        die "${os_name} SDK not found (${device_sdk}). Please install it in Xcode."
    fi
    if ! grep -q "$simulator_sdk" <<<"$sdks"; then
        die "${os_name} Simulator SDK not found (${simulator_sdk}). Please install it in Xcode."
    fi
    echo "${os_name} development environment check passed."
    echo ""
}

# show_xcode_sdk_path <label> <sdk>
show_xcode_sdk_path() {
    local path
    path="$(xcrun --sdk "$2" --show-sdk-path 2>/dev/null || true)"
    if [[ -z "$path" ]]; then
        die "$1 not found ($2)."
    fi
    echo "Using $1: ${path}"
}

#------------------------------------------------------------------------------
# CMake / Xcode steps
#------------------------------------------------------------------------------

# cmake_configure <build-dir> <generator|""> <cmake-arg>...
cmake_configure() {
    local build_dir="$1" generator="$2"
    shift 2
    local generator_args=()
    if [[ -n "$generator" ]]; then
        generator_args=(-G "$generator")
    fi
    echo "Configuring CMake (${generator:-default generator}) in: ${build_dir}"
    echo "CMake arguments: $*"
    cmake -S "$PROJECT_ROOT" -B "$build_dir" ${generator_args[@]+"${generator_args[@]}"} "$@"
}

# cmake_build <build-dir> [cmake --build arg]...   (uses JOBS)
cmake_build() {
    local build_dir="$1"
    shift
    echo "Building with ${JOBS} parallel jobs in: ${build_dir}"
    cmake --build "$build_dir" "$@" --parallel "$JOBS"
}

# run_ctest <build-dir> [ctest arg]...
run_ctest() {
    local build_dir="$1"
    shift
    echo "Running tests with CTest in: ${build_dir}"
    ctest --test-dir "$build_dir" "$@" --output-on-failure
}

# xcode_build <build-dir> <configuration> [sdk]   (uses JOBS)
xcode_build() {
    local build_dir="$1" configuration="$2" sdk="${3:-}"
    local projects=() sdk_args=()
    shopt -s nullglob
    projects=("${build_dir}"/*.xcodeproj)
    shopt -u nullglob
    if [[ ${#projects[@]} -eq 0 ]]; then
        die "No Xcode project found in ${build_dir}"
    fi
    if [[ -n "$sdk" ]]; then
        sdk_args=(-sdk "$sdk")
    fi
    echo "Building with Xcode in: ${build_dir}"
    (
        cd "$build_dir"
        xcodebuild -project "${projects[0]##*/}" -scheme ALL_BUILD -configuration "$configuration" \
            ${sdk_args[@]+"${sdk_args[@]}"} -jobs "$JOBS"
    )
}

# xcode_configuration <build-type>: the Xcode configuration for a CMake build type.
xcode_configuration() {
    case "$1" in
        Release | RelWithDebInfo | MinSizeRel) echo "$1" ;;
        *) echo "Debug" ;;
    esac
}

#------------------------------------------------------------------------------
# Output
#------------------------------------------------------------------------------

# clean_dirs <dir>...
clean_dirs() {
    local dir
    for dir in "$@"; do
        echo "Cleaning: ${dir}"
        rm -rf "$dir"
    done
}

# print_kv <key> <value>: one aligned line of a configuration summary.
print_kv() {
    printf '%-18s: %s\n' "$1" "$2"
}

print_success_banner() {
    echo ""
    echo "=== Build completed successfully ==="
}
