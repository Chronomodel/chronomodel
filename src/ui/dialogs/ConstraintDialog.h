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

#ifndef CONSTRAINTDIALOG_H
#define CONSTRAINTDIALOG_H

#include <QDialog>
#include <QJsonObject>

class Label;
class LineEdit;
class QComboBox;
class Button;


class ConstraintDialog: public QDialog
{
    Q_OBJECT
public:
    enum Type{
        eEvent = 0,
        ePhase = 1
    };

    ConstraintDialog(QWidget* parent = nullptr, Type type = eEvent, Qt::WindowFlags flags =  Qt::Widget);
    ~ConstraintDialog();

    void setConstraint(const QJsonObject& constraint);
    QJsonObject constraint() const;
    bool deleteRequested() const;

protected slots:
    void deleteConstraint();
    void showAppropriateOptions();

protected:
    void resizeEvent(QResizeEvent* e);

private:
    QJsonObject mConstraint;
    Type mType;
    bool mDeleteRequested;

    Label* mTypeLab;
    Label* mFixedLab;
    //Label* mMinLab;
    //Label* mMaxLab;

    QComboBox* mTypeCombo;
    LineEdit* mFixedEdit;
    //LineEdit* mMinEdit;
    //LineEdit* mMaxEdit;

    Button* mOkBut;
    Button* mCancelBut;
    Button* mDeleteBut;

    qreal mComboH;
};

#endif
