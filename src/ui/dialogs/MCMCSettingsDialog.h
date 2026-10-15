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

#ifndef MCMCSETTINGSDIALOG_H
#define MCMCSETTINGSDIALOG_H

#include <QDialog>
#include <QLabel>
#include "MCMCSettings.h"

class Label;
class LineEdit;
class Button;
class QDialogButtonBox;
class QSpinBox;
class HelpWidget;


class MCMCSettingsDialog: public QDialog
{
    Q_OBJECT
public:
    MCMCSettingsDialog(QWidget* parent = nullptr, const bool show_help = true);
    virtual ~MCMCSettingsDialog();

    void setSettings(const MCMCSettings &settings);
    MCMCSettings getSettings();

protected slots:
    void inputControl();
    void reset();
    void setQuickTest();

signals:
    void inputValided();
    void inputRejected();
    void nothing();

protected:
    void paintEvent(QPaintEvent*);
    void resizeEvent(QResizeEvent* e);
    void updateLayout();

private:
    // Simulated Annealing widgets
    QLabel*   mTitleAnnealLabel;
    QLabel*   mAnnealTempLabel;
    QLabel*   mAnnealRecurrenceLabel;
    QLabel*   mAnnealDwellLabel;
    LineEdit* mAnnealTempEdit;
    LineEdit* mAnnealRecurrenceEdit;
    LineEdit* mAnnealDwellEdit;
    QRectF    mAnnealRect;

    // Dimension
   MCMCSettings initalSetting;
   int mTop;
   int mColoredBoxHeigth; // heigth of the colored box
   int mBurnBoxWidth;
   int mAdaptBoxWidth;// total width of the colored boxes
   int mAcquireBoxWidth;

   int mBottom;

   int mLineH;
   int mEditW;

   int mButW ;
   int mButH ; // size of the button OK and Cancel
   int mMarginW;
   int mMarginH;

   int mTotalWidth;
   // Object

    QLabel *mNumProcLabel;
    LineEdit* mNumProcEdit;

    QLabel *mTitleBurnLabel;
    QLabel *mIterBurnLabel;
    LineEdit* mNumBurnEdit;

    LineEdit* mNumIterEdit;
    LineEdit* mMaxBatchesEdit;
    LineEdit* mIterPerBatchEdit;
    LineEdit* mDownSamplingEdit;

    QLabel *mSeedsLabel;
    LineEdit* mSeedsEdit;
    HelpWidget* mHelp;

    QLabel* mLevelLabel;
    LineEdit* mLevelEdit;

    QPushButton* mOkBut;
    QPushButton* mCancelBut;

    QPushButton* mTestBut;
    QPushButton* mResetBut;

    QRectF mBurnRect;
    QRectF mAdaptRect;
    QRectF mAquireRect;
    QRectF mBatch1Rect;
    QRectF mBatchInterRect;
    QRectF mBatchNRect;


};

#endif
