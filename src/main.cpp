#include "ChronoApp.h"
#include <QtWidgets>
#include "DarkBlueStyle.h"
#include "MainController.h"

#include "fftw3.h"

#include <iostream>
#include <cmath>
#include <errno.h>
#include <fenv.h>

#include "StdUtilities.h"

#pragma STDC FENV_ACCESS on



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
#ifdef Q_OS_MAC
    if(math_errhandling & MATH_ERREXCEPT)
    {
        std::cout << "cmath raises exceptions" << std::endl;
        feclearexcept(FE_ALL_EXCEPT);
    }
    if(math_errhandling & MATH_ERRNO)
    {
        std::cout << "cmath uses errno" << std::endl;
    }
#endif
    
    ChronoApp a(argc, argv);
    
    a.setApplicationName("Chronomodel");
    a.setApplicationDisplayName("Chronomodel");
    a.setApplicationVersion("1.1");
    a.setOrganizationDomain("http://www.chronomodel.com");
    a.setOrganizationName("CNRS");
    a.setWindowIcon(QIcon(":chronomodel.png"));
    
    qApp->setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    QString filePath;
    for(int i=0; i<argc; ++i)
    {
        QString arg(argv[i]);
        if(arg.contains(".chr", Qt::CaseInsensitive))
        {
            filePath = arg;
        }
    }
    
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    
    //if(argv)
    //QString filePath(argv[1]);
    
    
    //qInstallMessageHandler(customMessageHandler);
    
    //QApplication::setStyle(new DarkBlueStyle());
    
    MainController* c = new MainController(filePath);
    (void) c;
    
    return a.exec();
}

