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

#ifndef STUDYPERIODDIALOG_H
#define STUDYPERIODDIALOG_H

#include "StudyPeriodSettings.h"

#include <QDialog>
#include <QJsonObject>

class QLineEdit;
class QComboBox;
class QVBoxLayout;
class QGroupBox;
class QCheckBox;
class QDialogButtonBox;

class PluginFormAbstract;
class Collapsible;
class QRadioButton;
class QDoubleSpinBox;
class QLabel;
class HelpWidget;


class StudyPeriodDialog: public QDialog
{
    Q_OBJECT
public:
    StudyPeriodDialog(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Window);
    ~StudyPeriodDialog();

    void setStep(double step, bool forced, double suggested);
    void setSettings(const StudyPeriodSettings& s);
    StudyPeriodSettings getSettings() const;
    double step() const;
    bool forced() const;

protected slots:
    void setAdvancedVisible(bool visible);
    void updateVisibleControls();
    void setOkEnabled(const QString& text);
    void showMessageStepForced(bool forced);

signals:
    void wiggleChange();

public:
    QLabel* mMinLab;
    QLabel* mMaxLab;

    QLineEdit* mMinEdit;
    QLineEdit* mMaxEdit;

    Collapsible* mAdvanced;

    QCheckBox* mAdvancedCheck;
    QGroupBox* mAdvancedWidget;

    QLabel* mForcedLab;
    QCheckBox* mForcedCheck;
    QLabel* mStepLab;

    QDoubleSpinBox* mStepSpin;

    int mMargin;
    int mLineH;
    int mButW;
    int mButH;
    int mComboH;

    QVBoxLayout* mLayout;
    QDialogButtonBox* mButtonBox;
};

#endif
