#!/bin/bash
# version du 2026-09-25
# ne pas mettre de blanc autour de =
# penser à faire Ch_copy_library_and_signature.sh avant
#   cd /Users/dufresne/ChronoModel-SoftWare/chronomodel/QtInstaller_ChronoModel
#   bash Ch_copy_library_and_signature.sh
#   bash QtIFW_script_macOS.sh
# _________________________

set -euo pipefail
clear

# ---------------------------- A VERIFIER ---------------------------------
VERSION=4.0.0
QT_BIN_PATH=/Users/dufresne/Qt/Tools/QtInstallerFramework/4.11/bin

RELEASE_PATH=/Users/dufresne/ChronoModel-SoftWare/chronomodel/build/Qt_6_11_1_for_macOS-Release/build/release

ROOT_PATH="$(cd "$(dirname "$0")" && pwd)"
STAGING="$ROOT_PATH/staging"
# Bundle SIGNE ET DEPLOYE par Ch_copy_library_and_signature.sh (macdeployqt + libomp + codesign).
# PAS le bundle brut de RELEASE_PATH : celui-la n'a jamais ete traite par macdeployqt et ne
# contient donc aucun framework Qt (c'est ce qui causait le "Library not loaded: QtWidgets").
BUNDLE="$STAGING/chronomodel.app"

DEST="$ROOT_PATH/installer-packages-macOS/chronomodel_QtIFW.composant1/data"
# -------------------------------------------------------------------------

echo "➡️  1 - Verification des chemins"
[ -d "$QT_BIN_PATH" ] || { echo "❌ QtIFW introuvable : $QT_BIN_PATH"; exit 1; }
[ -d "$BUNDLE" ]      || { echo "❌ Bundle signe introuvable : $BUNDLE (faire d'abord : bash Ch_copy_library_and_signature.sh)"; exit 1; }
[ -d "$BUNDLE/Contents/Frameworks/QtCore.framework" ] || { echo "❌ QtCore.framework absent de $BUNDLE : macdeployqt n'a pas ete execute sur ce bundle (relancer Ch_copy_library_and_signature.sh)"; exit 1; }

echo "➡️  2 - Copy the ChronoModel BUNDLE"
# Le vrai bug etait que ce script zippait le mauvais bundle (celui du dossier de
# build brut, jamais traite par macdeployqt) : voir la verification QtCore.framework
# ci-dessus. Le contournement par archive .zip + ditto execute par installscript.qs
# a l'installation s'est en plus revele fragile (echec "ditto -x -k" a l'installation).
# Retour a une copie directe du bundle (maintenant complet) ; ditto preserve les
# liens symboliques et attributs etendus (signature comprise), comme cp -R ne le
# ferait pas de maniere fiable.
mkdir -p "$DEST"
rm -f "$DEST/chronomodel_app.zip"    # nettoyage d'un ancien depot en archive
rm -rf "$DEST/chronomodel.app"
ditto "$BUNDLE" "$DEST/chronomodel.app"

# _________________________
#echo "➡️  - Copy the ChronoModel_bash BUNDLE "

#BASH_RELEASE_PATH=/Users/dufresne/ChronoModel-SoftWare/chronomodel/build/Qt_6_9_1_for_macOS-Release/build/release/
#BASH_BUNDLE="$BASH_RELEASE_PATH"chronomodel_bash.app
#cp -R $BASH_BUNDLE installer-packages-macOS/chronomodel_QtIFW.composant2/data

echo "➡️  3 - Detection automatique de la version de Qt et de macOS"
# Version Qt : extraite du nom du dossier de build (ex: Qt_6_11_1_for_macOS-Release -> 6.11.1),
# comme pour l'installeur Windows (QtIFW_script_winOS.bat) qui la deduit de QTVERSION.
QT_KIT=$(echo "$RELEASE_PATH" | grep -oE 'Qt_[0-9]+_[0-9]+_[0-9]+' || true)
if [ -n "$QT_KIT" ]; then
    QT_NUM=$(echo "$QT_KIT" | sed -E 's/^Qt_//; s/_/./g')
else
    echo "⚠️  Version Qt non detectee dans RELEASE_PATH, verifier le nom du dossier de build"
    QT_NUM="inconnue"
fi
QT_VER_STR="Qt${QT_NUM}"
echo "   Qt detecte : $QT_VER_STR"

# Version macOS minimale : lue directement dans le binaire compile, plutot que
# recopiee a la main (l'ancien nom "macOS12" ne correspondait plus a la cible reelle).
# Lue depuis le bundle source (le binaire n'est plus copie tel quel dans data/).
EXE="$BUNDLE/Contents/MacOS/chronomodel"
MACOS_MIN=$(otool -l "$EXE" 2>/dev/null | awk '/minos/{print $2; exit}')
if [ -n "${MACOS_MIN:-}" ]; then
    MACOS_TAG="macOS${MACOS_MIN%%.*}"
else
    echo "⚠️  Version macOS minimale non lue dans le binaire, verifier avec : otool -l \"$EXE\""
    MACOS_TAG="macOS"
fi
echo "   Cible macOS detectee : $MACOS_TAG (minos $MACOS_MIN)"

DATE_FILE=$(date '+%Y%m%d')
INSTALLER="ChronoModel_v${VERSION}_${QT_VER_STR}_${MACOS_TAG}_${DATE_FILE}_Installer"

echo "➡️  4 - Executing binarycreator : $INSTALLER"
cd "$ROOT_PATH"
"$QT_BIN_PATH/binarycreator" --offline-only -c installer-config/config.xml -p installer-packages-macOS "$INSTALLER"

# _________________________
echo "✅ 5 - Terminé : $ROOT_PATH/$INSTALLER"
# -------------------------------------------------------