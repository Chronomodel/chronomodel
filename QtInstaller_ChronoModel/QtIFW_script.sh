#!/bin/bash
# ne pas mettre de blanc autour de =
#_____________________________________

clear
# _________________________
echo "$  1 Script compile intaller "
# -------------------------------------------------------

# -------------------------------------------------------
#	Vérifier que le chemin de Qt est bien celui de la machine
# -------------------------------------------------------
ROOT_PATH=$(dirname $0)
# _________________________
echo "$  2 copy the BUNDLE "
# -------------------------------------------------------

# make -j12 in /Users/dufresne/ChronoModel-SoftWare/build-Chronomodel-Qt_6_4_3_for_macOS-Release

RELEASE_PATH=/Users/dufresne/ChronoModel-SoftWare/build-Chronomodel-Qt_6_4_3_for_macOS-Release/build/release/
BUNDLE="$RELEASE_PATH"chronomodel.app
#/Users/dufresne/ChronoModel-SoftWare/chronomodel/QtIFW_src/installer-packages/chronomodel_QtIFW.composant2/data
cp -R $BUNDLE installer-packages/chronomodel_QtIFW.composant1/data

QT_BIN_PATH=/Users/dufresne/Qt/Tools/QtInstallerFramework/4.5/bin

VERSION=3.2.3

echo "$  3 Execution de binarycreator"
${QT_BIN_PATH}/binarycreator --offline-only -c installer-config/config.xml -p installer-packages ChronoModel_Installer_v${VERSION}