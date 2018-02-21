#include "MainController.h"
#include "PluginManager.h"
#include "MainWindow.h"
#include "Painting.h"
#include "AppSettings.h"


MainController::MainController(const QString& filePath)
{
    Painting::init();
    PluginManager::loadPlugins();
    AppSettings::AppSettings();
    //mAppSettings = new AppSettings();

    mMainWindow = MainWindow::getInstance();
    mMainWindow->readSettings(filePath);
    mMainWindow->show();

}
