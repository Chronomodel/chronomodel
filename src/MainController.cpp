#include "MainController.h"
#include "PluginManager.h"
#include "MainWindow.h"
#include "Painting.h"


MainController::MainController(const QString& filePath)
{
    Painting::init();
    PluginManager::loadPlugins();
    
    mMainWindow = MainWindow::getInstance();
    mMainWindow->readSettings(filePath);
    mMainWindow->show();

}
