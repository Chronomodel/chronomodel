#include "MainController.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainController* controller = new MainController();
    return app.exec();
}
