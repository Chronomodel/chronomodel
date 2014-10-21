#!/bin/sh

CURRENT_PATH=$(dirname $0)
ROOT_PATH=$CURRENT_PATH
BUNDLE_PATH=$ROOT_PATH/Release/Chronomodel.app

cd $ROOT_PATH
/Users/helori/Qt5.2.1/5.2.1/clang_64/bin/macdeployqt $BUNDLE_PATH
cp -r $ROOT_PATH/data/Calib $BUNDLE_PATH/Contents/
cp -r $ROOT_PATH/icon/Chronomodel.icns $BUNDLE_PATH/Contents/Resources/
exit 0
