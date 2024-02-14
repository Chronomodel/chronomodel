#!/bin/bash
# ne pas mettre de blanc autour de =
# cd /Users/dufresne/ChronoModel-SoftWare/chronomodel/QtInstaller_InstallCurves/
# sh QtIFW_InstallCurves_script.sh
#_____________________________________

clear
# _________________________
echo "$  1 Script compile Curves installer "
# -------------------------------------------------------

# -------------------------------------------------------
#	Vérifier que le chemin de Qt est bien celui de la machine
# -------------------------------------------------------

QT_BIN_PATH=/Users/dufresne/Qt/Tools/QtInstallerFramework/4.5/bin

# _________________________
echo "$  2 copy the folder Calib in Data "
# -------------------------------------------------------

DEPLOY_PATH=/Users/dufresne/ChronoModel-SoftWare/chronomodel/deploy/

cp -R $DEPLOY_PATH installer-packages/curves.composant1/data

VERSION=3.2.7

echo "$  2 Execution de binarycreator"
${QT_BIN_PATH}/binarycreator --offline-only -c installer-config/config.xml -p installer-packages --compression 9 Curves_Installer_v${VERSION} 