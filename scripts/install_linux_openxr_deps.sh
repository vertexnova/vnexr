#!/usr/bin/env bash
# Install Linux packages needed to build and run vnexr with OpenXR + Vulkan.
#
# Usage:
#   ./scripts/install_linux_openxr_deps.sh
#
# Tuned for Ubuntu 24.04 (Noble). For other distros install equivalents manually:
#   Vulkan SDK / libvulkan-dev, X11 or Wayland dev headers (OpenXR loader WSI),
#   Monado OpenXR runtime for desktop simulation.
set -euo pipefail

if ! command -v apt-get >/dev/null 2>&1; then
  echo "This script supports apt-based distros (Debian/Ubuntu)."
  echo "Install manually: cmake, g++ or clang, libvulkan-dev, libx11-dev,"
  echo "  libx11-xcb-dev, libxrandr-dev, libxxf86vm-dev, libxcb1-dev, libxcb-glx0-dev,"
  echo "  and an OpenXR runtime (Monado)."
  exit 1
fi

echo ">> Installing Linux OpenXR + Vulkan build dependencies (sudo required)"
# Khronos OpenXR-SDK loader includes <xcb/glx.h> via xr_dependencies.h on Linux.
sudo apt-get update -qq
sudo apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  pkg-config \
  libvulkan-dev \
  vulkan-tools \
  libx11-dev \
  libx11-xcb-dev \
  libxrandr-dev \
  libxxf86vm-dev \
  libgl1-mesa-dev \
  libxcb1-dev \
  libxcb-glx0-dev

echo ">> Installing OpenXR runtime packages (Ubuntu 24.04 names)"
# On Noble these packages provide the Monado service and a system OpenXR loader.
# Build-time loader still comes from FetchContent (cmake/VneXrOpenXR.cmake).
sudo apt-get install -y \
  monado-service \
  libopenxr-loader1 \
  openxr-layer-corevalidation \
  || {
    echo "WARNING: Monado/OpenXR runtime packages missing or renamed on this distro."
    echo "  Try: apt-cache search monado | head"
    echo "  Or build Monado from https://monado.freedesktop.org/"
  }

echo ">> Done. Verify Vulkan:"
if command -v vulkaninfo >/dev/null 2>&1; then
  vulkaninfo --summary 2>/dev/null | head -20 || echo "vulkaninfo: no GPU/runtime (build may still succeed)"
else
  echo "vulkaninfo not found after install"
fi

echo ""
echo "Monado desktop simulation (no headset):"
echo "  1. Start the service:  systemctl --user start monado.service"
echo "     (or: monado-service &)"
echo "  2. Point the active runtime at Monado, then run the sample:"
echo "       export XR_RUNTIME_JSON=/usr/share/openxr/1/openxr_monado.json"
echo "       # path may be /etc/xdg/openxr/1/active_runtime.json on some installs"
echo "       ./build/shared/Debug/build-linux-gcc-*/bin/samples/sample_02_openxr_stereo"
echo "  3. Controllers: Monado qwerty driver maps keyboard/mouse to poses."
echo "     See: https://monado.freedesktop.org/getting-started.html"
echo ""
echo "If XR_RUNTIME_JSON is unset, OpenXR uses the active_runtime.json symlink."
