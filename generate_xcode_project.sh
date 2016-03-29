#!/bin/sh

ROOT_PATH=$(dirname $0)
BUNDLE_PATH=release/chronomodel.app
QT_BIN_PATH=$1

cd $ROOT_PATH
${QT_BIN_PATH}/qmake -spec macx-xcode "CONFIG+=debug" $ROOT_PATH/Chronomodel.pro

exit 0
