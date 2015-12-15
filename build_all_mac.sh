#!/bin/sh

# -------------------------------------------------------
#	Usage : 
#	1) cd /path/to/chronomodel
#	2) sh build_all_mac.sh 0.1 /path/to/qt/bin/
# -------------------------------------------------------

# -------------------------------------------------------
#	Arguments checks
# -------------------------------------------------------
if [ $# -lt 2 ]; then
  echo "$1: please give the version number as argument!"
  echo "$2: please give your Qt bin path (absolute)! For example : /Users/your_name/Qt5.2.1/5.2.1/clang_64/bin"
  exit 2
fi

# -------------------------------------------------------
#	Create useful variables for the paths
# -------------------------------------------------------
ROOT_PATH=$(dirname $0)
BUNDLE_PATH=Release/Chronomodel.app
QT_BIN_PATH=$2
VERSION=$1

# -------------------------------------------------------
#	Clean last compilation files
# -------------------------------------------------------
rm -rf ${BUNDLE_PATH}

cd $ROOT_PATH


# -------------------------------------------------------
#	Create XCode project in Release mode (.pro file is in debug mode by default)
# -------------------------------------------------------
${QT_BIN_PATH}/qmake -spec macx-xcode "CONFIG+=release" $ROOT_PATH/Chronomodel.pro

# -------------------------------------------------------
#	Compile XCode project
#	Important note : important post-compilation steps are defined in the .pro file:
#	copying resource files as license, icons...
# ------------------------------------------------------
xcodebuild -configuration Release ONLY_ACTIVE_ARCH=NO | grep -A 5 error

# -------------------------------------------------------
#	Qt utility "macdeployqt" : deploy Chronomodel's Qt dependencies inside the bundle
# ------------------------------------------------------
${QT_BIN_PATH}/macdeployqt $BUNDLE_PATH

# -------------------------------------------------------
#	Create the package for installation
# ------------------------------------------------------
pkgbuild --component ${BUNDLE_PATH} --identifier com.chronomodel.pkg.app --version ${VERSION} --install-location /Applications ./deploy/mac/Chronomodel.pkg

# -------------------------------------------------------
#	Create the installer using the package and a distribution file (.dist)
# ------------------------------------------------------
productbuild --distribution deploy/mac/distribution.dist --package-path deploy/mac --resources deploy/mac/resources deploy/mac/chronomodel_mac_${VERSION}.pkg
#rm -rf ./deploy/mac/Chronomodel.pkg


exit 0
