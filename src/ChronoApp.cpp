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
#include "MainWindow.h"

#include <QtWidgets>

ChronoApp::ChronoApp(int& argc, char** argv) : QApplication(argc, argv)
{
#ifdef DEBUG
    std::cout << "start ChronoApp" << std::endl;
#endif
}

ChronoApp::~ChronoApp()
{

}
/**
 * @todo Handle file drag directly onto the application icon on Mac
 */
bool ChronoApp::event(QEvent* e)
{

    if (e->type() == QEvent::FileOpen) {
        QFileOpenEvent* foe = static_cast<QFileOpenEvent*>(e);
        QString path = foe->file();

#ifdef DEBUG
        std::cout << "in ChronoApp::event path = " << path.toStdString() << std::endl;
#endif
        MainWindow* w = MainWindow::getInstance();

        if (w) {
            w->readSettings(path);
        } else {
            qCritical() << "MainWindow instance is null in ChronoApp::event";
        }

        // Retourner true pour indiquer que l'événement a été traité
        return true;

    }

    // Appele la méthode de base pour les autres événements
    return QApplication::event(e); // et non QGuiApplication : QApplication::event gère le timer des toolTips
}
