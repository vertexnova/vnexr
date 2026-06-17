#!/usr/bin/env bash
# Install Linux packages needed to build vnexr with OpenXR + Vulkan.
#
# Usage:
#   ./scripts/install_linux_openxr_deps.sh
#
# Supports apt (Debian/Ubuntu). For other distros install equivalents manually:
#   Vulkan SDK / libvulkan-dev, X11 or Wayland dev headers (OpenXR loader WSI).
set -euo pipefail

if ! command -v apt-get >/dev/null 2>&1; then
  echo "This script supports apt-based distros (Debian/Ubuntu)."
  echo "Install manually: cmake, g++ or clang, libvulkan-dev, libx11-dev, libxrandr-dev, libxxf86vm-dev"
  exit 1
fi

echo ">> Installing Linux OpenXR + Vulkan build dependencies (sudo required)"
sudo apt-get update -qq
sudo apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  pkg-config \
  libvulkan-dev \
  vulkan-tools \
  libx11-dev \
  libxrandr-dev \
  libxxf86vm-dev \
  libgl1-mesa-dev

echo ">> Done. Verify Vulkan:"
if command -v vulkaninfo >/dev/null 2>&1; then
  vulkaninfo --summary 2>/dev/null | head -20 || echo "vulkaninfo: no GPU/runtime (build may still succeed)"
else
  echo "vulkaninfo not found after install"
fi

echo ""
echo "Optional runtime for headset testing: Monado (https://monado.freedesktop.org/)"
echo "  sudo apt-get install monado-vulkan-driver openxr-loader  # if packaged on your distro"
