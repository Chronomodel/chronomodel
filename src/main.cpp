#include "ChronoApp.h"
#include <QtWidgets>
#include "MainController.h"
#include "StdUtilities.h"

#include "fftw3.h"

#include <iostream>
#include <cmath>
#include <errno.h>
#include <fenv.h>


#pragma STDC FENV_ACCESS on

#define DEBUG

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);
    
    QString dt = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    QString txt = QString("[%1] ").arg(dt);
    
    switch((int)type)
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
    if(outFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
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
    
    // --------------------------------------
    //  OpenGL specific settings
    // --------------------------------------
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);
    // --------------------------------------
    
    ChronoApp a(argc, argv);
    
    a.setApplicationName("ChronoModel");
    a.setApplicationDisplayName("ChronoModel");
    a.setApplicationVersion("1.3.7"); // check in file Chronomodel.pro
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
    
    QLocale::Language newLanguage = QLocale::system().language();
    QLocale::Country newCountry= QLocale::system().country();
    QLocale locale = QLocale(newLanguage, newCountry);
    //QLocale locale("en");
    
    locale.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale::setDefault(locale);
    
    /*QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    a.installTranslator(&qtTranslator);
    
    QTranslator translator;
    if(translator.load(locale, ":/Chronomodel", "_")){
        qDebug() << "Locale set to : " << QLocale::languageToString(locale.language());
        a.installTranslator(&translator);
    }*/
    
    //qInstallMessageHandler(customMessageHandler);
    
    MainController* c = new MainController(filePath);
    (void) c;
    
    return a.exec();
}

