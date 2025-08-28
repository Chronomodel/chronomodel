#!/bin/sh
# version du 2025-08-28
# pour lancer
# cd /Users/dufresne/ChronoModel-SoftWare/chronomodel/
# sh generate_xcode_project.sh

ROOT_PATH=$(dirname "$0")
QT_BIN_PATH=/Users/dufresne/Qt/6.9.1/macos/bin

echo "Génération du projet Xcode pour Chronomodel..."

cd "$ROOT_PATH" || exit 1
"$QT_BIN_PATH/qmake" \
    -spec macx-xcode \
    CONFIG+=xcode \
    CONFIG+=debug \
    CONFIG+=release \
    QMAKE_XCODE_USE_MODERN_HEADERMAPS=YES \
    QMAKE_XCODE_RUN_QMAKE_SCRIPT=NO \
    QMAKE_MAC_SDK=macosx \
    QMAKE_MACOSX_DEPLOYMENT_TARGET=12.0 \
    QMAKE_XCODE_ALWAYS_SEARCH_USER_PATHS=NO \
    "$ROOT_PATH/Chronomodel.pro"

echo "Projet Xcode généré avec succès !"
exit 0
