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

#include "MainController.h"
#include "PluginManager.h"
#include "MainWindow.h"
#include "Painting.h"

MainController::MainController(const QString& filePath)
{
    Painting::init();
    PluginManager::loadPlugins();
    QCoreApplication::setApplicationName("ChronoModel");
    QCoreApplication::setApplicationVersion("3.1-alpha"); // must match value in Chronomodel.pro
    QCoreApplication::setOrganizationDomain("http://www.chronomodel.com");
    QCoreApplication::setOrganizationName("CNRS");

    AppSettings::readSettings();

    mMainWindow = MainWindow::getInstance();
    try {
        mMainWindow->readSettings(filePath); // le problème est ici pour macOS

        mMainWindow->move(AppSettings::mLastPosition);
        mMainWindow->resize(AppSettings::mLastSize);

    }  catch(...) {
        qDebug() << "MainController:: Caught Exception!\n";
    }

    mMainWindow->show();

}
