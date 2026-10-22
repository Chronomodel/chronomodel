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

#include "ParallelFor.h" // ProgressHook

#include <QDialog>
#include <QElapsedTimer>

#include <atomic>
#include <functional>

class MCMCLoop;
class QLabel;
class QProgressBar;
class QTextEdit;
class QPushButton;


class MCMCProgressDialog: public QDialog, public ProgressHook
{
    Q_OBJECT
public:
    MCMCProgressDialog(MCMCLoop* loop, QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Window);
    ~MCMCProgressDialog();

    int startMCMC();

    // Travail exécuté dans le thread GUI à la fin du calcul (résultats, graphes),
    // dialogue toujours visible ; n'est pas appelé si le calcul est annulé ou en erreur.
    void setPostProcess(std::function<void()> postProcess) { mPostProcess = std::move(postProcess); }

    // ProgressHook : progression du post-traitement (thread MCMC, threads du pool, thread GUI)
    void stage(const QString& label, double from, double to) override;
    void advance(std::size_t done, std::size_t total) override;

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
    void keyPressEvent(QKeyEvent* e) override;

public:
    MCMCLoop* mLoop;
    QLabel* mLabel1;
    QLabel* mLabel2;
    QProgressBar* mProgressBar1;
    QProgressBar* mProgressBar2;
   // QPushButton* mOKBut;
    QPushButton* mCancelBut;

private:
    void postProgress(int value);         // tout thread
    void showProgress(int value);         // thread GUI
    void showLabel(const QString& label); // thread GUI
    void pumpEvents();                    // thread GUI, phase "graphes" seulement

    // Barre de post-traitement sur [0, kPostProcessRange] :
    // phase "calcul" (thread MCMC) sur [0, kComputeEnd], phase "graphes" (thread GUI) ensuite.
    static constexpr int kComputeEnd = 850;

    std::function<void()> mPostProcess;
    std::atomic<int> mPhaseFrom {0};
    std::atomic<int> mPhaseTo {kComputeEnd};
    std::atomic<int> mStageFrom {-1};
    std::atomic<int> mStageTo {-1};
    std::atomic<int> mLastValue {-1};
    bool mGuiPhase = false;
    QElapsedTimer mPumpTimer;
};

#endif
