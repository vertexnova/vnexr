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

# C:/... form so Git Bash tests, Windows CMake, and local.properties agree.
to_tool_path() {
    local path="$1"
    [[ -n "$path" ]] || return 0
    if command -v cygpath >/dev/null 2>&1; then
        cygpath -m "$path"
    else
        echo "$path"
    fi
}

is_exec() {
    local path="$1"
    # Git Bash does not mark .bat files executable, and Windows tools are often *.exe.
    [[ -e "$path" || -e "${path}.exe" || -e "${path}.bat" || -e "${path}.cmd" ]]
}

resolve_sdk_root() {
    local candidate
    if [[ -n "${ANDROID_HOME:-}" && -d "${ANDROID_HOME}" ]]; then to_tool_path "$ANDROID_HOME"; return 0; fi
    if [[ -n "${ANDROID_SDK_ROOT:-}" && -d "${ANDROID_SDK_ROOT}" ]]; then to_tool_path "$ANDROID_SDK_ROOT"; return 0; fi
    if [[ -d "$HOME/Android/Sdk" ]]; then to_tool_path "$HOME/Android/Sdk"; return 0; fi
    # Android Studio's default SDK on macOS.
    if [[ -d "$HOME/Library/Android/sdk" ]]; then to_tool_path "$HOME/Library/Android/sdk"; return 0; fi
    # Android Studio's default SDK on Windows.
    if [[ -n "${LOCALAPPDATA:-}" ]]; then
        candidate="$(to_tool_path "${LOCALAPPDATA}/Android/Sdk")"
        if [[ -d "$candidate" ]]; then echo "$candidate"; return 0; fi
    fi
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
    if [[ -n "${GRADLE_DIR:-}" ]] && is_exec "${GRADLE_DIR}/gradlew"; then
        if [[ -x "${GRADLE_DIR}/gradlew" ]]; then echo "${GRADLE_DIR}/gradlew"; else echo "${GRADLE_DIR}/gradlew.bat"; fi
        return 0
    fi
    if command -v gradle >/dev/null 2>&1; then command -v gradle; return 0; fi
    local wrapper_gradle
    wrapper_gradle="$(find "${HOME}/.gradle/wrapper/dists" \( -path "*/gradle-*/bin/gradle" -o -path "*/gradle-*/bin/gradle.bat" -o -path "*/gradle-*/bin/gradle.cmd" \) -type f 2>/dev/null | sort -V | tail -n 1 || true)"
    if [[ -n "$wrapper_gradle" ]] && is_exec "$wrapper_gradle"; then echo "$wrapper_gradle"; return 0; fi
    return 1
}

# Major version of the JDK at $1 ("javac 17.0.9" -> 17, "javac 1.8.0_392" -> 8), or 0 when it cannot be read.
jdk_major_version() {
    local javac_out major
    javac_out="$("$1/bin/javac" -version 2>&1 | head -n 1 | tr -d '\r' || true)"
    major="$(sed -n 's/^javac \([0-9][0-9]*\)\(\.\([0-9][0-9]*\)\)\{0,1\}.*/\1 \3/p' <<<"$javac_out")"
    set -- $major
    if [[ "${1:-}" == "1" ]]; then echo "${2:-0}"; else echo "${1:-0}"; fi
}

# True for macOS's /usr/bin/javac, a stub that exists even when no JDK is installed. On Linux it is a real compiler.
is_macos_javac_stub() {
    [[ "$(uname -s)" == "Darwin" && "$1" == "/usr/bin/javac" ]]
}

resolve_java_home() {
    if [[ -n "${JAVA_HOME:-}" ]] && is_exec "${JAVA_HOME}/bin/javac"; then to_tool_path "$JAVA_HOME"; return 0; fi
    local studio_jbr program_files="" program_files_x86=""
    local candidates=()
    if [[ -n "${ProgramFiles:-}" ]]; then
        program_files="$(to_tool_path "$ProgramFiles")"
    elif [[ -n "${PROGRAMFILES:-}" ]]; then
        program_files="$(to_tool_path "$PROGRAMFILES")"
    fi
    program_files_x86="$(printenv 'ProgramFiles(x86)' 2>/dev/null || true)"
    if [[ -n "$program_files_x86" ]]; then
        program_files_x86="$(to_tool_path "$program_files_x86")"
    fi
    # macOS: the system picker knows installed JDKs; /usr/bin/java is only a stub without one.
    if [[ "$(uname -s)" == "Darwin" ]]; then
        local mac_jdk
        # -F makes a missing match an error: without it java_home silently returns the default JDK, whatever version.
        mac_jdk="$(/usr/libexec/java_home -F -v 17+ 2>/dev/null || true)"
        if [[ -n "$mac_jdk" ]] && is_exec "${mac_jdk}/bin/javac" && (( $(jdk_major_version "$mac_jdk") >= 17 )); then
            echo "$mac_jdk"
            return 0
        fi
    fi
    candidates=(
        /snap/android-studio/current/jbr
        "$HOME/android-studio/jbr"
        /opt/android-studio/jbr
        "/Applications/Android Studio.app/Contents/jbr/Contents/Home"
        /opt/homebrew/opt/openjdk@17/libexec/openjdk.jdk/Contents/Home
        /usr/local/opt/openjdk@17/libexec/openjdk.jdk/Contents/Home
    )
    if [[ -n "$program_files" ]]; then
        candidates+=("${program_files}/Android/Android Studio/jbr")
    fi
    if [[ -n "$program_files_x86" ]]; then
        candidates+=("${program_files_x86}/Android/Android Studio/jbr")
    fi
    if [[ -n "${LOCALAPPDATA:-}" ]]; then
        candidates+=("$(to_tool_path "${LOCALAPPDATA}/Programs/Android Studio/jbr")")
    fi
    for studio_jbr in "${candidates[@]}"; do
        if [[ -n "$studio_jbr" ]] && is_exec "${studio_jbr}/bin/javac"; then
            to_tool_path "$studio_jbr"
            return 0
        fi
    done
    # Last resort: a javac on PATH. Resolve symlinks so /usr/bin/javac on Linux maps to the real JDK home
    # (/usr/lib/jvm/...), not /usr; skip macOS's stub, which exists even when no JDK is installed.
    if command -v javac >/dev/null 2>&1 && ! is_macos_javac_stub "$(command -v javac)"; then
        local javac_path
        javac_path="$(command -v javac)"
        javac_path="$(readlink -f "$javac_path" 2>/dev/null || echo "$javac_path")"
        to_tool_path "$(dirname "$(dirname "$javac_path")")"
        return 0
    fi
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
    if [[ -n "$sdk_root" && -x "${sdk_root}/platform-tools/adb.exe" ]]; then
        echo "${sdk_root}/platform-tools/adb.exe"
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
if [[ -n "$SDK_ROOT" ]]; then
    export ANDROID_HOME="$SDK_ROOT"
    export ANDROID_SDK_ROOT="$SDK_ROOT"
fi
if [[ -n "$NDK_ROOT" ]]; then
    export ANDROID_NDK_ROOT="$NDK_ROOT"
    export ANDROID_NDK_HOME="$NDK_ROOT"
fi

if [[ -z "$NDK_ROOT" ]]; then
    die "Android NDK not found. Set ANDROID_NDK_ROOT or install via sdkmanager."
fi

NDK_VERSION="$(basename "$NDK_ROOT")"
if [[ -n "${WINDIR:-}" || -n "${MSYSTEM:-}" ]]; then
    # AGP writes object files under <build>/android-studio/app/.cxx/.../CMakeFiles.
    # The full NDK build-dir name pushes those paths past the Windows MAX_PATH limit, so keep it short, but
    # still distinct per ABI, build type, library type and NDK major: a build-only action must never reuse the
    # cached configuration of another combination.
    BUILD_DIR="$PROJECT_ROOT/build/android/${ABI}-$(lowercase "$BUILD_TYPE")-${LIB_TYPE}-ndk${NDK_VERSION%%.*}"
else
    BUILD_DIR="$PROJECT_ROOT/build/${LIB_TYPE}/${BUILD_TYPE}/build-android-ndk-${NDK_VERSION}-${ABI}"
fi
GRADLE_PROJECT_DIR="${BUILD_DIR}/android-studio"
SAMPLE_MANIFEST="${GRADLE_PROJECT_DIR}/vne_samples.txt"
TOOLCHAIN_FILE="${NDK_ROOT}/build/cmake/android.toolchain.cmake"

# Windows CMake defaults to Visual Studio, which cannot use the NDK toolchain.
# The Android SDK CMake package ships Ninja beside cmake.exe.
# VneXR needs CMake 3.26 or newer; Android Studio's default SDK CMake (3.22.1) is older and must not shadow a
# suitable CMake that is already on PATH.
is_cmake_version_ok() {
    [[ "$(printf '%s\n%s\n' "3.26" "$1" | sort -V | head -n 1)" == "3.26" ]]
}

if [[ -n "${WINDIR:-}" || -n "${MSYSTEM:-}" ]]; then
    if [[ -n "$SDK_ROOT" && -d "${SDK_ROOT}/cmake" ]]; then
        suitable_sdk_cmake=""
        newest_sdk_cmake=""
        while IFS= read -r sdk_cmake_dir; do
            [[ -d "${sdk_cmake_dir}/bin" ]] || continue
            newest_sdk_cmake="$sdk_cmake_dir"
            if is_cmake_version_ok "$(basename "$sdk_cmake_dir")"; then suitable_sdk_cmake="$sdk_cmake_dir"; fi
        done < <(ls -1d "${SDK_ROOT}/cmake/"* 2>/dev/null | sort -V || true)
        if [[ -n "$suitable_sdk_cmake" ]]; then
            export PATH="${suitable_sdk_cmake}/bin:${PATH}"
        elif [[ -n "$newest_sdk_cmake" ]]; then
            # Too old to use as CMake, but its folder is where the SDK keeps Ninja: append so PATH's CMake wins.
            log_warn "Android SDK CMake $(basename "$newest_sdk_cmake") is older than 3.26; install 'cmake;3.31.6' with sdkmanager"
            export PATH="${PATH}:${newest_sdk_cmake}/bin"
        fi
    fi
fi

CMAKE_ARGS=()
if [[ -n "${WINDIR:-}" || -n "${MSYSTEM:-}" ]]; then
    CMAKE_ARGS+=("-G" "Ninja")
fi
CMAKE_ARGS+=(
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
    local sdk_dir
    sdk_dir="$(to_tool_path "$SDK_ROOT")"
    echo "sdk.dir=${sdk_dir}" >"${GRADLE_PROJECT_DIR}/local.properties"
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

# Run Gradle with the given arguments. Git Bash drops arguments when it launches a Windows .bat directly, so go
# through cmd. cmd /c strips the first and last quote of a command that starts with one, hence the doubled outer
# quotes: they keep a Gradle path that contains spaces intact.
run_gradle() {
    local gradle="$1"
    shift
    if [[ "$gradle" == *.bat || "$gradle" == *.cmd ]]; then
        local gradle_win
        gradle_win="$(cygpath -w "$gradle")"
        cmd //c "\"\"${gradle_win}\" $*\""
    else
        "$gradle" "$@"
    fi
}

# The generated project uses Android Gradle Plugin 9.0.1, which refuses to run on Gradle older than 9.1.0.
readonly MIN_GRADLE_VERSION="9.1.0"
require_gradle_version() {
    local gradle="$1" version
    version="$(run_gradle "$gradle" --version 2>/dev/null | tr -d '\r' | sed -n 's/^Gradle \([0-9][0-9.]*\).*/\1/p' | head -n 1 || true)"
    if [[ -z "$version" ]]; then
        log_warn "Could not read the Gradle version from '$gradle'; the build needs Gradle ${MIN_GRADLE_VERSION} or newer"
        return 0
    fi
    if [[ "$(printf '%s\n%s\n' "$MIN_GRADLE_VERSION" "$version" | sort -V | head -n 1)" != "$MIN_GRADLE_VERSION" ]]; then
        die "Gradle $version is too old: Android Gradle Plugin 9.0.1 needs ${MIN_GRADLE_VERSION} or newer ($gradle). Install a newer Gradle (for example 'brew install gradle'), or set GRADLE_DIR to a project with a newer wrapper."
    fi
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

    (
        cd "$GRADLE_PROJECT_DIR"
        export JAVA_HOME="$java_home"
        export PATH="$java_home/bin:$PATH"
        require_gradle_version "$gradle"
        run_gradle "$gradle" "assemble${variant_suffix}"
    )
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
