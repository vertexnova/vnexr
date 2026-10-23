#!/usr/bin/env bash

#==============================================================================
# vnexr Android Vulkan / device smoke check before debugging OpenXR APKs.
#==============================================================================

set -Eeuo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/common.sh"

PKG="${1:-com.vertexnova.vnexr.openxr_stereo}"

require_command adb "adb not found. Install Android SDK platform-tools."

echo "=== Android Vulkan / XR environment ==="
adb wait-for-device
echo "API level: $(adb shell getprop ro.build.version.sdk | tr -d '\r')"
echo "ABI: $(adb shell getprop ro.product.cpu.abi | tr -d '\r')"
echo "Model: $(adb shell getprop ro.product.model | tr -d '\r')"
echo ""
echo "ICD files on device:"
adb shell 'ls -la /vendor/lib64/hw/vulkan*.so 2>/dev/null || echo "(none)"'
echo ""
echo "gpu vkjson (empty {} = no Vulkan devices):"
vkjson="$(adb shell cmd gpu vkjson 2>/dev/null || true)"
sed -n 1,20p <<<"${vkjson}"
echo ""
if adb shell pm path "$PKG" >/dev/null 2>&1; then
    echo "Package $PKG is installed."
    echo "Recent log lines:"
    adb logcat -d -s vne.xr:* OpenXR:* 2>/dev/null | tail -40 || true
else
    echo "Package $PKG not installed."
    echo "Run: ./scripts/build_android.sh -a install -sample sample_02_openxr_stereo -abi arm64-v8a"
fi
