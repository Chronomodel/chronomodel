/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#include "ChronoApp.h"
#include "MainController_bash.h"

#include "QtCore/qlogging.h"


#include <QtWidgets>
#include <iostream>
#include <cmath>
#include <errno.h>
#include <fenv.h>
#include <stdlib.h>

#include <vector>

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
            exit (EXIT_FAILURE);
            break;
        default:
            return ;
    }

    QFile outFile("LogFile.log");
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
        QTextStream textStream(&outFile);
        textStream << txt << Qt::endl;
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

    a.setApplicationName("ChronoModel"); // Must be ChronoModel to find the calibration curves
    a.setApplicationDisplayName("ChronoModel_bash");
    a.setApplicationVersion("3.2.2");//VERSION_NUMBER);  // must match value in Chronomodel.pro
    //a.setOrganizationDomain("http://www.chronomodel.com");
    a.setOrganizationName("CNRS");
    a.setWindowIcon(QIcon(":chronomodel_bash.icns"));
     
    QString filePath = "";

    QLocale::Language newLanguage = QLocale::system().language();
#if QT_DEPRECATED_SINCE(6, 6)
    QLocale::Territory newCountry= QLocale::system().territory();
#else
    QLocale::Country newCountry= QLocale::system().country();
#endif
    QLocale locale = QLocale(newLanguage, newCountry);
    
    locale.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale::setDefault(locale);


#ifdef DEBUG
    std::cout<<"in main_bash filePath ="<<filePath.toStdString();
#endif
    MainController* c = new MainController(filePath);
    (void) c;

    a.exec();
    
    delete c;
    
    return 0;

}
