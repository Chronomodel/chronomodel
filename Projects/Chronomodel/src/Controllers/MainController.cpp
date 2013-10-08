#include "MainController.h"
#include "MainWindow.h"

MainController::MainController()
{
    mMainWindow = new MainWindow();
    mMainWindow->show();
}
