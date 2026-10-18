#!/usr/bin/env bash
# Prints the patch notes of one version from CHANGELOG.md: the lines under "## <version>",
# up to the next "## " heading. The release workflow uses them as the release description.
#
# Usage: tools/release_notes.sh v2.3
set -euo pipefail

VERSION="$1"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

NOTES="$(awk -v version="$VERSION" '
    /^## / { inside = ($2 == version) ; next }
    inside
' "$ROOT/CHANGELOG.md")"

if [ -z "$(echo "$NOTES" | tr -d '[:space:]')" ]; then
    echo "No notes for $VERSION in CHANGELOG.md: add a \"## $VERSION\" section first" >&2
    exit 1
fi
echo "$NOTES"
