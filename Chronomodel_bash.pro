# ---------------------------------------------------------------------

#Copyright or © or Copr. CNRS	2014 - 2024

#Authors :
#	Philippe LANOS
#	Helori LANOS
# 	Philippe DUFRESNE

#This software is a computer program whose purpose is to
#create chronological models of archeological data using Bayesian statistics.

# This software is governed by the CeCILL V2.1 license under French law and
# abiding by the rules of distribution of free software.  You can  use,
# modify and/ or redistribute the software under the terms of the CeCILL
# license as circulated by CEA, CNRS and INRIA at the following URL
# "http://www.cecill.info".

# As a counterpart to the access to the source code and  rights to copy,
# modify and redistribute granted by the license, users are provided only
# with a limited warranty  and the software's author,  the holder of the
# economic rights,  and the successive licensors  have only  limited
# liability.

# In this respect, the user's attention is drawn to the risks associated
# with loading,  using,  modifying and/or developing or reproducing the
# software by the user in light of its specific status of free software,
# that may mean  that it is complicated to manipulate,  and  that  also
# therefore means  that it is reserved for developers  and  experienced
# professionals having in-depth computer knowledge. Users are therefore
# encouraged to load and test the software's suitability as regards their
# requirements in conditions enabling the security of their systems and/or
# data to be ensured and,  more generally, to use and operate it in the
# same conditions as regards security.

# The fact that you are presently reading this means that you have had
# knowledge of the CeCILL V2.1 license and that you accept its terms.
# --------------------------------------------------------------------- */
include(Chronomodel.pro)
message("PRO_PATH : $$_PRO_FILE_PWD_")
TARGET = chronomodel_bash
#########################################
# MacOS specific settings
#########################################
macx{

     ICON = $$PRO_PATH/icon/Chronomodel_Bash.icns

     RESOURCES_FILES.files -= $$PRO_PATH/deploy/Chronomodel.png
     RESOURCES_FILES.files += $$PRO_PATH/deploy/Chronomodel_Bash.png
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

DISTFILES += \
    icon/Chronomodel_Bash.icns
