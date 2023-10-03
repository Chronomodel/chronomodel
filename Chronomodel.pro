# ---------------------------------------------------------------------

#Copyright or © or Copr. CNRS	2014 - 2023

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

DEFINES += VERSION_NUMBER="3,2,6"
VERSION = 3.2.6
 #VERSION_NUMBER # must match value in src/main.cpp and mainControler and Chronomodel.rc (for windows)
#PRO_PATH=$$PWD
PRO_PATH=$$_PRO_FILE_PWD_

message("-------------------------------------------")
CONFIG(debug, debug|release) {
        BUILD_DIR=build/debug
        DEFINES += DEBUG # to validate all #ifdef DEBUG in all file
	message("Running qmake : Debug")
	macx{
		REAL_DESTDIR=Debug
	}
} else {
        BUILD_DIR=build/release
	message("Running qmake : Release")
        DEFINES += QT_NO_DEBUG_OUTPUT # to disable qDebug()
	macx{
		REAL_DESTDIR=Release
	}
}
message("-------------------------------------------")


TARGET = chronomodel
TEMPLATE = app

DESTDIR = $$BUILD_DIR
OBJECTS_DIR = $$BUILD_DIR/obj
MOC_DIR = $$BUILD_DIR/moc
RCC_DIR = $$BUILD_DIR/rcc

message("PRO_PATH : $$_PRO_FILE_PWD_")
message("BUILD_DIR : $$BUILD_DIR")
message("DESTDIR : $$DESTDIR")
message("OBJECTS_DIR : $$OBJECTS_DIR")
message("MOC_DIR : $$MOC_DIR")
message("RCC_DIR : $$RCC_DIR")


# Qt modules (must be deployed along with the application)
QT += core gui widgets svg

# Resource file (for images)
RESOURCES = $$PRO_PATH/Chronomodel.qrc
#RESOURCES = Chronomodel.qrc

# Compilation warning flags
 QMAKE_CXXFLAGS_WARN_ON += -Wno-unknown-pragmas -Wno-unused-parameter # invalid option for MSVC2015
# QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter


#########################################
# C++ 2a
# Config must use C++ 11 for random number generator
# This works for Windows, Linux & Mac 10.7 and +
# In the future we'll need to increase to C++17
# which offered namespace std::experimental::parallel;
#########################################
CONFIG += c++2a
QMAKE_CXXFLAGS += -std=c++2a

#########################################
# MacOS specific settings
#########################################
macx{
    message("MacOSX specific settings")
	# Icon file
        ICON = $$PRO_PATH/icon/Chronomodel.icns

	# This is the SDK used to compile : change it to whatever latest version of mac you are using.
	# to determine which version of the macOS SDK is installed with xcode? type on a terminal
	# xcodebuild -showsdks


        QMAKESPEC = macx-clang

        QMAKE_MAC_SDK = macosx
        message("QMAKE_MAC_SDK = $$QMAKE_MAC_SDK")

	# This is the minimal Mac OS X version supported by the application. You must have the corresponding SDK installed whithin XCode.
        #QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.14 # OS X 10.9 	Mavericks oct 2013  # essai sinon 10.14
        QMAKE_MACOSX_DEPLOYMENT_TARGET = 12 # depuis le 2022-10-11
	# Define a set of resources to deploy inside the bundle :
	RESOURCES_FILES.path = Contents/Resources
        # RESOURCES_FILES.files += $$PRO_PATH/deploy/Calib // used for older version <3.1.6
        RESOURCES_FILES.files += $$PRO_PATH/deploy/ABOUT.html
        RESOURCES_FILES.files += $$PRO_PATH/deploy/Chronomodel.png
        #RESOURCES_FILES.files += $$PRO_PATH/icon/Chronomodel.icns
	QMAKE_BUNDLE_DATA += RESOURCES_FILES

}
#########################################
# Windows specific settings
#########################################
win32{
	# Resource file (Windows only)
        message("WIN specific settings")
        QMAKESPEC = win32-g++ #win32-msvc  # for 32-bit and 64-bit
        RC_FILE += Chronomodel.rc
        RC_ICONS += $$PRO_PATH/icon/Chronomodel.ico
        QT_FATAL_WARNING = 1
}


#########################################
# DEFINES
#########################################

DEFINES += _USE_MATH_DEFINES

# Activate this to use FFT kernel method on histograms

USE_FFT = 1
DEFINES += "USE_FFT=$${USE_FFT}"

# Choose the plugins to compile directly with the application

USE_PLUGIN_UNIFORM = 1
USE_PLUGIN_GAUSS = 1
USE_PLUGIN_14C = 1
USE_PLUGIN_TL = 1
USE_PLUGIN_AM = 1
USE_PLUGIN_F14C = 1
USE_PLUGIN_DENSITY = 1

DEFINES += "USE_PLUGIN_UNIFORM=$${USE_PLUGIN_UNIFORM}"
DEFINES += "USE_PLUGIN_GAUSS=$${USE_PLUGIN_GAUSS}"
DEFINES += "USE_PLUGIN_14C=$${USE_PLUGIN_14C}"
DEFINES += "USE_PLUGIN_TL=$${USE_PLUGIN_TL}"
DEFINES += "USE_PLUGIN_AM=$${USE_PLUGIN_AM}"
DEFINES += "USE_PLUGIN_F14C=$${USE_PLUGIN_F14C}"
DEFINES += "USE_PLUGIN_DENSITY=$${USE_PLUGIN_DENSITY}"

#########################################
# FFTW
#########################################

macx{
	# IMPORTANT NOTE :
	# We use FFTW 3.2.2 on Mac to support Mac OS X versions from 10.7.
	# We provide FFTW.3.2.2.dmg if you want to install it on your system, but this is not necessary!
	# The generated XCode project will locate FFTW files in the project directory and statically link against it.

	# this is to include fftw.h in the code :
	# INCLUDEPATH += $$_PRO_FILE_PWD_/lib/FFTW/mac
        INCLUDEPATH += $$_PRO_FILE_PWD_/lib/fftw-3.2.2/mac # for macos 12
        LIBS += -L"$$_PRO_FILE_PWD_/lib/fftw-3.2.2/mac" -lfftw3

        #INCLUDEPATH += $$_PRO_FILE_PWD_/lib/fftw-3.3.10/mac # with intel and arm, for macos 13
        #LIBS += -L"$$_PRO_FILE_PWD_/lib/fftw-3.3.10/mac" -lfftw3

	# Link the application with FFTW library
	# If no dylib are present, static libs (.a) are used => that's why we moved .dylib files in a "dylib" folder.


        #LIBS += -L"$$_PRO_FILE_PWD_/lib/FFTW/mac" -lfftw3



	# If we were deploying FFTW as a dynamic library, we should :
	# - Move all files from "lib/FFTW/mac/dylib" to "lib/FFTW/mac"
	# - Uncomment the lines below to copy dylib files to the bundle
	# - We may also need to call install_name_tool on both dylib and chronomodel executable.
	#	This has not been tested, so use otool -L path/to/dylib/files to check dependencies

	#FFTW_FILES.path = Contents/Frameworks
	#FFTW_FILES.files += $$PRO_PATH/deploy/mac/FFTW/libfftw3f.dylib
	#QMAKE_BUNDLE_DATA += FFTW_FILES
	#QMAKE_POST_LINK += install_name_tool -id @executable_path/../Frameworks/libfftw3f.dylib $$PRO_PATH/deploy/mac/FFTW/libfftw3f.dylib
	#QMAKE_POST_LINK += install_name_tool -change old/path @executable_path/../Frameworks/libfftw3f.3.dylib $$PRO_PATH/Release/Chronomodel.app/Contents/MacOS/Chronomodel;
}

win32{
        INCLUDEPATH += lib/fftw-3.2.2
        contains(QT_ARCH, i386) {
            message("32-bit")
            LIBS += -L"$$_PRO_FILE_PWD_/lib/fftw-3.2.2/win32" -lfftw3-3

        } else { # to compile with a x64 machine
            message("64-bit")
            LIBS += -L"$$_PRO_FILE_PWD_/lib/fftw-3.2.2/win64" -lfftw3-3
        }
}
#linux :
unix:!macx{
        INCLUDEPATH += lib/fftw-3.2.2
        LIBS += -lfftw3
}

#########################################
message("INCLUDEPATH : $$INCLUDEPATH")
message("LIBS : $$LIBS")

# TRANSLATIONS
#########################################
TRANSLATIONS = translations/Chronomodel_fr.ts \
               translations/Chronomodel_en.ts

# For Microsoft Visual Studio only
CODECFORSRC = UTF-8

#########################################
# INCLUDES
#########################################

INCLUDEPATH += src/
INCLUDEPATH += src/mcmc/
INCLUDEPATH += src/model/
INCLUDEPATH += src/plugins/
INCLUDEPATH += src/plugins/plugin_14C/
INCLUDEPATH += src/plugins/plugin_am/
INCLUDEPATH += src/plugins/plugin_gauss/
INCLUDEPATH += src/plugins/plugin_tl/
INCLUDEPATH += src/plugins/plugin_uniform/
INCLUDEPATH += src/plugins/plugin_F14C/
INCLUDEPATH += src/plugins/plugin_density/
INCLUDEPATH += src/project/
INCLUDEPATH += src/curve/
INCLUDEPATH += src/ui/
INCLUDEPATH += src/ui/dialogs/
INCLUDEPATH += src/ui/graphs/
INCLUDEPATH += src/ui/lookandfeel/
INCLUDEPATH += src/ui/panel_model/
INCLUDEPATH += src/ui/panel_model/data/
INCLUDEPATH += src/ui/panel_model/scenes/
INCLUDEPATH += src/ui/panel_results/
INCLUDEPATH += src/ui/panel_mcmc/
INCLUDEPATH += src/ui/widgets/
INCLUDEPATH += src/ui/window/
INCLUDEPATH += src/utilities/

#########################################
# HEADERS
#########################################

HEADERS += src/MainController.h \
    src/ui/dialogs/RebuildCurveDialog.h \
    src/ui/panel_model/MultiCalibrationView.h \
    src/ui/panel_model/MultiCalibrationDrawing.h \
    src/utilities/Matrix.h
HEADERS += src/AppSettings.h
HEADERS += src/StateKeys.h
HEADERS += src/ChronoApp.h

HEADERS += src/curve/CurveSettings.h
HEADERS += src/curve/CurveSettingsView.h
HEADERS += src/curve/MCMCLoopCurve.h
HEADERS += src/curve/ModelCurve.h
HEADERS += src/curve/CurveUtilities.h

HEADERS += src/mcmc/Functions.h
HEADERS += src/mcmc/Generator.h
HEADERS += src/mcmc/MCMCLoop.h
HEADERS += src/mcmc/MCMCLoopChrono.h
HEADERS += src/mcmc/MCMCSettings.h
HEADERS += src/mcmc/MetropolisVariable.h
HEADERS += src/mcmc/MHVariable.h

HEADERS += src/model/Constraint.h
HEADERS += src/model/Date.h
HEADERS += src/model/Event.h
HEADERS += src/model/EventConstraint.h
HEADERS += src/model/Bound.h
HEADERS += src/model/Model.h
HEADERS += src/model/ModelUtilities.h
HEADERS += src/model/Phase.h
HEADERS += src/model/PhaseConstraint.h

HEADERS += src/plugins/CalibrationCurve.h
HEADERS += src/plugins/GraphViewRefAbstract.h
HEADERS += src/plugins/PluginAbstract.h
HEADERS += src/plugins/PluginFormAbstract.h
HEADERS += src/plugins/PluginRefCurveSettingsView.h
HEADERS += src/plugins/PluginSettingsViewAbstract.h
HEADERS += src/plugins/RefCurve.h

equals(USE_PLUGIN_TL, 1){
	HEADERS += src/plugins/plugin_tl/PluginTL.h
	HEADERS += src/plugins/plugin_tl/PluginTLForm.h
	HEADERS += src/plugins/plugin_tl/PluginTLRefView.h
	HEADERS += src/plugins/plugin_tl/PluginTLSettingsView.h
}
equals(USE_PLUGIN_14C, 1){
	HEADERS += src/plugins/plugin_14C/Plugin14C.h
	HEADERS += src/plugins/plugin_14C/Plugin14CForm.h
	HEADERS += src/plugins/plugin_14C/Plugin14CRefView.h
        HEADERS += src/plugins/plugin_14C/Plugin14CSettingsView.h
}
equals(USE_PLUGIN_GAUSS, 1){
	HEADERS += src/plugins/plugin_gauss/PluginGauss.h
	HEADERS += src/plugins/plugin_gauss/PluginGaussForm.h
	HEADERS += src/plugins/plugin_gauss/PluginGaussRefView.h
        HEADERS += src/plugins/plugin_gauss/PluginGaussSettingsView.h
}
equals(USE_PLUGIN_AM, 1){
	HEADERS += src/plugins/plugin_am/PluginMag.h
	HEADERS += src/plugins/plugin_am/PluginMagForm.h
	HEADERS += src/plugins/plugin_am/PluginMagRefView.h
        HEADERS += src/plugins/plugin_am/PluginMagSettingsView.h
}
equals(USE_PLUGIN_UNIFORM, 1){
	HEADERS += src/plugins/plugin_uniform/PluginUniform.h
	HEADERS += src/plugins/plugin_uniform/PluginUniformForm.h
	HEADERS += src/plugins/plugin_uniform/PluginUniformRefView.h
	HEADERS += src/plugins/plugin_uniform/PluginUniformSettingsView.h
}
equals(USE_PLUGIN_F14C, 1){
    HEADERS += src/plugins/plugin_F14C/PluginF14C.h
    HEADERS += src/plugins/plugin_F14C/PluginF14CForm.h
    HEADERS += src/plugins/plugin_F14C/PluginF14CRefView.h
    HEADERS += src/plugins/plugin_F14C/PluginF14CSettingsView.h
}
equals(USE_PLUGIN_DENSITY, 1){
        HEADERS += src/plugins/plugin_density/PluginDensity.h
        HEADERS += src/plugins/plugin_density/PluginDensityForm.h
        HEADERS += src/plugins/plugin_density/PluginDensityRefView.h
        HEADERS += src/plugins/plugin_density/PluginDensitySettingsView.h
}

HEADERS += src/project/PluginManager.h
HEADERS += src/project/Project.h
HEADERS += src/project/StudyPeriodSettings.h
HEADERS += src/project/SetProjectState.h
HEADERS += src/project/StateEvent.h


HEADERS += src/ui/dialogs/AboutDialog.h
HEADERS += src/ui/dialogs/AppSettingsDialog.h
HEADERS += src/ui/dialogs/AppSettingsDialogItemDelegate.h
HEADERS += src/ui/dialogs/ConstraintDialog.h
HEADERS += src/ui/dialogs/DateDialog.h
HEADERS += src/ui/dialogs/EventDialog.h
HEADERS += src/ui/dialogs/MCMCProgressDialog.h
HEADERS += src/ui/dialogs/MCMCSettingsDialog.h
HEADERS += src/ui/dialogs/PhaseDialog.h
HEADERS += src/ui/dialogs/PluginOptionsDialog.h
HEADERS += src/ui/dialogs/StudyPeriodDialog.h
HEADERS += src/ui/dialogs/TrashDialog.h

HEADERS += src/ui/graphs/AxisTool.h
HEADERS += src/ui/graphs/GraphCurve.h
HEADERS += src/ui/graphs/GraphView.h
HEADERS += src/ui/graphs/GraphViewAbstract.h
HEADERS += src/ui/graphs/GraphZone.h
HEADERS += src/ui/graphs/Ruler.h

HEADERS += src/ui/lookandfeel/Painting.h

HEADERS += src/ui/panel_model/EventPropertiesView.h
HEADERS += src/ui/panel_model/EventsListItemDelegate.h
HEADERS += src/ui/panel_model/ImportDataView.h
HEADERS += src/ui/panel_model/ModelView.h
HEADERS += src/ui/panel_model/SceneGlobalView.h

HEADERS += src/ui/panel_model/data/CalibrationDrawing.h
HEADERS += src/ui/panel_model/data/CalibrationView.h
HEADERS += src/ui/panel_model/data/DatesList.h
HEADERS += src/ui/panel_model/data/DatesListItemDelegate.h

HEADERS += src/ui/panel_model/scenes/AbstractItem.h
HEADERS += src/ui/panel_model/scenes/AbstractScene.h
HEADERS += src/ui/panel_model/scenes/ArrowItem.h
HEADERS += src/ui/panel_model/scenes/ArrowTmpItem.h
HEADERS += src/ui/panel_model/scenes/DateItem.h
HEADERS += src/ui/panel_model/scenes/EventItem.h
HEADERS += src/ui/panel_model/scenes/EventKnownItem.h
HEADERS += src/ui/panel_model/scenes/EventsScene.h
HEADERS += src/ui/panel_model/scenes/PhaseItem.h
HEADERS += src/ui/panel_model/scenes/PhasesScene.h

HEADERS += src/ui/panel_results/GraphViewLambda.h
HEADERS += src/ui/panel_results/GraphViewCurve.h
HEADERS += src/ui/panel_results/GraphViewDate.h
HEADERS += src/ui/panel_results/GraphViewEvent.h
HEADERS += src/ui/panel_results/GraphViewPhase.h
HEADERS += src/ui/panel_results/GraphViewResults.h
HEADERS += src/ui/panel_results/GraphViewS02.h
HEADERS += src/ui/panel_results/ResultsView.h

HEADERS += src/ui/widgets/Button.h
HEADERS += src/ui/widgets/CheckBox.h
HEADERS += src/ui/widgets/Collapsible.h
HEADERS += src/ui/widgets/ColorPicker.h
HEADERS += src/ui/widgets/GroupBox.h
HEADERS += src/ui/widgets/HelpWidget.h
HEADERS += src/ui/widgets/Label.h
HEADERS += src/ui/widgets/LineEdit.h
HEADERS += src/ui/widgets/Marker.h
HEADERS += src/ui/widgets/RadioButton.h
HEADERS += src/ui/widgets/ScrollCompressor.h
HEADERS += src/ui/widgets/Tabs.h
HEADERS += src/ui/widgets/SwitchAction.h
HEADERS += src/ui/widgets/CurveWidget.h

HEADERS += src/ui/window/MainWindow.h
HEADERS += src/ui/window/ProjectView.h

HEADERS += src/utilities/DateUtils.h
HEADERS += src/utilities/DoubleValidator.h
HEADERS += src/utilities/QtUtilities.h
HEADERS += src/utilities/Singleton.h
HEADERS += src/utilities/StdUtilities.h


#########################################
# SOURCES
#########################################

SOURCES += src/AppSettings.cpp \
    src/ui/dialogs/RebuildCurveDialog.cpp \
    src/ui/panel_model/MultiCalibrationView.cpp \
    src/ui/panel_model/MultiCalibrationDrawing.cpp \
    src/utilities/Matrix.cpp
SOURCES += src/ChronoApp.cpp
SOURCES += src/main.cpp
SOURCES += src/MainController.cpp

SOURCES += src/curve/CurveSettings.cpp
SOURCES += src/curve/CurveSettingsView.cpp
SOURCES += src/curve/MCMCLoopCurve.cpp
SOURCES += src/curve/ModelCurve.cpp
SOURCES += src/curve/CurveUtilities.cpp

SOURCES += src/mcmc/Functions.cpp
SOURCES += src/mcmc/Generator.cpp
SOURCES += src/mcmc/MCMCLoop.cpp
SOURCES += src/mcmc/MCMCLoopChrono.cpp
SOURCES += src/mcmc/MCMCSettings.cpp
SOURCES += src/mcmc/MetropolisVariable.cpp
SOURCES += src/mcmc/MHVariable.cpp

SOURCES += src/model/Constraint.cpp
SOURCES += src/model/Date.cpp
SOURCES += src/model/Event.cpp
SOURCES += src/model/EventConstraint.cpp
SOURCES += src/model/Bound.cpp
SOURCES += src/model/Model.cpp
SOURCES += src/model/ModelUtilities.cpp
SOURCES += src/model/Phase.cpp
SOURCES += src/model/PhaseConstraint.cpp

SOURCES += src/plugins/CalibrationCurve.cpp
SOURCES += src/plugins/PluginRefCurveSettingsView.cpp
SOURCES += src/plugins/RefCurve.cpp

equals(USE_PLUGIN_TL, 1) {
    SOURCES += src/plugins/plugin_tl/PluginTL.cpp
    SOURCES += src/plugins/plugin_tl/PluginTLForm.cpp
    SOURCES += src/plugins/plugin_tl/PluginTLRefView.cpp
    SOURCES += src/plugins/plugin_tl/PluginTLSettingsView.cpp
}
equals(USE_PLUGIN_14C, 1) {
    SOURCES += src/plugins/plugin_14C/Plugin14C.cpp
    SOURCES += src/plugins/plugin_14C/Plugin14CForm.cpp
    SOURCES += src/plugins/plugin_14C/Plugin14CRefView.cpp
    SOURCES += src/plugins/plugin_14C/Plugin14CSettingsView.cpp
}
equals(USE_PLUGIN_GAUSS, 1) {
    SOURCES += src/plugins/plugin_gauss/PluginGauss.cpp
    SOURCES += src/plugins/plugin_gauss/PluginGaussForm.cpp
    SOURCES += src/plugins/plugin_gauss/PluginGaussRefView.cpp
    SOURCES += src/plugins/plugin_gauss/PluginGaussSettingsView.cpp
}
equals(USE_PLUGIN_AM, 1) {
    SOURCES += src/plugins/plugin_am/PluginMag.cpp
    SOURCES += src/plugins/plugin_am/PluginMagForm.cpp
    SOURCES += src/plugins/plugin_am/PluginMagRefView.cpp
    SOURCES += src/plugins/plugin_am/PluginMagSettingsView.cpp
}
equals(USE_PLUGIN_UNIFORM, 1) {
    SOURCES += src/plugins/plugin_uniform/PluginUniform.cpp
    SOURCES += src/plugins/plugin_uniform/PluginUniformForm.cpp
    SOURCES += src/plugins/plugin_uniform/PluginUniformRefView.cpp
    SOURCES += src/plugins/plugin_uniform/PluginUniformSettingsView.cpp
}
equals(USE_PLUGIN_F14C, 1) {
    SOURCES += src/plugins/plugin_F14C/PluginF14C.cpp
    SOURCES += src/plugins/plugin_F14C/PluginF14CForm.cpp
    SOURCES += src/plugins/plugin_F14C/PluginF14CRefView.cpp
    SOURCES += src/plugins/plugin_F14C/PluginF14CSettingsView.cpp
}
equals(USE_PLUGIN_DENSITY, 1) {
    SOURCES +=src/plugins/plugin_density/PluginDensity.cpp
    SOURCES += src/plugins/plugin_density/PluginDensityForm.cpp
    SOURCES += src/plugins/plugin_density/PluginDensityRefView.cpp
    SOURCES += src/plugins/plugin_density/PluginDensitySettingsView.cpp
}
SOURCES += src/project/PluginManager.cpp
SOURCES += src/project/Project.cpp
SOURCES += src/project/StudyPeriodSettings.cpp
SOURCES += src/project/SetProjectState.cpp
SOURCES += src/project/StateEvent.cpp

SOURCES += src/ui/dialogs/AboutDialog.cpp
SOURCES += src/ui/dialogs/AppSettingsDialog.cpp
SOURCES += src/ui/dialogs/ConstraintDialog.cpp
SOURCES += src/ui/dialogs/DateDialog.cpp
SOURCES += src/ui/dialogs/EventDialog.cpp
SOURCES += src/ui/dialogs/MCMCProgressDialog.cpp
SOURCES += src/ui/dialogs/MCMCSettingsDialog.cpp
SOURCES += src/ui/dialogs/PhaseDialog.cpp
SOURCES += src/ui/dialogs/PluginOptionsDialog.cpp
SOURCES += src/ui/dialogs/StudyPeriodDialog.cpp
SOURCES += src/ui/dialogs/TrashDialog.cpp

SOURCES += src/ui/graphs/AxisTool.cpp
SOURCES += src/ui/graphs/GraphCurve.cpp
SOURCES += src/ui/graphs/GraphView.cpp
SOURCES += src/ui/graphs/GraphViewAbstract.cpp
SOURCES += src/ui/graphs/GraphZone.cpp
SOURCES += src/ui/graphs/Ruler.cpp

SOURCES += src/ui/lookandfeel/Painting.cpp

SOURCES += src/ui/panel_model/data/CalibrationDrawing.cpp
SOURCES += src/ui/panel_model/data/CalibrationView.cpp
SOURCES += src/ui/panel_model/data/DatesList.cpp

SOURCES += src/ui/panel_model/EventPropertiesView.cpp
SOURCES += src/ui/panel_model/ImportDataView.cpp
SOURCES += src/ui/panel_model/ModelView.cpp
SOURCES += src/ui/panel_model/SceneGlobalView.cpp

SOURCES += src/ui/panel_model/scenes/AbstractItem.cpp
SOURCES += src/ui/panel_model/scenes/AbstractScene.cpp
SOURCES += src/ui/panel_model/scenes/ArrowItem.cpp
SOURCES += src/ui/panel_model/scenes/ArrowTmpItem.cpp
SOURCES += src/ui/panel_model/scenes/DateItem.cpp
SOURCES += src/ui/panel_model/scenes/EventItem.cpp
SOURCES += src/ui/panel_model/scenes/EventKnownItem.cpp
SOURCES += src/ui/panel_model/scenes/EventsScene.cpp
SOURCES += src/ui/panel_model/scenes/PhaseItem.cpp
SOURCES += src/ui/panel_model/scenes/PhasesScene.cpp

SOURCES += src/ui/panel_results/GraphViewLambda.cpp
SOURCES += src/ui/panel_results/GraphViewCurve.cpp
SOURCES += src/ui/panel_results/GraphViewDate.cpp
SOURCES += src/ui/panel_results/GraphViewEvent.cpp
SOURCES += src/ui/panel_results/GraphViewPhase.cpp
SOURCES += src/ui/panel_results/GraphViewResults.cpp
SOURCES += src/ui/panel_results/GraphViewS02.cpp
SOURCES += src/ui/panel_results/ResultsView.cpp

SOURCES += src/ui/widgets/ColorPicker.cpp
SOURCES += src/ui/widgets/Collapsible.cpp
SOURCES += src/ui/widgets/ScrollCompressor.cpp
SOURCES += src/ui/widgets/Button.cpp
SOURCES += src/ui/widgets/CheckBox.cpp
SOURCES += src/ui/widgets/RadioButton.cpp
SOURCES += src/ui/widgets/Label.cpp
SOURCES += src/ui/widgets/LineEdit.cpp
SOURCES += src/ui/widgets/GroupBox.cpp
SOURCES += src/ui/widgets/HelpWidget.cpp
SOURCES += src/ui/widgets/Tabs.cpp
SOURCES += src/ui/widgets/Marker.cpp
SOURCES += src/ui/widgets/SwitchAction.cpp
SOURCES += src/ui/widgets/CurveWidget.cpp

SOURCES += src/ui/window/MainWindow.cpp
SOURCES += src/ui/window/ProjectView.cpp

SOURCES += src/utilities/StdUtilities.cpp
SOURCES += src/utilities/QtUtilities.cpp
SOURCES += src/utilities/DoubleValidator.cpp
SOURCES += src/utilities/DateUtils.cpp

DISTFILES += \
    Chronomodel.rc \
    icon/Chronomodel.ico
