#include <QApplication>
#include <QtWidgets>
#include "DarkBlueStyle.h"
#include "MainController.h"

#include "fftw3.h"

#include <iostream>

//#include "JuceHeader.h"

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);
    
    QString dt = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    QString txt = QString("[%1] ").arg(dt);
    
    switch (type)
    {
        case QtDebugMsg:
            txt += QString("{Debug} \t\t %1").arg(msg);
            break;
        case QtWarningMsg:
            txt += QString("{Warning} \t %1").arg(msg);
            break;
        case QtCriticalMsg:
            txt += QString("{Critical} \t %1").arg(msg);
            break;
        case QtFatalMsg:
            txt += QString("{Fatal} \t\t %1").arg(msg);
            abort();
            break;
    }
    
    QFile outFile("LogFile.log");
    if(outFile.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QTextStream textStream(&outFile);
        textStream << txt << endl;
    }
}


int main(int argc, char *argv[])
{
    //std::cout << "CPU speed : " << SystemStats::getCpuSpeedInMegaherz() << "MHz" << std::endl;
    
    QApplication a(argc, argv);
    
    a.setApplicationName("Chronomodel");
    a.setApplicationDisplayName("Chronomodel");
    a.setApplicationVersion("0.0.2");
    a.setOrganizationDomain("http://www.chronomodel.com");
    a.setOrganizationName("CNRS");
    a.setWindowIcon(QIcon(":chronomodel.png"));
    
    qApp->setAttribute(Qt::AA_UseHighDpiPixmaps);

    qInstallMessageHandler(customMessageHandler);
    
    //QApplication::setStyle(new DarkBlueStyle());
    
    MainController* c = new MainController();
    (void) c;
    
    return a.exec();
}

