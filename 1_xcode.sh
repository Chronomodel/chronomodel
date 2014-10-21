#!/bin/sh

CURRENT_PATH=$(dirname $0)
ROOT_PATH=$CURRENT_PATH

/Users/helori/Qt5.2.1/5.2.1/clang_64/bin/qmake -spec macx-xcode $ROOT_PATH/Chronomodel.pro
exit 0
