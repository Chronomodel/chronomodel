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

#include "AboutDialog.h"
#include <QtWidgets>


AboutDialog::AboutDialog(QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags)
{

    const QString text = "About " + qGuiApp->applicationDisplayName();//+ " Version: " + qApp->applicationVersion();
    setWindowTitle(text);

#ifdef Q_OS_MAC
    QString path  =  qApp->applicationDirPath();
    QDir dir(path);
    dir.cdUp();
    path = dir.absolutePath() + "/Resources/";
#else
    //http://doc.qt.io/qt-5/qstandardpaths.html#details
    //QStringList dataPath = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    //QString path  =  dataPath[0];
    QString path  = "";
#endif

    QFile htmlFile(path + "ABOUT.html");
    (void) htmlFile.open(QIODevice::ReadOnly);

    mText = new QTextEdit();
    mText->setTextInteractionFlags(Qt::TextBrowserInteraction);
    mText->setHtml(htmlFile.readAll());
    mText->setTextBackgroundColor(Qt::white);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(mText);
    setLayout(layout);
    //layout->setMargin(2);

    setMinimumSize(600, 600);
}

AboutDialog::~AboutDialog()
{

}

