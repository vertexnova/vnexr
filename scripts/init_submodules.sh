#!/usr/bin/env bash
# Initialize VneXR git submodules.
#
# Usage:
#   ./scripts/init_submodules.sh              # public deps only
#   ./scripts/init_submodules.sh full         # includes private vnerhi (+ vnemath via vnerhi)
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
    echo "ERROR: $dest exists and is not a symlink — remove it or unset VNE_VNERHI_PATH"
    exit 1
  fi
  ln -sfn "$(cd "$VNE_VNERHI_PATH" && pwd)" "$dest"
  echo "Linked vnerhi: $dest -> $VNE_VNERHI_PATH"
  return 0
}

init_vnerhi() {
  if link_local_vnerhi; then
    return 0
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
    echo ">> submodule init: $path"
    git submodule update --init "$path"
  done
}

echo "=== VneXR submodule init (tier=$TIER) ==="
init_paths "${PHASE1[@]}"

if [[ "$TIER" == "full" ]]; then
  init_paths "${FULL_EXTRA[@]}"
  init_vnerhi
  echo ">> nested init inside vnerhi (may take a while)..."
  git -C deps/internal/vnerhi submodule sync --recursive || true
  git -C deps/internal/vnerhi submodule update --init --recursive \
    cmake/vnecmake deps/internal/vnecommon deps/internal/vnelogging deps/internal/vnemath || true
fi

echo "=== submodule init done ==="
