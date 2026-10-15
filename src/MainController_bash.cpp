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

#include "MainController_bash.h"

#include "PluginManager.h"
#include "MainWindow.h"
#include "Painting.h"


MainController::MainController(const QString& filePath)
{
    QVersionNumber version (VERSION_NUMBER);
    Painting::init();
    PluginManager::loadPlugins();
    QCoreApplication::setApplicationName("ChronoModel_bash");
    QCoreApplication::setApplicationVersion(version.toString());
    QCoreApplication::setOrganizationDomain("https://www.chronomodel.com");
    QCoreApplication::setOrganizationName("CNRS");

    mMainWindow = MainWindow::getInstance();
    try {
       // AppSettings();
        AppSettings::readSettings();
        QString path;
       if (filePath != "" ) {
              path = filePath;

       } else  if ((filePath == "" ) && AppSettings::mOpenLastProjectAtLaunch) {
           const QString dir = AppSettings::mLastDir;
           const QString filename = AppSettings::mLastFile;
           if ((dir != "") && (filename !=""))
               path = dir + "/" + filename;

       }


        mMainWindow->readSettings(path); // le problème est ici pour macOS

        mMainWindow->move(AppSettings::mLastPosition);
        mMainWindow->resize(AppSettings::mLastSize);

    }  catch(...) {
        qDebug() << "MainController:: Caught Exception!\n";
    }

    mMainWindow->show();

}
