#include "MainController.h"
#include "ProjectManager.h"
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
    
    //Project* project = ProjectManager::newProject();
    // ProjectView must be created before loading the project :
    //mMainWindow->setProject(project);
    
    // This loads the project :
    ProjectManager::readSettings();
    mMainWindow->updateWindowTitle();
}
