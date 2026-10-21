#!/usr/bin/env bash
# Prints the patch notes of one version from CHANGELOG.md: the lines under "## <version>",
# up to the next "## " heading. The release workflow uses them as the release description.
#
# Usage: tools/release_notes.sh v2.3
set -euo pipefail

VERSION="$1"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

section() {
    # No version given: the topmost section
    awk -v version="$1" '
        /^## / { n++ ; inside = (version == "" ? n == 1 : $2 == version) ; next }
        inside
    ' "$ROOT/CHANGELOG.md"
}

empty() { [ -z "$(echo "$1" | tr -d '[:space:]')" ]; }

# No section named after the tag ("## Next" not renamed, a typo): the newest section is this release's notes
NOTES="$(section "$VERSION")"
empty "$NOTES" && NOTES="$(section "")"

if empty "$NOTES"; then
    echo "No notes in CHANGELOG.md: add a \"## $VERSION\" section first" >&2
    exit 1
fi
echo "$NOTES"
