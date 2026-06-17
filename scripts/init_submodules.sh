#!/usr/bin/env bash
# Initialize VneXR git submodules.
#
# Usage:
#   ./scripts/init_submodules.sh
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

SUBMODULES=(
  cmake/vnecmake
  deps/internal/vnecommon
  deps/internal/vnelogging
  deps/external/googletest
)

echo "=== VneXR submodule init ==="
for path in "${SUBMODULES[@]}"; do
  echo ">> submodule init: $path"
  git submodule update --init "$path"
done
echo "=== submodule init done ==="
