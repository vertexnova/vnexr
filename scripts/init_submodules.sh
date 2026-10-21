#!/usr/bin/env bash
# Initialize VneXR git submodules.
#
# Usage:
#   ./scripts/init_submodules.sh              # public deps only
#   ./scripts/init_submodules.sh full         # includes vnemath + private vnerhi
#
# full tier expects recorded gitlinks (mode 160000) for deps/internal/vnemath and
# deps/internal/vnerhi — same as vnegfx. .gitmodules alone is not enough; add with:
#   git submodule add <url> deps/internal/vnemath
#   git submodule add git@github.com:vertexnova/vnerhi.git deps/internal/vnerhi
#
# Local override when vnerhi is checked out beside vnexr:
#   export VNE_VNERHI_PATH=/path/to/vnerhi
#   ./scripts/init_submodules.sh full
#
# CI: when GITHUB_TOKEN or REPO_ACCESS_TOKEN is set, configures authenticated HTTPS
# for submodule fetch.
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

configure_git_for_submodules() {
  local token="${REPO_ACCESS_TOKEN:-${GITHUB_TOKEN:-}}"
  if [[ -n "$token" ]]; then
    echo ">> git: configuring HTTPS auth for submodule fetch"
    git config --global --add url."https://x-access-token:${token}@github.com/".insteadOf "https://github.com/"
    git config --global --add url."https://x-access-token:${token}@github.com/".insteadOf "git@github.com:"
    git config --global --add url."https://x-access-token:${token}@github.com/".insteadOf "ssh://git@github.com/"
    return 0
  fi

  if [[ "${GITHUB_ACTIONS:-}" == "true" ]]; then
    echo ">> git: CI detected — rewriting SSH GitHub URLs to HTTPS"
    git config --global --add url."https://github.com/".insteadOf "git@github.com:"
    git config --global --add url."https://github.com/".insteadOf "ssh://git@github.com/"
  fi
}

configure_git_for_submodules

TIER="${1:-public}"

PHASE1=(
  cmake/vnecmake
  deps/internal/vnecommon
  deps/internal/vnelogging
  deps/external/googletest
)

FULL_EXTRA=(
  deps/internal/vnemath
)

# True when path is a recorded gitlink (mode 160000) in the index / HEAD.
is_gitlink() {
  local path="$1"
  local mode
  mode="$(git ls-files --stage -- "$path" 2>/dev/null | awk '{print $1}' | head -1 || true)"
  [[ "$mode" == "160000" ]]
}

link_local_vnerhi() {
  local dest="deps/internal/vnerhi"
  if [[ -z "${VNE_VNERHI_PATH:-}" ]]; then
    return 1
  fi
  if [[ ! -f "${VNE_VNERHI_PATH}/CMakeLists.txt" ]]; then
    echo "ERROR: VNE_VNERHI_PATH does not look like vnerhi: ${VNE_VNERHI_PATH}"
    exit 1
  fi
  mkdir -p deps/internal
  if [[ -e "$dest" && ! -L "$dest" ]]; then
    # Real checkout already present (submodule update) — leave it.
    if [[ -f "$dest/CMakeLists.txt" ]]; then
      echo ">> vnerhi already present at $dest (ignoring VNE_VNERHI_PATH)"
      return 0
    fi
    echo "ERROR: $dest exists and is not a symlink — remove it or unset VNE_VNERHI_PATH"
    exit 1
  fi
  ln -sfn "$(cd "$VNE_VNERHI_PATH" && pwd)" "$dest"
  echo "Linked vnerhi: $dest -> $VNE_VNERHI_PATH"
  return 0
}

# Drop absolute / broken symlinks left from local VNE_VNERHI_PATH workflows so
# `git submodule update` can populate a real tree (especially on CI runners).
clear_broken_vnerhi_symlink() {
  local dest="deps/internal/vnerhi"
  if [[ -L "$dest" && ! -e "$dest" ]]; then
    echo ">> removing broken symlink $dest"
    rm -f "$dest"
    return 0
  fi
  if [[ -L "$dest" && "${GITHUB_ACTIONS:-}" == "true" ]]; then
    echo ">> CI: removing local symlink $dest so submodule can be checked out"
    rm -f "$dest"
  fi
}

init_vnerhi() {
  if link_local_vnerhi; then
    return 0
  fi

  clear_broken_vnerhi_symlink

  if ! is_gitlink deps/internal/vnerhi; then
    cat <<'EOF' >&2
ERROR: deps/internal/vnerhi is not registered as a git submodule (no gitlink).

Fix: add the submodule (do not commit a local symlink):
  git submodule add git@github.com:vertexnova/vnerhi.git deps/internal/vnerhi

Or for local-only work:
  export VNE_VNERHI_PATH=../vnerhi && ./scripts/init_submodules.sh full
EOF
    exit 1
  fi

  echo "Initializing private submodule deps/internal/vnerhi (requires org access)..."
  git submodule sync deps/internal/vnerhi
  if ! git submodule update --init deps/internal/vnerhi; then
    cat <<'EOF' >&2
ERROR: failed to init deps/internal/vnerhi (private repo).

Options:
  1. SSH access:  git@github.com:vertexnova/vnerhi.git
  2. HTTPS + PAT: export REPO_ACCESS_TOKEN=<token> && ./scripts/init_submodules.sh full
  3. Local path:  export VNE_VNERHI_PATH=../vnerhi && ./scripts/init_submodules.sh full
EOF
    exit 1
  fi
}

init_paths() {
  local path
  for path in "$@"; do
    if ! is_gitlink "$path"; then
      echo "ERROR: '$path' is listed for init but is not a git submodule (no mode-160000 gitlink)." >&2
      echo "  Remove it from scripts/init_submodules.sh or: git submodule add <url> $path" >&2
      exit 1
    fi
    echo ">> submodule init: $path"
    git submodule update --init "$path"
  done
}

echo "=== VneXR submodule init (tier=$TIER) ==="
init_paths "${PHASE1[@]}"

if [[ "$TIER" == "full" ]]; then
  init_paths "${FULL_EXTRA[@]}"
  init_vnerhi
  # Nested deps required for Vulkan (Android NDK has vulkan.h but not vulkan.hpp).
  # Keep this list minimal — optional sample deps (vneio, vnescene, …) stay skipped.
  local_vnerhi_nested=(
    cmake/vnecmake
    deps/internal/vnecommon
    deps/internal/vnelogging
    deps/internal/vnemath
    deps/external/vulkan-headers
  )
  echo ">> nested init inside vnerhi (may take a while)..."
  git -C deps/internal/vnerhi submodule sync --recursive
  git -C deps/internal/vnerhi submodule update --init --recursive \
    "${local_vnerhi_nested[@]}"
  if [[ ! -f deps/internal/vnerhi/deps/external/vulkan-headers/include/vulkan/vulkan.hpp ]]; then
    echo "ERROR: vnerhi deps/external/vulkan-headers missing vulkan.hpp after nested init." >&2
    exit 1
  fi
fi

echo "=== submodule init done ==="
