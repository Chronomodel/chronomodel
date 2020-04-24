/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#include "ChronoApp.h"
#include <QtWidgets>
#include "MainController.h"
#include "StdUtilities.h"

#include "fftw3.h"

#include <iostream>
#include <cmath>
#include <errno.h>
#include <fenv.h>


// STDC FENV_ACCESS ON // not supported with Clang

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);

    QString dt = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    QString txt = QString("[%1] ").arg(dt);

    switch (int(type)) {
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
        default:
            return ;
    }

    QFile outFile("LogFile.log");
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
        QTextStream textStream(&outFile);
        textStream << txt << endl;
    }
}

int main(int argc, char *argv[])
{

#ifdef Q_OS_MAC
    if (math_errhandling & MATH_ERREXCEPT) {
        std::cout << "cmath raises exceptions" << std::endl;
        feclearexcept(FE_ALL_EXCEPT);
    }
    if (math_errhandling & MATH_ERRNO)
        std::cout << "cmath uses errno" << std::endl;

#endif

    ChronoApp a(argc, argv);

    a.setApplicationName("ChronoModel");
    a.setApplicationDisplayName("ChronoModel");
    a.setApplicationVersion("2.0.21");//VERSION_NUMBER);//"2.0.9-alpha");  // must match value in Chronomodel.pro
    a.setOrganizationDomain("http://www.chronomodel.com");
    a.setOrganizationName("CNRS");
    a.setWindowIcon(QIcon(":chronomodel.png"));

    qApp->setAttribute(Qt::AA_UseHighDpiPixmaps);

    QString filePath = "";
    for (int i(0); i<argc; ++i) {
        QString arg(argv[i]);
        if (arg.contains(".chr", Qt::CaseInsensitive))
            filePath = arg;
    }

    QLocale::Language newLanguage = QLocale::system().language();
    QLocale::Country newCountry= QLocale::system().country();
    QLocale locale = QLocale(newLanguage, newCountry);

    locale.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale::setDefault(locale);

  //  qApp->setFont(QApplication::font("QMenu"));

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
    c = nullptr;
    return  a.exec();

}
