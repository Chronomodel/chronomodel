#!/bin/sh

CURRENT_PATH=$(dirname $0)
ROOT_PATH=$CURRENT_PATH
BUNDLE_PATH=$ROOT_PATH/build/release/Chronomodel.app
VERSION=1.0.0

cd $ROOT_PATH

# Create XCode project
/Users/helori/Qt5.2.1/5.2.1/clang_64/bin/qmake -spec macx-xcode $ROOT_PATH/Chronomodel.pro

# Build project
#MACOSX_DEPLOYMENT_TARGET=10.6 # to put in an xconfig file??
xcodebuild -configuration release -sdk macosx10.9 ONLY_ACTIVE_ARCH=NO

# Create bundle
/Users/helori/Qt5.2.1/5.2.1/clang_64/bin/macdeployqt $BUNDLE_PATH

# copy bundle resources
cp -r $ROOT_PATH/deploy/Calib $BUNDLE_PATH/Contents/
cp -r $ROOT_PATH/icon/Chronomodel.icns $BUNDLE_PATH/Contents/Resources/
cp -r $ROOT_PATH/deploy/Chronomodel_User_Manual.pdf $BUNDLE_PATH/Contents/Resources/
cp -r $ROOT_PATH/deploy/LicenseGPL30.txt $BUNDLE_PATH/Contents/Resources/
cp -r $ROOT_PATH/deploy/readme.rtf $BUNDLE_PATH/Contents/Resources/

# Create package
productbuild --component build/release/Chronomodel.app /Applications deploy/mac/Chronomodel.pkg

# Alternative :
#pkgbuild --root "${ROOT_PATH}/build/release" \
#    --identifier "com.chronomodel.pkg.chronomodel" \
#    --version "$VERSION" \
#    --install-location "/Applications/" \
#    "deploy/mac/Chronomodel.pkg"


exit 0
