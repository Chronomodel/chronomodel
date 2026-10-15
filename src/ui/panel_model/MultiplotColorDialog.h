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

#ifndef MULTIPLOTCOLORWIDGET_H
#define MULTIPLOTCOLORWIDGET_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QColorDialog>
//#include <QToolButton>
#include <QRadioButton>
#include <QDialogButtonBox>
#include <QButtonGroup>

class MultiplotColorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MultiplotColorDialog(QWidget *parent = nullptr);
    MultiplotColorDialog(bool usePluginColor, bool useEventColor, QColor selectedColor, QWidget *parent = nullptr);

    virtual ~MultiplotColorDialog() {};

    QColor getColor() const { return mSelectedColor;}
    bool usePluginColor() const { return mUsePluginColor;}
    bool useEventColor() const { return mUseEventColor;}
    bool useCustomColor() const { return !mUsePluginColor & !mUseEventColor;}

private slots:
    void selectColor();
    void updateOptions();

private:
    QColor mSelectedColor;
    bool mUsePluginColor;
    bool mUseEventColor;

    QPushButton *colorDialogButton;

    QRadioButton *mPluginRadio;
    QRadioButton *mEventRadio;
    QRadioButton *mCustomRadio;

    QButtonGroup *mRadioGroup;

    QDialogButtonBox *okButton;
};

#endif // MULTIPLOTCOLORWIDGET_H
