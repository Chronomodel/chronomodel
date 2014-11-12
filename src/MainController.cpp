#include "MainController.h"
#include "PluginManager.h"
#include "MainWindow.h"
#include "Painting.h"


MainController::MainController()
{
    Painting::init();
    
    PluginManager::loadPlugins();
    
    mMainWindow = MainWindow::getInstance();
    mMainWindow->readSettings();
    mMainWindow->show();
}
