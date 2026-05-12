#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPOS_DIR="$SCRIPT_DIR/../repos"

mkdir -p "$REPOS_DIR"

clone_or_update() {
    local url="$1"
    local dest="$2"
    if [ -d "$dest/.git" ]; then
        echo "Updating $dest..."
        git -C "$dest" pull --ff-only
    else
        echo "Cloning $url -> $dest..."
        git clone "$url" "$dest"
    fi
}

clone_or_update https://github.com/bitcoin/bitcoin "$REPOS_DIR/bitcoin"
