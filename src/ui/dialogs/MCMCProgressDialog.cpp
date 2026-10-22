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
    ProgressHook* self = this;
    progressHook().compare_exchange_strong(self, nullptr);
}

int MCMCProgressDialog::startMCMC()
{
#ifdef _WIN32
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED); //https://learn.microsoft.com/fr-fr/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate?redirectedfrom=MSDN
#endif
    mPhaseFrom.store(0);
    mPhaseTo.store(kComputeEnd);
    mStageFrom.store(-1);
    mLastValue.store(-1);
    mGuiPhase = false;
    progressHook().store(this, std::memory_order_release);

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

    if (mLoop->mAbortedReason.isEmpty() && mPostProcess) {
        // Phase "graphes" : exécutée ici, dans le thread GUI, dialogue toujours affiché.
        // La barre continue sur [kComputeEnd, kPostProcessRange].
        mGuiPhase = true;
        mStageFrom.store(-1);
        mPhaseFrom.store(std::max(kComputeEnd, mLastValue.load()));
        mPhaseTo.store(kPostProcessRange);
        mPumpTimer.invalidate();

        mPostProcess();

        postProgress(kPostProcessRange);
        mGuiPhase = false;
    }

    ProgressHook* self = this;
    progressHook().compare_exchange_strong(self, nullptr);

    accept();
}

// ---------------------------------------------------------------------------
// ProgressHook
// ---------------------------------------------------------------------------
void MCMCProgressDialog::stage(const QString& label, double from, double to)
{
    const int a = mPhaseFrom.load();
    const int b = mPhaseTo.load();
    const int sFrom = a + qRound((b - a) * std::clamp(from, 0.0, 1.0));
    const int sTo   = a + qRound((b - a) * std::clamp(to,   0.0, 1.0));
    mStageFrom.store(sFrom);
    mStageTo.store(sTo);

    if (QThread::currentThread() == thread())
        showLabel(label);
    else
        QMetaObject::invokeMethod(this, [this, label] { showLabel(label); }, Qt::QueuedConnection);

    postProgress(sFrom);
}

void MCMCProgressDialog::advance(std::size_t done, std::size_t total)
{
    const int sFrom = mStageFrom.load();
    if (sFrom < 0 || total == 0)
        return; // aucune étape ouverte (ex. parallel_run hors post-traitement)

    const int sTo = mStageTo.load();
    const double f = static_cast<double>(std::min(done, total)) / static_cast<double>(total);
    postProgress(sFrom + static_cast<int>((sTo - sFrom) * f));
}

void MCMCProgressDialog::postProgress(int value)
{
    // Monotone et dédoublonné : au plus kPostProcessRange mises à jour, quel que soit
    // le nombre de tâches ; un parallel_run imbriqué ne fait jamais reculer la barre.
    int prev = mLastValue.load();
    do {
        if (value <= prev)
            return;
    } while (!mLastValue.compare_exchange_weak(prev, value));

    if (QThread::currentThread() == thread())
        showProgress(value);
    else
        QMetaObject::invokeMethod(this, [this, value] { showProgress(value); }, Qt::QueuedConnection);
}

void MCMCProgressDialog::showLabel(const QString& label)
{
    setMessage(label);
    pumpEvents();
}

void MCMCProgressDialog::showProgress(int value)
{
    if (mProgressBar1->maximum() != kPostProcessRange)
        mProgressBar1->setRange(0, kPostProcessRange);
    mProgressBar1->setValue(value);
    pumpEvents();
}

void MCMCProgressDialog::pumpEvents()
{
    // En phase "graphes", le thread GUI est occupé : sans cela, ni la barre ni le
    // libellé ne seraient redessinés (et macOS afficherait le curseur d'attente).
    // Même principe que QProgressDialog::setValue() pour un dialogue modal.
    if (!mGuiPhase)
        return;
    if (mPumpTimer.isValid() && mPumpTimer.elapsed() < 40)
        return;
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    mPumpTimer.start();
}

void MCMCProgressDialog::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        e->ignore();
    } else {
        QDialog::keyPressEvent(e);
    }
}
