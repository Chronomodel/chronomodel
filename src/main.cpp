#include <QApplication>
#include <QtWidgets>
#include "DarkBlueStyle.h"
#include "MainController.h"

#include "fftw3.h"

#include <iostream>

#include "JuceHeader.h"


int main(int argc, char *argv[])
{
    std::cout << "CPU speed : " << SystemStats::getCpuSpeedInMegaherz() << "MHz" << std::endl;
    
    QApplication a(argc, argv);
    
    a.setApplicationName("Chronomodel");
    a.setApplicationDisplayName("Chronomodel");
    a.setApplicationVersion("1.0.0");
    a.setOrganizationDomain("http://www.chronomodel.com");
    a.setOrganizationName("CNRS");
    a.setWindowIcon(QIcon(":chronomodel.png"));
    
    qApp->setAttribute(Qt::AA_UseHighDpiPixmaps);

    
    //QApplication::setStyle(new DarkBlueStyle());
    
    MainController* c = new MainController();
    (void) c;
    
    return a.exec();
}
