#include "MainController.h"
#include "PluginManager.h"
#include "MainWindow.h"
#include "Painting.h"
#include "AppSettings.h"


MainController::MainController(const QString& filePath)
{
    Painting::init();
    PluginManager::loadPlugins();
    //AppSettings::AppSettings();
    AppSettings::readSettings();

    mMainWindow = MainWindow::getInstance();
    mMainWindow->readSettings(filePath);
    mMainWindow->move(AppSettings::mLastPosition);
    mMainWindow->resize(AppSettings::mLastSize);

    mMainWindow->show();

}
