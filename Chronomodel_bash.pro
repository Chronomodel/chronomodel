# ---------------------------------------------------------------------
#
# Copyright © CNRS	2014 - 2026
#
# Authors :
# 	Philippe LANOS
# 	Helori LANOS
# 	Philippe DUFRESNE
#
# This software is a computer program whose purpose is to
# create chronological models of archeological data using Bayesian statistics.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: GPL-3.0-or-later
# --------------------------------------------------------------------- */
include(Chronomodel.pro)
message("PRO_PATH : $$_PRO_FILE_PWD_")
TARGET = chronomodel_bash

RESOURCES = $$PRO_PATH/Chronomodel_bash.qrc


#########################################
# MacOS specific settings
#########################################
macx{

    ICON = $$PRO_PATH/icon/Chronomodel_Bash.icns

    RESOURCES_FILES.files -= $$PRO_PATH/deploy/Chronomodel.png
    RESOURCES_FILES.files += $$PRO_PATH/deploy/Chronomodel_Bash.png

    DISTFILES -= icon/Chronomodel.icns
    DISTFILES += icon/Chronomodel_Bash.icns

    RESOURCES -= icon/Chronomodel.icns
    RESOURCES -= icon/Chronomodel.ico
    RESOURCES -= icon/Chronomodel_Bash.ico

    RESOURCES += icon/Chronomodel_Bash.icns

}

win32{
        # Resource file (Windows only)
        message("WIN specific settings")
        QMAKESPEC = win32-g++ #win32-msvc  # for 32-bit and 64-bit
        RC_ICONS = $$PRO_PATH/icon/Chronomodel_bash.ico

        QT_FATAL_WARNING = 1

        ICON = $$PRO_PATH/icon/Chronomodel_bash.ico
        DISTFILES -= icon/Chronomodel.ico
        DISTFILES += icon/Chronomodel_Bash.ico

        RESOURCES -= icon/Chronomodel.icns
        RESOURCES -= icon/Chronomodel_Bash.icns
        RESOURCES -= icon/Chronomodel.ico

        RESOURCES += icon/Chronomodel_Bash.ico
}

# TRANSLATIONS
#########################################
TRANSLATIONS = translations_bash/Chronomodel_fr.ts \
               translations_bash/Chronomodel_en.ts

SOURCES -= src/main.cpp
SOURCES += src/main_bash.cpp

SOURCES -= src/MainController.cpp
SOURCES += src/MainController_bash.cpp

HEADERS -= src/MainController.h
HEADERS += src/MainController_bash.h

HEADERS -= src/ui/window/MainWindow.h
HEADERS += src/ui/window/MainWindow_bash.h

HEADERS -= src/ui/window/ProjectView.h
HEADERS += src/ui/window/ProjectView_bash.h

SOURCES -= src/ui/window/MainWindow.cpp
SOURCES += src/ui/window/MainWindow_bash.cpp

SOURCES -= src/ui/window/ProjectView.cpp
SOURCES += src/ui/window/ProjectView_bash.cpp


