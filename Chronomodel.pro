#-------------------------------------------------
#
# Created by Helori LANOS
# March 14th, 2014
# Chronomodel
#
#-------------------------------------------------

#PRO_PATH=$$PWD
PRO_PATH=$$_PRO_FILE_PWD_

message("-------------------------------------------")
CONFIG(debug, debug|release) {
        BUILD_DIR=build/debug
	message("Running qmake : Debug")
	macx{
		REAL_DESTDIR=Debug
	}
} else {
        BUILD_DIR=build/release
	message("Running qmake : Release")
	macx{
		REAL_DESTDIR=Release
	}
}
message("-------------------------------------------")


TARGET = Chronomodel
TEMPLATE = app

DESTDIR = $$BUILD_DIR
OBJECTS_DIR = $$BUILD_DIR/obj
MOC_DIR = $$BUILD_DIR/moc
RCC_DIR = $$BUILD_DIR/rcc

message("BUILD_DIR : $$BUILD_DIR")
message("DESTDIR : $$DESTDIR")
message("OBJECTS_DIR : $$OBJECTS_DIR")
message("MOC_DIR : $$MOC_DIR")
message("RCC_DIR : $$RCC_DIR")


# Qt modules (must be deployed along with the application
QT += core gui widgets svg

# Resource file (for images)
RESOURCES = $$PRO_PATH/Chronomodel.qrc

# Resource file (Windows only)
RC_FILE = $$PRO_PATH/Chronomodel.rc

# Config must use C++ 11 for random number generator
CONFIG += c++11

# Compilation warning flags
QMAKE_CXXFLAGS_WARN_ON += -Wno-unknown-pragmas

# Icon file
ICON = $$PRO_PATH/icon/Chronomodel.icns

macx{
	QMAKE_MAC_SDK = macosx10.10
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7

	RESOURCES_FILES.path = Contents/Resources
	RESOURCES_FILES.files += $$PRO_PATH/deploy/Calib
	RESOURCES_FILES.files += $$PRO_PATH/icon/Chronomodel.icns
	RESOURCES_FILES.files += $$PRO_PATH/deploy/Chronomodel_User_Manual.pdf
	RESOURCES_FILES.files += $$PRO_PATH/deploy/License.txt
	RESOURCES_FILES.files += $$PRO_PATH/deploy/readme.rtf
	QMAKE_BUNDLE_DATA += RESOURCES_FILES
}


#########################################
# DEFINES
#########################################

DEFINES += _USE_MATH_DEFINES

# Activate this to use FFT kernel method on hiostograms

USE_FFT = 1
DEFINES += "USE_FFT=$${USE_FFT}"

# Choose the plugins to compile directly with the application

USE_PLUGIN_UNIFORM = 1
USE_PLUGIN_GAUSS = 1
USE_PLUGIN_14C = 1
USE_PLUGIN_TL = 1
USE_PLUGIN_AM = 1

DEFINES += "USE_PLUGIN_UNIFORM=$${USE_PLUGIN_UNIFORM}"
DEFINES += "USE_PLUGIN_GAUSS=$${USE_PLUGIN_GAUSS}"
DEFINES += "USE_PLUGIN_14C=$${USE_PLUGIN_14C}"
DEFINES += "USE_PLUGIN_TL=$${USE_PLUGIN_TL}"
DEFINES += "USE_PLUGIN_AM=$${USE_PLUGIN_AM}"

#########################################
# LIBS
#########################################

INCLUDEPATH += lib/FFTW
macx{
    LIBS += -Llib/FFTW/mac -lfftw3f
}win32{
    LIBS += -L"$$_PRO_FILE_PWD_/lib/FFTW/win32" -lfftw3f-3
}

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
INCLUDEPATH += src/project/
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

HEADERS += src/MainController.h
HEADERS += src/AppSettings.h
HEADERS += src/StateKeys.h
HEADERS += src/ChronoApp.h

HEADERS += src/mcmc/Functions.h
HEADERS += src/mcmc/Generator.h
HEADERS += src/mcmc/MCMCLoop.h
HEADERS += src/mcmc/MCMCLoopMain.h
HEADERS += src/mcmc/MetropolisVariable.h
HEADERS += src/mcmc/MHVariable.h
HEADERS += src/mcmc/MCMCSettings.h

HEADERS += src/model/Model.h
HEADERS += src/model/Date.h
HEADERS += src/model/Event.h
HEADERS += src/model/EventKnown.h
HEADERS += src/model/Phase.h
HEADERS += src/model/Constraint.h
HEADERS += src/model/EventConstraint.h
HEADERS += src/model/PhaseConstraint.h
HEADERS += src/model/ModelUtilities.h

HEADERS += src/plugins/PluginAbstract.h
HEADERS += src/plugins/PluginFormAbstract.h
HEADERS += src/plugins/GraphViewRefAbstract.h

equals(USE_PLUGIN_TL, 1){
	HEADERS += src/plugins/plugin_tl/PluginTL.h
	HEADERS += src/plugins/plugin_tl/PluginTLForm.h
	HEADERS += src/plugins/plugin_tl/PluginTLRefView.h
}
equals(USE_PLUGIN_14C, 1){
	HEADERS += src/plugins/plugin_14C/Plugin14C.h
	HEADERS += src/plugins/plugin_14C/Plugin14CForm.h
	HEADERS += src/plugins/plugin_14C/Plugin14CRefView.h
}
equals(USE_PLUGIN_GAUSS, 1){
	HEADERS += src/plugins/plugin_gauss/PluginGauss.h
	HEADERS += src/plugins/plugin_gauss/PluginGaussForm.h
	HEADERS += src/plugins/plugin_gauss/PluginGaussRefView.h
}
equals(USE_PLUGIN_AM, 1){
	HEADERS += src/plugins/plugin_am/PluginMag.h
	HEADERS += src/plugins/plugin_am/PluginMagForm.h
	HEADERS += src/plugins/plugin_am/PluginMagRefView.h
}
equals(USE_PLUGIN_UNIFORM, 1){
	HEADERS += src/plugins/plugin_uniform/PluginUniform.h
	HEADERS += src/plugins/plugin_uniform/PluginUniformForm.h
}

HEADERS += src/project/Project.h
HEADERS += src/project/PluginManager.h
HEADERS += src/project/ProjectSettings.h
HEADERS += src/project/ProjectSettingsDialog.h
HEADERS += src/project/SetProjectState.h
HEADERS += src/project/StateEvent.h
    
HEADERS += src/ui/dialogs/AboutDialog.h
HEADERS += src/ui/dialogs/MCMCProgressDialog.h
HEADERS += src/ui/dialogs/MCMCSettingsDialog.h
HEADERS += src/ui/dialogs/AppSettingsDialog.h
HEADERS += src/ui/dialogs/EventDialog.h
HEADERS += src/ui/dialogs/ConstraintDialog.h
HEADERS += src/ui/dialogs/PhaseDialog.h
HEADERS += src/ui/dialogs/DateDialog.h
HEADERS += src/ui/dialogs/TrashDialog.h
HEADERS += src/ui/dialogs/StepDialog.h
HEADERS += src/ui/dialogs/PluginOptionsDialog.h

HEADERS += src/ui/graphs/GraphViewAbstract.h
HEADERS += src/ui/graphs/GraphView.h
HEADERS += src/ui/graphs/GraphCurve.h
HEADERS += src/ui/graphs/GraphZone.h
HEADERS += src/ui/graphs/AxisTool.h
HEADERS += src/ui/graphs/Ruler.h

HEADERS += src/ui/lookandfeel/Painting.h
HEADERS += src/ui/lookandfeel/DarkBlueStyle.h

HEADERS += src/ui/panel_model/ModelView.h
HEADERS += src/ui/panel_model/ImportDataView.h
HEADERS += src/ui/panel_model/EventPropertiesView.h
HEADERS += src/ui/panel_model/SceneGlobalView.h
HEADERS += src/ui/panel_model/EventsListItemDelegate.h

HEADERS += src/ui/panel_model/data/DatesList.h
HEADERS += src/ui/panel_model/data/DatesListItemDelegate.h
HEADERS += src/ui/panel_model/data/CalibrationView.h

HEADERS += src/ui/panel_model/scenes/AbstractScene.h
HEADERS += src/ui/panel_model/scenes/EventsScene.h
HEADERS += src/ui/panel_model/scenes/PhasesScene.h
HEADERS += src/ui/panel_model/scenes/AbstractItem.h
HEADERS += src/ui/panel_model/scenes/EventItem.h
HEADERS += src/ui/panel_model/scenes/EventKnownItem.h
HEADERS += src/ui/panel_model/scenes/DateItem.h
HEADERS += src/ui/panel_model/scenes/PhaseItem.h
HEADERS += src/ui/panel_model/scenes/ArrowItem.h
HEADERS += src/ui/panel_model/scenes/ArrowTmpItem.h

HEADERS += src/ui/panel_results/ResultsView.h
HEADERS += src/ui/panel_results/GraphViewResults.h
HEADERS += src/ui/panel_results/GraphViewDate.h
HEADERS += src/ui/panel_results/GraphViewEvent.h
HEADERS += src/ui/panel_results/GraphViewPhase.h

HEADERS += src/ui/panel_mcmc/MCMCView.h

HEADERS += src/ui/widgets/ColorPicker.h
HEADERS += src/ui/widgets/ZoomControls.h
HEADERS += src/ui/widgets/Collapsible.h
HEADERS += src/ui/widgets/ScrollCompressor.h
HEADERS += src/ui/widgets/Button.h
HEADERS += src/ui/widgets/CheckBox.h
HEADERS += src/ui/widgets/RadioButton.h
HEADERS += src/ui/widgets/Label.h
HEADERS += src/ui/widgets/LineEdit.h
HEADERS += src/ui/widgets/GroupBox.h
HEADERS += src/ui/widgets/HelpWidget.h
HEADERS += src/ui/widgets/Tabs.h
HEADERS += src/ui/widgets/Marker.h

HEADERS += src/ui/window/MainWindow.h
HEADERS += src/ui/window/ProjectView.h

HEADERS += src/utilities/Singleton.h
HEADERS += src/utilities/StdUtilities.h
HEADERS += src/utilities/QtUtilities.h


#########################################
# SOURCES
#########################################

SOURCES += src/main.cpp
SOURCES += src/MainController.cpp
SOURCES += src/AppSettings.cpp
SOURCES += src/ChronoApp.cpp

SOURCES += src/mcmc/Functions.cpp
SOURCES += src/mcmc/Generator.cpp
SOURCES += src/mcmc/MCMCLoop.cpp
SOURCES += src/mcmc/MCMCLoopMain.cpp
SOURCES += src/mcmc/MetropolisVariable.cpp
SOURCES += src/mcmc/MHVariable.cpp
SOURCES += src/mcmc/MCMCSettings.cpp

SOURCES += src/model/Model.cpp
SOURCES += src/model/Date.cpp
SOURCES += src/model/Event.cpp
SOURCES += src/model/EventKnown.cpp
SOURCES += src/model/Phase.cpp
SOURCES += src/model/Constraint.cpp
SOURCES += src/model/EventConstraint.cpp
SOURCES += src/model/PhaseConstraint.cpp
SOURCES += src/model/ModelUtilities.cpp

equals(USE_PLUGIN_TL, 1){
	SOURCES += src/plugins/plugin_tl/PluginTL.cpp
	SOURCES += src/plugins/plugin_tl/PluginTLForm.cpp
	SOURCES += src/plugins/plugin_tl/PluginTLRefView.cpp
}
equals(USE_PLUGIN_14C, 1){
	SOURCES += src/plugins/plugin_14C/Plugin14C.cpp
	SOURCES += src/plugins/plugin_14C/Plugin14CForm.cpp
	SOURCES += src/plugins/plugin_14C/Plugin14CRefView.cpp
}
equals(USE_PLUGIN_GAUSS, 1){
	SOURCES += src/plugins/plugin_gauss/PluginGauss.cpp
	SOURCES += src/plugins/plugin_gauss/PluginGaussForm.cpp
	SOURCES += src/plugins/plugin_gauss/PluginGaussRefView.cpp
}
equals(USE_PLUGIN_AM, 1){
	SOURCES += src/plugins/plugin_am/PluginMag.cpp
	SOURCES += src/plugins/plugin_am/PluginMagForm.cpp
	SOURCES += src/plugins/plugin_am/PluginMagRefView.cpp
}
equals(USE_PLUGIN_UNIFORM, 1){
	SOURCES += src/plugins/plugin_uniform/PluginUniform.cpp
	SOURCES += src/plugins/plugin_uniform/PluginUniformForm.cpp
}

SOURCES += src/project/Project.cpp
SOURCES += src/project/PluginManager.cpp
SOURCES += src/project/ProjectSettings.cpp
SOURCES += src/project/ProjectSettingsDialog.cpp
SOURCES += src/project/SetProjectState.cpp
SOURCES += src/project/StateEvent.cpp
    
SOURCES += src/ui/dialogs/AboutDialog.cpp
SOURCES += src/ui/dialogs/MCMCProgressDialog.cpp
SOURCES += src/ui/dialogs/MCMCSettingsDialog.cpp
SOURCES += src/ui/dialogs/AppSettingsDialog.cpp
SOURCES += src/ui/dialogs/EventDialog.cpp
SOURCES += src/ui/dialogs/PhaseDialog.cpp
SOURCES += src/ui/dialogs/ConstraintDialog.cpp
SOURCES += src/ui/dialogs/DateDialog.cpp
SOURCES += src/ui/dialogs/TrashDialog.cpp
SOURCES += src/ui/dialogs/StepDialog.cpp
SOURCES += src/ui/dialogs/PluginOptionsDialog.cpp

SOURCES += src/ui/graphs/GraphViewAbstract.cpp
SOURCES += src/ui/graphs/GraphView.cpp
SOURCES += src/ui/graphs/GraphCurve.cpp
SOURCES += src/ui/graphs/GraphZone.cpp
SOURCES += src/ui/graphs/AxisTool.cpp
SOURCES += src/ui/graphs/Ruler.cpp

SOURCES += src/ui/lookandfeel/Painting.cpp
SOURCES += src/ui/lookandfeel/DarkBlueStyle.cpp

SOURCES += src/ui/panel_model/ModelView.cpp
SOURCES += src/ui/panel_model/ImportDataView.cpp
SOURCES += src/ui/panel_model/EventPropertiesView.cpp
SOURCES += src/ui/panel_model/SceneGlobalView.cpp

SOURCES += src/ui/panel_model/data/DatesList.cpp
SOURCES += src/ui/panel_model/data/CalibrationView.cpp

SOURCES += src/ui/panel_model/scenes/AbstractScene.cpp
SOURCES += src/ui/panel_model/scenes/EventsScene.cpp
SOURCES += src/ui/panel_model/scenes/PhasesScene.cpp
SOURCES += src/ui/panel_model/scenes/AbstractItem.cpp
SOURCES += src/ui/panel_model/scenes/EventItem.cpp
SOURCES += src/ui/panel_model/scenes/EventKnownItem.cpp
SOURCES += src/ui/panel_model/scenes/DateItem.cpp
SOURCES += src/ui/panel_model/scenes/PhaseItem.cpp
SOURCES += src/ui/panel_model/scenes/ArrowItem.cpp
SOURCES += src/ui/panel_model/scenes/ArrowTmpItem.cpp

SOURCES += src/ui/panel_results/ResultsView.cpp
SOURCES += src/ui/panel_results/GraphViewResults.cpp
SOURCES += src/ui/panel_results/GraphViewDate.cpp
SOURCES += src/ui/panel_results/GraphViewEvent.cpp
SOURCES += src/ui/panel_results/GraphViewPhase.cpp

SOURCES += src/ui/panel_mcmc/MCMCView.cpp

SOURCES += src/ui/widgets/ColorPicker.cpp
SOURCES += src/ui/widgets/ZoomControls.cpp
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

SOURCES += src/ui/window/MainWindow.cpp
SOURCES += src/ui/window/ProjectView.cpp

SOURCES += src/utilities/StdUtilities.cpp
SOURCES += src/utilities/QtUtilities.cpp


message("-------------------------------------------")
