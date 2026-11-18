#!/bin/zsh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GAME="$ROOT/.local/keeperfx"
if [[ ! -x "$GAME/keeperfx" || ! -f "$GAME/keeperfx.cfg" ]]; then
  echo "KeeperFX runtime is missing from $GAME" >&2
  exit 1
fi
exec "$GAME/keeperfx" "$@"
