#!/bin/bash
# version du 2026-09-25
# A lancer avec bash (pas sh) :
#   cd /Users/dufresne/ChronoModel-SoftWare/chronomodel/QtInstaller_ChronoModel
#   bash Ch_copy_library_and_signature.sh
#
# Ordre correct (l'ancienne version signait AVANT macdeployqt et avant la
# modification d'Info.plist : toute modification du bundle après la signature
# invalide celle-ci, ce qui fait planter l'application sur Apple Silicon) :
#   1. copie du bundle dans un dossier de travail (le dossier de build reste intact)
#   2. macdeployqt (copie Qt + libomp)
#   3. modification d'Info.plist
#   4. signature ad hoc (sans compte Apple payant), de l'intérieur vers l'extérieur
#      (dylibs, frameworks, puis .app)
#   5. vérifications
#
# Pas de blanc autour de = dans les affectations.
# -------------------------------------------------------------------------

set -euo pipefail
clear

# ---------------------------- A VERIFIER ---------------------------------
VERSION=4.0.0

QT_VERSION=6.11.1
QT_ROOT=/Users/dufresne/Qt/$QT_VERSION/macos
QT_BIN_PATH=$QT_ROOT/bin

RELEASE_PATH=/Users/dufresne/ChronoModel-SoftWare/chronomodel/build/Qt_6_11_1_for_macOS-Release/build/release
SRC_BUNDLE="$RELEASE_PATH/chronomodel.app"

# libomp universel (install name @rpath/libomp.dylib) : sert de -libpath à macdeployqt
LIBOMP_DIR=/Users/dufresne/ChronoModel-SoftWare/chronomodel/lib/openMP/macOS11

# Signature "ad hoc" (identité "-") : gratuite, sans compte Apple payant.
# Elle donne une signature cohérente et valide (obligatoire sur Apple Silicon),
# mais elle n'est PAS reconnue par Gatekeeper : à l'ouverture d'un fichier téléchargé,
# l'utilisateur devra autoriser l'application manuellement (voir LISEZMOI du .dmg).
SIGN_ID="-"

ROOT_PATH="$(cd "$(dirname "$0")" && pwd)"
STAGING="$ROOT_PATH/staging"
BUNDLE="$STAGING/chronomodel.app"
EXE="$BUNDLE/Contents/MacOS/chronomodel"
# -------------------------------------------------------------------------

echo "➡️  1 - Copie du bundle dans $STAGING"
[ -d "$SRC_BUNDLE" ] || { echo "❌ Bundle introuvable : $SRC_BUNDLE"; exit 1; }
rm -rf "$STAGING"
mkdir -p "$STAGING"
# ditto conserve les liens symboliques, les attributs étendus, etc.
ditto "$SRC_BUNDLE" "$BUNDLE"

echo "   Architectures de l'exécutable :"
ARCHS=$(lipo -archs "$EXE")
echo "   $ARCHS"
case "$ARCHS" in
    *x86_64*arm64*|*arm64*x86_64*) ;;
    *) echo "❌ L'exécutable n'est pas universel (x86_64 + arm64)"; exit 1 ;;
esac

echo "➡️  2 - macdeployqt"
# macdeployqt signe en "ad hoc" par défaut ; on re-signera tout à l'étape 4.
"$QT_BIN_PATH/macdeployqt" "$BUNDLE" -libpath="$LIBOMP_DIR"

find "$BUNDLE" -name "*.cstemp" -delete

echo "➡️  3 - Info.plist (version, types de document)"
PLIST="$BUNDLE/Contents/Info.plist"
PB=/usr/libexec/PlistBuddy

# Set si la clé existe, sinon Add
plist_set() {   # $1 = clé, $2 = type, $3 = valeur
    "$PB" -c "Set :$1 $3" "$PLIST" 2>/dev/null || "$PB" -c "Add :$1 $2 $3" "$PLIST"
}

plist_set CFBundleSignature string chml
plist_set CFBundleVersion string "$VERSION"
plist_set CFBundleShortVersionString string "$VERSION"

"$PB" -c "Delete :CFBundleDocumentTypes" "$PLIST" 2>/dev/null || true
"$PB" -c "Add :CFBundleDocumentTypes array" "$PLIST"
"$PB" -c "Add :CFBundleDocumentTypes:0:CFBundleTypeRole string Editor" "$PLIST"
"$PB" -c "Add :CFBundleDocumentTypes:0:CFBundleTypeIconFile string Chronomodel.icns" "$PLIST"
"$PB" -c "Add :CFBundleDocumentTypes:0:CFBundleTypeName string Chronomodel Project" "$PLIST"

"$PB" -x -c "Print" "$PLIST"

echo "➡️  4 - Signature (de l'intérieur vers l'extérieur, sans --deep)"
# Pas de --options runtime ni de --timestamp : ils ne servent qu'à la notarisation.
# Attention : avec une signature ad hoc, le "hardened runtime" ferait REFUSER le
# chargement de libomp et des plugins Qt (validation des bibliothèques), donc on ne l'active pas.
SIGN=(codesign --force --sign "$SIGN_ID")

echo "   dylibs (Frameworks, PlugIns, libomp...)"
find "$BUNDLE/Contents" -type f -name "*.dylib" -print0 | while IFS= read -r -d '' f; do
    echo "   - $f"
    "${SIGN[@]}" "$f"
done

echo "   frameworks"
if [ -d "$BUNDLE/Contents/Frameworks" ]; then
    for f in "$BUNDLE"/Contents/Frameworks/*.framework; do
        [ -d "$f" ] || continue
        echo "   - $f"
        "${SIGN[@]}" "$f"
    done
fi

echo "   application"
"${SIGN[@]}" "$BUNDLE"

echo "➡️  5 - Vérifications"
codesign --verify --deep --strict --verbose=2 "$BUNDLE"
codesign -dv --verbose=2 "$BUNDLE" 2>&1 | grep -E "Identifier|Authority|flags|TeamIdentifier" || true

echo "   Dépendances restées en chemin absolu (doit être vide) :"
find "$BUNDLE/Contents" -type f \( -perm -u+x -o -name "*.dylib" \) -print0 \
    | xargs -0 otool -L 2>/dev/null \
    | grep -E "/usr/local/|/opt/homebrew/|/Users/" || echo "   (aucune)"

echo "   libomp :"
otool -L "$EXE" | grep omp || echo "   ⚠️  pas de dépendance libomp (OpenMP inactif ?)"
ls "$BUNDLE/Contents/Frameworks" | grep omp || echo "   ⚠️  libomp.dylib absent de Contents/Frameworks"

# spctl -a répondrait "rejected" : normal, une signature ad hoc n'est pas acceptée par Gatekeeper.
# L'application se lance quand même sur votre Mac ; testez :
#   open "$BUNDLE"
echo "✅ Bundle prêt : $BUNDLE"