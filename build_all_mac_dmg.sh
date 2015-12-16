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
  echo "$2: please give your Qt bin path (absolute)! For example : /Users/your_name/Qt5.5.1/5.5.1/clang_64/bin"
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
#rm -rf ${BUNDLE_PATH}

cd $ROOT_PATH

# -------------------------------------------------------
#	Create translations
# ------------------------------------------------------
${QT_BIN_PATH}/lupdate $ROOT_PATH/Chronomodel.pro
${QT_BIN_PATH}/linguist $ROOT_PATH/translations/Chronomodel_en.ts $ROOT_PATH/translations/Chronomodel_fr.ts
${QT_BIN_PATH}/lrelease $ROOT_PATH/Chronomodel.pro

# -------------------------------------------------------
#	Create XCode project in Release mode (.pro file is in debug mode by default)
# -------------------------------------------------------
#${QT_BIN_PATH}/qmake -spec macx-xcode "CONFIG+=release" $ROOT_PATH/Chronomodel.pro

# -------------------------------------------------------
#	Compile XCode project
#	Important note : important post-compilation steps are defined in the .pro file:
#	copying resource files as license, icons...
# ------------------------------------------------------
#xcodebuild -configuration Release ONLY_ACTIVE_ARCH=NO | grep -A 5 error

# -------------------------------------------------------
#	Qt utility "macdeployqt" : deploy Chronomodel's Qt dependencies inside the bundle
# ------------------------------------------------------
#${QT_BIN_PATH}/macdeployqt $BUNDLE_PATH

# -------------------------------------------------------
#	Create DMG
# ------------------------------------------------------
ln -s /Applications ./Release/Applications
mkdir ./Release/.background
cp deploy/mac/dmg_back.png ./Release/.background/background.png
cp deploy/mac/resources/license.rtf ./Release/license.rtf
cp deploy/mac/resources/readme.rtf ./Release/readme.rtf

hdiutil create -volname Chronomodel -srcfolder ./Release/ -ov -format UDZO Chronomodel.dmg

if true; then
device=$(hdiutil attach Chronomodel.dmg | egrep '^/dev/' | sed 1q | awk '{print $1}')
osascript build_dmg.scpt
sync
sync
#hdiutil unmount ${device}
fi

exit 0
