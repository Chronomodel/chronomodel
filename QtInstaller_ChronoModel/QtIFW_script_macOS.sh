#!/bin/bash
# ne pas mettre de blanc autour de =
# penser à faire Ch_copy_library.sh avant
# cd /Users/dufresne/ChronoModel-SoftWare/chronomodel/QtInstaller_ChronoModel
# sh Ch_copy_library.sh
#
# cd /Users/dufresne/ChronoModel-SoftWare/chronomodel/QtInstaller_ChronoModel/
# sh QtIFW_script_macOS.sh
# _________________________

clear

VERSION=3.3.5
QT_BIN_PATH=/Users/dufresne/Qt/Tools/QtInstallerFramework/4.10/bin

# _________________________
echo " 1 - Script compile intaller "

# -------------------------------------------------------
#	Vérifier que le chemin de Qt est bien celui de la machine
# -------------------------------------------------------
#ROOT_PATH=$(dirname $0)
# _________________________
echo " 2 - Copy the ChronoModel BUNDLE "

# make -j12 in /Users/dufresne/ChronoModel-SoftWare/build-Chronomodel-Qt_6_7_2_for_macOS-Release

RELEASE_PATH=/Users/dufresne/ChronoModel-SoftWare/chronomodel/build/Qt_6_9_1_for_macOS-Release/build/release/

BUNDLE="$RELEASE_PATH"chronomodel.app

#/Users/dufresne/ChronoModel-SoftWare/chronomodel/QtIFW_src/installer-packages/chronomodel_QtIFW.composant2/data

cp -R $BUNDLE installer-packages-macOS/chronomodel_QtIFW.composant1/data

# _________________________
echo " 2 - Copy the ChronoModel_bash BUNDLE "

BASH_RELEASE_PATH=/Users/dufresne/ChronoModel-SoftWare/chronomodel/build/Qt_6_9_1_for_macOS-Release/build/release/
BASH_BUNDLE="$BASH_RELEASE_PATH"chronomodel_bash.app
#/Users/dufresne/ChronoModel-SoftWare/chronomodel/QtIFW_src/installer-packages/chronomodel_QtIFW.composant2/data
cp -R $BASH_BUNDLE installer-packages-macOS/chronomodel_QtIFW.composant2/data

# _________________________
echo " 3 - Executing binarycreator"


DATE_FILE=$(date '+%Y%m%d')

INSTALLER=ChronoModel_v${VERSION}_Qt6.9.1_macOS12_${DATE_FILE}_Installer

echo " 4 - Created file : " $INSTALLER
${QT_BIN_PATH}/binarycreator --offline-only -c installer-config/config.xml -p installer-packages-macOS $INSTALLER

# _________________________
echo " 5 - View the BUNDLE : /Users/dufresne/ChronoModel-SoftWare/chronomodel/QtInstaller_ChronoModel"
# -------------------------------------------------------

