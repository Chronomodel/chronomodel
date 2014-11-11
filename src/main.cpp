#include <QApplication>
#include <QtWidgets>
#include "DarkBlueStyle.h"
#include "MainController.h"

#include "fftw3.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    a.setApplicationName("Chronomodel");
    a.setApplicationDisplayName("Chronomodel");
    a.setApplicationVersion("1.0.0");
    a.setOrganizationDomain("http://www.chronomodel.com");
    a.setOrganizationName("CNRS");
    a.setWindowIcon(QIcon(":chronomodel.png"));
    
    qApp->setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    /*
    QPalette palette = QGuiApplication::palette();
    palette.setColor(QPalette::Window, QColor(75, 75, 75));
    palette.setColor(QPalette::WindowText, QColor(200, 200, 200));
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::AlternateBase, QColor(230, 230, 230));
    palette.setColor(QPalette::ToolTipBase, QColor(110, 110, 110));
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::black);
    palette.setColor(QPalette::Button, QColor(50, 50, 50));
    palette.setColor(QPalette::BrightText, Qt::white);
    palette.setBrush(QPalette::Window, QColor(75, 75, 75));
    a.setPalette(palette);*/
    
    //QApplication::setStyle(new DarkBlueStyle());
    
    MainController* c = new MainController();
    (void) c;
    
    return a.exec();
}
