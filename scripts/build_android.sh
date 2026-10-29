#!/usr/bin/env bash

#==============================================================================
# VneXR Android Build Script
#
# Configure with the NDK, generate an Android Studio Gradle project from
# the vnexr_samples launcher, then build / install / run the APK (Quest 3, Galaxy XR: arm64-v8a).
# One APK holds every sample; the sample to run is chosen at launch (-sample, or the "sample" intent extra).
#==============================================================================

set -Eeuo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/common.sh"
enable_error_trap

BUILD_TYPE="Debug"
ACTION="configure_and_build"
LIB_TYPE="static"
JOBS=10
CLEAN=false
ABI="arm64-v8a"
API_LEVEL="24"
SAMPLE=""
WITH_DEV=true
WITH_TESTS=false
WITH_SAMPLES=true

usage() {
    echo "Usage: $0 [-t Debug|Release] [-a configure|build|configure_and_build|apk|install|run|studio|clean]"
    echo "          [-abi arm64-v8a|x86_64] [-api 24] [-sample <name>] [-l static|shared]"
    echo "          [-clean] [-j N] [--dev] [--with-tests] [--no-samples]"
    echo ""
    echo "Defaults: OpenXR+Vulkan ON, samples ON, ABI arm64-v8a (Quest 3, Galaxy XR)."
    echo "  -sample <name>   Sample to launch with -a run (see: vnexr_samples --list). Default: the first registered."
    echo ""
    echo "Environment: ANDROID_HOME / ANDROID_SDK_ROOT, ANDROID_NDK_ROOT / ANDROID_NDK_HOME"
    echo ""
    echo "Examples:"
    echo "  $0 -a studio --dev -abi arm64-v8a"
    echo "  $0 -t Debug -a install -abi arm64-v8a"
    echo "  $0 -t Debug -a run -sample stereo_cube -abi arm64-v8a"
    exit 1
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -t | --type | --build-type) require_value "$1" "${2:-}"; BUILD_TYPE="$2"; shift 2 ;;
        -a | --action) require_value "$1" "${2:-}"; ACTION="$2"; shift 2 ;;
        -l | --lib-type) require_value "$1" "${2:-}"; LIB_TYPE="$2"; shift 2 ;;
        -j | --jobs) require_value "$1" "${2:-}"; set_jobs "$2"; shift 2 ;;
        -j*) set_jobs "${1#-j}"; shift ;;
        -abi | --abi) require_value "$1" "${2:-}"; ABI="$2"; shift 2 ;;
        -api | --api) require_value "$1" "${2:-}"; API_LEVEL="$2"; shift 2 ;;
        -sample | --sample) require_value "$1" "${2:-}"; SAMPLE="$2"; shift 2 ;;
        -clean | --clean) CLEAN=true; shift ;;
        --dev) WITH_DEV=true; WITH_SAMPLES=true; shift ;;
        --with-samples | --with-examples) WITH_SAMPLES=true; shift ;;
        --with-tests) WITH_TESTS=true; shift ;;
        --no-samples | --no-examples) WITH_SAMPLES=false; shift ;;
        --no-tests) WITH_TESTS=false; shift ;;
        -h | --help) usage ;;
        *) log_error "Unknown option: $1"; usage ;;
    esac
done

if [[ "$ACTION" == studio || "$ACTION" == apk || "$ACTION" == install || "$ACTION" == run ]] && [[ "$WITH_SAMPLES" != true ]]; then
    die "action '$ACTION' requires samples (omit --no-samples)"
fi
if [[ "$ABI" != arm64-v8a && "$ABI" != x86_64 ]]; then
    die "unsupported ABI '$ABI' (expected arm64-v8a or x86_64)"
fi
validate_lib_type "$LIB_TYPE"

resolve_sdk_root() {
    if [[ -n "${ANDROID_HOME:-}" ]]; then echo "$ANDROID_HOME"; return 0; fi
    if [[ -n "${ANDROID_SDK_ROOT:-}" ]]; then echo "$ANDROID_SDK_ROOT"; return 0; fi
    if [[ -d "$HOME/Android/Sdk" ]]; then echo "$HOME/Android/Sdk"; return 0; fi
    return 1
}

resolve_ndk_root() {
    local root
    for root in "${ANDROID_NDK_ROOT:-}" "${ANDROID_NDK_HOME:-}"; do
        if [[ -n "$root" && -f "${root}/build/cmake/android.toolchain.cmake" ]]; then
            echo "$root"
            return 0
        fi
    done
    local sdk_root latest_ndk
    sdk_root="$(resolve_sdk_root || true)"
    if [[ -n "$sdk_root" ]]; then
        latest_ndk="$(ls -1d "${sdk_root}/ndk/"* 2>/dev/null | sort -V | tail -n 1 || true)"
        if [[ -n "$latest_ndk" && -f "${latest_ndk}/build/cmake/android.toolchain.cmake" ]]; then
            echo "$latest_ndk"
            return 0
        fi
    fi
    return 1
}

resolve_gradle() {
    if [[ -n "${GRADLE_DIR:-}" && -x "${GRADLE_DIR}/gradlew" ]]; then echo "${GRADLE_DIR}/gradlew"; return 0; fi
    if command -v gradle >/dev/null 2>&1; then echo "gradle"; return 0; fi
    local wrapper_gradle
    wrapper_gradle="$(find "${HOME}/.gradle/wrapper/dists" -path "*/gradle-*/bin/gradle" -type f 2>/dev/null | sort -V | tail -n 1 || true)"
    if [[ -n "$wrapper_gradle" && -x "$wrapper_gradle" ]]; then echo "$wrapper_gradle"; return 0; fi
    return 1
}

resolve_java_home() {
    if [[ -n "${JAVA_HOME:-}" && -x "${JAVA_HOME}/bin/javac" ]]; then echo "$JAVA_HOME"; return 0; fi
    local studio_jbr
    for studio_jbr in \
        /snap/android-studio/current/jbr \
        "$HOME/android-studio/jbr" \
        /opt/android-studio/jbr \
        "/Applications/Android Studio.app/Contents/jbr/Contents/Home"; do
        if [[ -d "$studio_jbr" && -x "${studio_jbr}/bin/javac" ]]; then
            echo "$studio_jbr"
            return 0
        fi
    done
    if command -v javac >/dev/null 2>&1; then dirname "$(dirname "$(command -v javac)")"; return 0; fi
    return 1
}

resolve_adb() {
    if command -v adb >/dev/null 2>&1; then command -v adb; return 0; fi
    local sdk_root
    sdk_root="$(resolve_sdk_root || true)"
    if [[ -n "$sdk_root" && -x "${sdk_root}/platform-tools/adb" ]]; then
        echo "${sdk_root}/platform-tools/adb"
        return 0
    fi
    return 1
}

resolve_studio() {
    if command -v android-studio >/dev/null 2>&1; then echo "android-studio"; return 0; fi
    if [[ -x "/opt/android-studio/bin/studio.sh" ]]; then echo "/opt/android-studio/bin/studio.sh"; return 0; fi
    if [[ -x "$HOME/android-studio/bin/studio.sh" ]]; then echo "$HOME/android-studio/bin/studio.sh"; return 0; fi
    return 1
}

capitalize() {
    echo "$(tr '[:lower:]' '[:upper:]' <<<"${1:0:1}")${1:1}"
}

lowercase() {
    tr '[:upper:]' '[:lower:]' <<<"$1"
}

SDK_ROOT="$(resolve_sdk_root || true)"
NDK_ROOT="$(resolve_ndk_root || true)"

if [[ -z "$NDK_ROOT" ]]; then
    die "Android NDK not found. Set ANDROID_NDK_ROOT or install via sdkmanager."
fi

NDK_VERSION="$(basename "$NDK_ROOT")"
BUILD_DIR="$PROJECT_ROOT/build/${LIB_TYPE}/${BUILD_TYPE}/build-android-ndk-${NDK_VERSION}-${ABI}"
GRADLE_PROJECT_DIR="${BUILD_DIR}/android-studio"
SAMPLE_MANIFEST="${GRADLE_PROJECT_DIR}/vne_samples.txt"
TOOLCHAIN_FILE="${NDK_ROOT}/build/cmake/android.toolchain.cmake"

CMAKE_ARGS=(
    "-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}"
    "-DANDROID_ABI=${ABI}"
    "-DANDROID_PLATFORM=android-${API_LEVEL}"
    "-DANDROID_STL=c++_shared"
    "-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON"
    "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
    "-DVNE_XR_LIB_TYPE=${LIB_TYPE}"
    "-DVNE_XR_DEV=$(on_off "$WITH_DEV")"
    "-DVNE_XR_TESTS=$(on_off "$WITH_TESTS")"
    "-DVNE_XR_SAMPLES=$(on_off "$WITH_SAMPLES")"
    "-DVNE_XR_EXAMPLES=$(on_off "$WITH_SAMPLES")"
    "-DVNE_TARGET_PLATFORM=Android"
    "-DVNE_XR_WITH_OPENXR=ON"
    "-DVNE_XR_WITH_RHI=ON"
    "-DVNE_XR_BACKEND_VULKAN=ON"
    "-DVNE_XR_GENERATE_ANDROID_GRADLE=ON"
    "-DCMAKE_CXX_STANDARD=20"
    "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
)

# The generated manifest lists one line per APK flavor (name,flavor,package,label); vnexr produces a single one.
load_sample_entries() {
    if [[ ! -f "$SAMPLE_MANIFEST" ]]; then
        die "sample manifest not found at $SAMPLE_MANIFEST. Re-run configure so CMake can generate Android Studio files."
    fi
    SAMPLE_ENTRIES="$(awk -F, 'NF >= 4 && $1 !~ /^#/ {print $1","$2","$3","$4}' "$SAMPLE_MANIFEST")"
    [[ -n "$SAMPLE_ENTRIES" ]] || die "no Android flavor listed in $SAMPLE_MANIFEST"
}

write_local_properties() {
    if [[ -z "$SDK_ROOT" ]]; then
        log_warn "Android SDK not found; Android Studio may prompt for SDK path"
        return 0
    fi
    echo "sdk.dir=${SDK_ROOT}" >"${GRADLE_PROJECT_DIR}/local.properties"
}

configure_cmake() {
    if [[ "$CLEAN" == true ]]; then
        rm -rf "$BUILD_DIR"
    fi
    mkdir -p "$BUILD_DIR"
    cmake_configure "$BUILD_DIR" "" "${CMAKE_ARGS[@]}"
}

build_native() {
    cmake_build "$BUILD_DIR"
}

build_apk() {
    local gradle java_home
    gradle="$(resolve_gradle || true)"
    [[ -n "$gradle" ]] || die "Gradle not found. Use -a studio or install Gradle."
    java_home="$(resolve_java_home || true)"
    [[ -n "$java_home" ]] || die "JDK with javac not found. Set JAVA_HOME."

    write_local_properties
    local variant_suffix build_type_lower
    variant_suffix="$(capitalize "$BUILD_TYPE")"
    build_type_lower="$(lowercase "$BUILD_TYPE")"

    (cd "$GRADLE_PROJECT_DIR" && JAVA_HOME="$java_home" PATH="$java_home/bin:$PATH" "$gradle" "assemble${variant_suffix}")
    load_sample_entries
    local line flavor
    while IFS= read -r line; do
        [[ -z "$line" ]] && continue
        IFS=',' read -r _ flavor _ _ <<<"$line"
        echo "APK: ${GRADLE_PROJECT_DIR}/app/build/outputs/apk/${flavor}/${build_type_lower}/app-${flavor}-${build_type_lower}.apk"
    done <<<"$SAMPLE_ENTRIES"
}

install_apk() {
    local adb_bin
    adb_bin="$(resolve_adb || true)"
    [[ -n "$adb_bin" ]] || die "adb not found. Install Android SDK platform-tools."
    if ! "$adb_bin" devices | awk 'NR>1 && $2=="device" {found=1} END {exit !found}'; then
        die "No Android device connected (Quest 3 or Galaxy XR in developer mode)."
    fi
    build_apk
    load_sample_entries
    local line name flavor package label build_type_lower apk
    build_type_lower="$(lowercase "$BUILD_TYPE")"
    while IFS= read -r line; do
        [[ -z "$line" ]] && continue
        IFS=',' read -r name flavor package label <<<"$line"
        apk="${GRADLE_PROJECT_DIR}/app/build/outputs/apk/${flavor}/${build_type_lower}/app-${flavor}-${build_type_lower}.apk"
        [[ -f "$apk" ]] || die "APK missing: $apk"
        echo "Installing $label ($package)"
        "$adb_bin" install -r "$apk"
    done <<<"$SAMPLE_ENTRIES"
    echo "Installed. Launch from the headset library, or run: $0 -a run [-sample <name>]"
}

# Install, then start the activity with the sample name as an intent extra and show how to follow the log.
run_apk() {
    install_apk
    local adb_bin line package
    adb_bin="$(resolve_adb)"
    IFS=',' read -r _ _ package _ <<<"$(head -n 1 <<<"$SAMPLE_ENTRIES")"
    local start_args=(shell am start -n "${package}/com.vertexnova.vnexr.samples.VneXrNativeActivity")
    if [[ -n "$SAMPLE" ]]; then
        start_args+=(-e sample "$SAMPLE")
    fi
    "$adb_bin" "${start_args[@]}"
    echo "Started ${package}${SAMPLE:+ with sample '$SAMPLE'}. Follow the log: adb logcat -s vne.xr:* OpenXR:*"
}

open_studio() {
    configure_cmake
    write_local_properties
    local studio
    studio="$(resolve_studio || true)"
    if [[ -n "$studio" ]]; then
        "$studio" "$GRADLE_PROJECT_DIR" &
    else
        log_ok "Gradle project ready at $GRADLE_PROJECT_DIR - open it in Android Studio."
    fi
}

case "$ACTION" in
    configure) configure_cmake ;;
    build) build_native ;;
    configure_and_build) configure_cmake; build_native ;;
    apk) configure_cmake; build_apk ;;
    install) configure_cmake; install_apk ;;
    run) configure_cmake; run_apk ;;
    studio) open_studio ;;
    clean)
        if [[ -d "$BUILD_DIR" ]]; then
            cmake --build "$BUILD_DIR" --target clean 2>/dev/null || true
            log_ok "Cleaned $BUILD_DIR"
        else
            echo "Nothing to clean"
        fi
        ;;
    *) usage ;;
esac

echo "=== Build completed ==="
echo "Native build: $BUILD_DIR"
echo "Gradle project: $GRADLE_PROJECT_DIR"
