#!/usr/bin/env bash
# Builds a .pkg installer from the build output.
# Usage: tools/macos/make-pkg.sh <version> [build-dir] [arch-label]
# Output: dist/TriVerse-Chorus-<version>-macOS[-label].pkg
#   example labels: universal, arm64, x86_64
set -euo pipefail

VERSION="${1:?usage: make-pkg.sh <version> [build-dir] [arch-label]}"
BUILD_DIR="${2:-build}"
ARCH_LABEL="${3:-}"
ART="$BUILD_DIR/TriVerseChorus_artefacts/Release"
NAME="TriVerse Chorus"
ID="com.triverseaudio.chorus"

ROOT="$(mktemp -d)"
trap 'rm -rf "$ROOT"' EXIT

mkdir -p "$ROOT/Library/Audio/Plug-Ins/VST3" "$ROOT/Library/Audio/Plug-Ins/Components" "$ROOT/Applications"
cp -R "$ART/VST3/$NAME.vst3"      "$ROOT/Library/Audio/Plug-Ins/VST3/"
cp -R "$ART/AU/$NAME.component"   "$ROOT/Library/Audio/Plug-Ins/Components/"
cp -R "$ART/Standalone/$NAME.app" "$ROOT/Applications/"

# Stop Installer from installing over an old copy found elsewhere (relocation).
PLIST="$ROOT/../components.$$.plist"
pkgbuild --analyze --root "$ROOT" "$PLIST" > /dev/null
python3 - "$PLIST" <<'PY'
import plistlib, sys
path = sys.argv[1]
with open(path, 'rb') as f:
    items = plistlib.load(f)
for item in items:
    item['BundleIsRelocatable'] = False
with open(path, 'wb') as f:
    plistlib.dump(items, f)
PY

mkdir -p dist
COMPONENT="$ROOT/../component.$$.pkg"
pkgbuild --root "$ROOT" --component-plist "$PLIST" \
         --identifier "$ID" --version "$VERSION" --install-location / "$COMPONENT"

OUT="dist/TriVerse-Chorus-$VERSION-macOS${ARCH_LABEL:+-$ARCH_LABEL}.pkg"
if [[ -n "${INSTALLER_SIGN_IDENTITY:-}" ]]; then
    productbuild --package "$COMPONENT" --sign "$INSTALLER_SIGN_IDENTITY" "$OUT"
else
    productbuild --package "$COMPONENT" "$OUT"
fi
rm -f "$PLIST" "$COMPONENT"

echo "Created: $OUT"
