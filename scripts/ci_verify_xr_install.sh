#!/usr/bin/env bash
# Verify a cmake --install prefix contains the expected VneXR package layout.
set -euo pipefail

PREFIX="${1:?usage: $0 <install-prefix> [shared|static]}"
LIB_KIND="${2:-shared}"

HEADER="${PREFIX}/include/vertexnova/xr/xr.h"
if [[ ! -f "$HEADER" ]]; then
  echo "::error::Missing header: $HEADER"
  exit 1
fi

CMAKE_CONFIG="${PREFIX}/lib/cmake/VneXR/VneXRConfig.cmake"
if [[ ! -f "$CMAKE_CONFIG" ]]; then
  CMAKE_CONFIG="$(find "$PREFIX" -path '*/cmake/VneXR/VneXRConfig.cmake' -print -quit 2>/dev/null || true)"
fi
if [[ -z "$CMAKE_CONFIG" || ! -f "$CMAKE_CONFIG" ]]; then
  echo "::error::Missing VneXRConfig.cmake under $PREFIX"
  exit 1
fi

found_lib=false
while IFS= read -r -d '' lib; do
  found_lib=true
  echo "Found library: $lib"
done < <(find "$PREFIX/lib" "$PREFIX/lib64" -maxdepth 1 \( -name 'libvnexr.*' -o -name 'vnexr.lib' \) -print0 2>/dev/null || true)

if [[ "$found_lib" != true ]]; then
  echo "::error::No libvnexr.* under $PREFIX/lib (kind=$LIB_KIND)"
  exit 1
fi

echo "VneXR install verification passed: $PREFIX"
