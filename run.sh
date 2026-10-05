#!/usr/bin/env bash
# native ps4 launcher wrapper
set -euo pipefail
HERE="$(cd -- "$(dirname -- "$0")" && pwd)"
exec "$HERE/play" "$@"
