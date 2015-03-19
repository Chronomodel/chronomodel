#!/bin/sh

# -------------------------------------------------------
#	Ce fichier permet de créer Chronomodel et son installeur
#	sous Mac à partir du fichier .pro
#	Utilisation : 
#	1) cd /path/to/chronomodel (ie :::Macintosh HD ▸ Utilisateurs ▸ dufresne ▸ ChronoModel-SoftWare ▸ chronomodel)
#	2) sh create_pkg_mac.sh 0.1
# -------------------------------------------------------

# -------------------------------------------------------
#	Vérifier que la version de l'application est bien donnée en argument
# -------------------------------------------------------
if [ $# -lt 1 ]; then
  echo "$0: please give the version number as argument!"
  exit 2
fi

# -------------------------------------------------------
#	Vérifier que le chemin de Qt est bien celui de la machine
# -------------------------------------------------------
ROOT_PATH=$(dirname $0)
BUNDLE_PATH=Release/Chronomodel.app
QT_BIN_PATH=/Users/dufresne/Qt/5.4/clang_64/bin
VERSION=$1

# -------------------------------------------------------
#	Nettoyer la dernière compilation
# -------------------------------------------------------
rm -rf ${BUNDLE_PATH}

cd $ROOT_PATH


# -------------------------------------------------------
#	Créer le projet XCode : qmake a une option pour ça (-spec macx-xcode)
#	Ajouter la config "Release" car le .pro est en debug
# -------------------------------------------------------
${QT_BIN_PATH}/qmake -spec macx-xcode "CONFIG+=release" $ROOT_PATH/Chronomodel.pro

# -------------------------------------------------------
#	Compiler le projet XCode
#	Important : certaines étapes post-compilation sont définies dans le .pro
#	Par exemple : la copie des fichiers resource tels que la license, l'icone...
# ------------------------------------------------------
xcodebuild -configuration Release ONLY_ACTIVE_ARCH=NO | grep -A 5 error

# -------------------------------------------------------
#	Qt utility "macdeployqt" permet de déployer les dépendances (librairies Qt) dans le bundle .app
# ------------------------------------------------------
${QT_BIN_PATH}/macdeployqt $BUNDLE_PATH

# Create package
# Note : pkgbuild : create a component package.

# Create the component plist :

# -------------------------------------------------------
#	Create the package for installation
# ------------------------------------------------------
pkgbuild --component Release/Chronomodel.app --identifier com.chronomodel.pkg.app --version ${VERSION} --install-location ./Applications ./deploy/mac/Chronomodel.pkg
# -------------------------------------------------------
#	Create the installer using the package and a distribution file (.dist)
# "productbuild" is the tool used to create product archives
# ------------------------------------------------------
productbuild \
--distribution ./deploy/mac/Chrono_distribution.dist \
--resources ./deploy/mac/resource \
--package-path ./deploy/mac \
 ./deploy/mac/ChronoModel_${VERSION}.pkg

#rm -rf /deploy/mac/ChronoModel.pkg


exit 0
