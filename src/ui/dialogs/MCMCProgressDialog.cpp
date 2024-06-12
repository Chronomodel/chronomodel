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

MCMCProgressDialog::MCMCProgressDialog(MCMCLoop* loop, QWidget* parent, Qt::WindowFlags flags):QDialog(parent, flags),
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
