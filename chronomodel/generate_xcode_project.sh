#!/bin/sh

ROOT_PATH=$(dirname $0)
BUNDLE_PATH=Release/Chronomodel.app
QT_BIN_PATH=/Users/helori/Qt5.2.1/5.2.1/clang_64/bin

cd $ROOT_PATH
${QT_BIN_PATH}/qmake -spec macx-xcode "CONFIG+=debug" $ROOT_PATH/Chronomodel.pro

exit 0
