#!/bin/bash
# ne pas mettre de blanc autour de =
# cd /Users/dufresne/ChronoModel-SoftWare/chronomodel/QtInstaller_InstallCurves/
# sh QtIFW_InstallCurves_script.sh
#_____________________________________

clear
# _________________________
echo "  1 Script compile Curves installer "
# -------------------------------------------------------

# -------------------------------------------------------
#	Vérifier que le chemin de Qt est bien celui de la machine
# -------------------------------------------------------

QT_BIN_PATH=/Users/dufresne/Qt/Tools/QtInstallerFramework/4.8/bin

# _________________________
echo "  2 copy the folder Calib in Data "
# -------------------------------------------------------

DEPLOY_PATH=/Users/dufresne/ChronoModel-SoftWare/chronomodel/deploy/Calib

cp -R $DEPLOY_PATH installer-packages/curves.composant1/data/Calib

VERSION=3.2.8

DATE_FILE=$(date '+%Y%m%d')

INSTALLER=ChronoModel_v${VERSION}_Curves_Installer_Qt6.7.2_macOS_${DATE_FILE}

echo "  3 Execution de binarycreator"
#${QT_BIN_PATH}/binarycreator --offline-only -c installer-config/config.xml -p installer-packages --compression 9 ChronoModel_Curves_Installer_v${VERSION} 

${QT_BIN_PATH}/binarycreator --offline-only -c installer-config/config_macOS.xml -p installer-packages --compression 9 $INSTALLER

echo " 4 - Created file : " $INSTALLER
# _________________________
echo " 5 - View the BUNDLE : /Users/dufresne/ChronoModel-SoftWare/chronomodel/QtInstaller_ChronoModel"
# -------------------------------------------------------