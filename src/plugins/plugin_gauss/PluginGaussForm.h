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

#ifndef PLUGINGAUSSFORM_H
#define PLUGINGAUSSFORM_H

#if USE_PLUGIN_GAUSS

#include "PluginFormAbstract.h"

class PluginGauss;
class QLineEdit;
class QLabel;
class QRadioButton;
class QComboBox;


class PluginGaussForm: public PluginFormAbstract
{
    Q_OBJECT
public:
    PluginGaussForm(PluginGauss* plugin, QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    virtual ~PluginGaussForm();

    void setData(const QJsonObject& data, bool isCombined);
    QJsonObject getData();

    bool isValid();


protected slots:
    void updateVisibleElements();
    void errorIsValid(QString str);
    void equationIsValid();

    void validOK();

private:
    QLabel* mAverageLab;
    QLabel* mErrorLab;
    QLabel* mCalibLab;

    QLineEdit* mAverageEdit;
    QLineEdit* mErrorEdit;

    QWidget* mEqWidget;
    QLabel* mEq1Lab;
    QLabel* mEq2Lab;
    QLabel* mEq3Lab;
    QLineEdit* mAEdit;
    QLineEdit* mBEdit;
    QLineEdit* mCEdit;

    QRadioButton* mEquationRadio;
    QRadioButton* mCurveRadio;
    QRadioButton* mNoneRadio;

    QComboBox* mCurveCombo;
    int mComboH;
};

#endif
#endif
