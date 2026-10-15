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

#include "MCMCProgressDialog.h"

#include "MCMCLoop.h"
#include "AppSettings.h"

#include <QtWidgets>

// to prevent windows to stop for energy
// https://learn.microsoft.com/fr-fr/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate?redirectedfrom=MSDN

#ifdef _WIN32
//#include "winbase.h"
#include <windows.h> //for Qt 6.7
#endif

MCMCProgressDialog::MCMCProgressDialog(MCMCLoop* loop, QWidget* parent, Qt::WindowFlags flags):
    QDialog(parent, flags),
    mLoop(loop)
{
    setWindowTitle(tr("MCMC in progress..."));

    // -----------
    mLabel1 = new QLabel(this);
    mLabel2 = new QLabel(this);

    mProgressBar1 = new QProgressBar(this);
    mProgressBar1->setMinimum(0);
    mProgressBar1->setMaximum(0);
    mProgressBar1->setMinimumWidth(10 * AppSettings::widthUnit());

    // ----------

    QDialogButtonBox* buttonBox = new QDialogButtonBox();
    mCancelBut = buttonBox->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);

    // ----------

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(mLabel1);
    layout->addWidget(mProgressBar1);
    layout->addWidget(buttonBox);
    setLayout(layout);

    // -----------

    connect(mCancelBut, &QPushButton::clicked, this, &MCMCProgressDialog::cancelMCMC);
    connect(mLoop, &MCMCLoop::setMessage, this, &MCMCProgressDialog::setMessage);
    connect(mLoop, &MCMCLoop::finished, this, &MCMCProgressDialog::setFinishedState);
    connect(mLoop, &MCMCLoop::stepChanged, this, &MCMCProgressDialog::setTitle1);
    connect(mLoop, &MCMCLoop::stepProgressed, this, &MCMCProgressDialog::setProgress1);
}

MCMCProgressDialog::~MCMCProgressDialog()
{

}

int MCMCProgressDialog::startMCMC()
{
#ifdef _WIN32
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED); //https://learn.microsoft.com/fr-fr/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate?redirectedfrom=MSDN
#endif
    mLoop->start(QThread::QThread::NormalPriority);// TimeCriticalPriority);// NormalPriority);HighestPriority
    return exec();
}

void MCMCProgressDialog::cancelMCMC()
{
    mLoop->requestInterruption();
}

void MCMCProgressDialog:: setMessage(const QString& message)
{
    mLabel1->setText(message);
    setMinimumWidth(int (fontMetrics().horizontalAdvance(message) * 1.5));
}

void MCMCProgressDialog::setTitle1(const QString& message, int minProgress, int maxProgress)
{
    mLabel1->setText(message);
    setMinimumWidth(int (fontMetrics().horizontalAdvance(message) * 1.5));

    mProgressBar1->setMinimum(minProgress);
    mProgressBar1->setMaximum(maxProgress);
}

void MCMCProgressDialog::setProgress1(int value)
{
    mProgressBar1->setValue(value);
}

void MCMCProgressDialog::setTitle2(const QString& message, int minProgress, int maxProgress)
{
    mLabel2->setText(message);
    mProgressBar2->setMinimum(minProgress);
    mProgressBar2->setMaximum(maxProgress);
}

void MCMCProgressDialog::setProgress2(int value)
{
    mProgressBar2->setValue(value);
}

void MCMCProgressDialog::setFinishedState()
{
    mCancelBut->setEnabled(false);
    accept();
}

void MCMCProgressDialog::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        e->ignore();
    } else {
        QDialog::keyPressEvent(e);
    }
}
