#!/bin/sh

ROOT_PATH=$(dirname $0)
#BUNDLE_PATH=release/chronomodel.app
# set the QT directory in argument ex: sh generate_xcode_project.sh /Users/myName/Qt/5.7/clang_64/bin

# to determine which version of the macOS SDK is installed with xcode? type on a terminal
# xcodebuild -showsdks
QT_BIN_PATH=$1

cd $ROOT_PATH
${QT_BIN_PATH}/qmake -spec macx-xcode "CONFIG+=debug" $ROOT_PATH/Chronomodel.pro

exit 0
