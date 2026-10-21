#!/usr/bin/env bash
# Prints the patch notes of one version from CHANGELOG.md: the lines under "## <version>",
# up to the next "## " heading. The release workflow uses them as the release description.
#
# Usage: tools/release_notes.sh v2.3
set -euo pipefail

VERSION="$1"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

section() {
    awk -v version="$1" '
        /^## / { inside = ($2 == version) ; next }
        inside
    ' "$ROOT/CHANGELOG.md"
}

empty() { [ -z "$(echo "$1" | tr -d '[:space:]')" ]; }

# Tagged before renaming "## Next" to "## <version>": the Next section is this release's notes
NOTES="$(section "$VERSION")"
empty "$NOTES" && NOTES="$(section Next)"

if empty "$NOTES"; then
    echo "No notes for $VERSION in CHANGELOG.md: add a \"## $VERSION\" (or \"## Next\") section first" >&2
    exit 1
fi
echo "$NOTES"
