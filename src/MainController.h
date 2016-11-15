#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include <QString>

class MainWindow;

class MainController
{
public:
    MainController(const QString& filePath);

private:
    MainWindow* mMainWindow;
};

#endif
