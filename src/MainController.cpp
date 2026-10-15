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

#include "MainController.h"

#include "PluginManager.h"
#include "MainWindow.h"
#include "Painting.h"
#include "version.h"

#include <iostream>
#include <qversionnumber.h>
#include <QtCore/QCoreApplication>

MainController::MainController(const QString& filePath)
{
    QVersionNumber version = QVersionNumber(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    std::cout << "[MainController] version " << version.toString().toStdString() << std::endl;

    Painting::init();
    PluginManager::loadPlugins();
    QCoreApplication::setApplicationName("ChronoModel"); // correspond à qApp->applicationName()
    QCoreApplication::setApplicationVersion (version.toString());
    QCoreApplication::setOrganizationDomain("https://www.chronomodel.com");
    QCoreApplication::setOrganizationName("CNRS");

    mMainWindow = MainWindow::getInstance();

    //std::cout<<"Inside [MainController::MainController]" <<std::endl;

    try {
AppSettings::readSettings();
        mMainWindow->move(AppSettings::mLastPosition);
        mMainWindow->resize(AppSettings::mLastSize);
mMainWindow->show();


        QString path;
        if (filePath != "" ) {
              path = filePath;

        } else  if ((filePath == "" ) && AppSettings::mOpenLastProjectAtLaunch) {
           const QString dir = AppSettings::mLastDir;
           const QString filename = AppSettings::mLastFile;
           if ((dir != "") && (filename !=""))
               path = dir + "/" + filename;

        }

        mMainWindow->readSettings(path); //do MCMCFinish

        //mMainWindow->move(AppSettings::mLastPosition);
        //mMainWindow->resize(AppSettings::mLastSize);

    }  catch(...) {
        std::cout << "[MainController] Caught Exception!" << std::endl;
    }

  //  mMainWindow->show();

}
