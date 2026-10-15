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

#ifndef MCMCPROGRESSDIALOG_H
#define MCMCPROGRESSDIALOG_H

#include <QDialog>

class MCMCLoop;
class QLabel;
class QProgressBar;
class QTextEdit;
class QPushButton;


class MCMCProgressDialog: public QDialog
{
    Q_OBJECT
public:
    MCMCProgressDialog(MCMCLoop* loop, QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Window);
    ~MCMCProgressDialog();

    int startMCMC();

public slots:
    void cancelMCMC();
    //void addMessage(QString message);

    void setTitle1(const QString& message, int minProgress, int maxProgress);
    void setProgress1(int value);

    void setTitle2(const QString& message, int minProgress, int maxProgress);
    void setProgress2(int value);

    void setFinishedState();
    void setMessage(const QString& message);

protected:
    void keyPressEvent(QKeyEvent* e);

public:
    MCMCLoop* mLoop;
    QLabel* mLabel1;
    QLabel* mLabel2;
    QProgressBar* mProgressBar1;
    QProgressBar* mProgressBar2;
   // QPushButton* mOKBut;
    QPushButton* mCancelBut;
};

#endif
