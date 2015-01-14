#!/bin/sh

ROOT_PATH=$(dirname $0)
BUNDLE_PATH=Release/Chronomodel.app
QT_BIN_PATH=/Users/helori/Qt5.2.1/5.2.1/clang_64/bin
VERSION=1.0.0

rm -rf ${BUNDLE_PATH}

cd $ROOT_PATH

# Create XCode project
${QT_BIN_PATH}/qmake -spec macx-xcode "CONFIG+=release" $ROOT_PATH/Chronomodel.pro

# Build project
#MACOSX_DEPLOYMENT_TARGET=10.6 # to put in an xconfig file??
xcodebuild -configuration Release ONLY_ACTIVE_ARCH=NO | grep -A 5 error

# Create bundle
${QT_BIN_PATH}/macdeployqt $BUNDLE_PATH

# copy bundle resources (done in .pro file)
#cp -r $ROOT_PATH/deploy/Calib $BUNDLE_PATH/Contents/
#cp -r $ROOT_PATH/icon/Chronomodel.icns $BUNDLE_PATH/Contents/Resources/
#cp -r $ROOT_PATH/deploy/Chronomodel_User_Manual.pdf $BUNDLE_PATH/Contents/Resources/
#cp -r $ROOT_PATH/deploy/LicenseGPL30.txt $BUNDLE_PATH/Contents/Resources/
#cp -r $ROOT_PATH/deploy/readme.rtf $BUNDLE_PATH/Contents/Resources/

# Create package
# Note : pkgbuild : create a component package.

# Create the component plist :
pkgbuild --analyze --root ./Release/Chronomodel.app ./Release/ChronomodelComponent.plist
# Create the component package :
pkgbuild --root ./Release/Chronomodel.app --component-plist ./Release/ChronomodelComponent.plist ChronomodelComponent.pkg


productbuild --synthesize --package deploy/mac/Chronomodel.pkg deploy/mac/Distribution.xml 
productbuild --distribution ./Distribution.xml --package-path deploy/mac deploy/mac/Installer.pkg
productbuild --distribution deploy/mac/distribution.xml --component Release/Chronomodel.app /Applications deploy/mac/Chronomodel.pkg

# Alternative :
#pkgbuild --root "${ROOT_PATH}/build/release" \
#    --identifier "com.chronomodel.pkg.chronomodel" \
#    --version "$VERSION" \
#    --install-location "/Applications/" \
#    "deploy/mac/Chronomodel.pkg"


exit 0
