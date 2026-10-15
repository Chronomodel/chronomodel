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

#ifndef SILVERMANDIALOG_H
#define SILVERMANDIALOG_H

#include "CurveUtilities.h"

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>

class SilvermanDialog:public QDialog
{
    Q_OBJECT
private:
    QDialogButtonBox* mButtonBox;

    QLabel* mDescriptionLabel;

    QLabel* mLambdaTypeLabel;
    QComboBox* mLambdaTypeInput;

    QLabel* mLambdaLabel;
    QLineEdit* mLambdaInput;

    QCheckBox* mUseErrMesureInput;

public:
    SilvermanParam *mParam;

public:
    SilvermanDialog(SilvermanParam* param, QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    virtual ~SilvermanDialog();
    void memo();
    void updateLayout();

protected slots:
    void validateInputs();


};

#endif // SILVERMANDIALOG_H
