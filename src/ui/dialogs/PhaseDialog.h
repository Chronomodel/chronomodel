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

#ifndef PHASEDIALOG_H
#define PHASEDIALOG_H

#include <QDialog>
#include <QJsonObject>
#include <QString>
#include <QLabel>

class Button;
class LineEdit;
class QComboBox;
class ColorPicker;


class PhaseDialog: public QDialog
{
    Q_OBJECT
public:


    QLabel* mNameLab;
    QLabel* mColorLab;
    QLabel* mTauTypeLab;
    QLabel* mTauFixedLab;

    LineEdit* mNameEdit;
    ColorPicker* mColorPicker;
    QComboBox* mTauTypeCombo;
    LineEdit* mTauFixedEdit;

    QString mError;

public:
    PhaseDialog(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Window);
    ~PhaseDialog();

    void setPhase(QJsonObject phaseObj);
    QJsonObject getPhase();

    bool isValid();

protected slots:
    void showAppropriateTauOptions();

private :
    QJsonObject _data;
};

#endif
