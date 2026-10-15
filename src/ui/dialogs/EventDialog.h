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

#ifndef EVENTDIALOG_H
#define EVENTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QComboBox>

class LineEdit;
class ColorPicker;


class EventDialog: public QDialog
{
    Q_OBJECT

    LineEdit* mNameEdit;
    ColorPicker* mColorPicker;

#pragma mark Function
public:
    EventDialog(QWidget* parent, const QString& title, Qt::WindowFlags flags =  Qt::WindowType::Widget);
    ~EventDialog();


    QString getName() const;
    QColor getColor() const;


};

#endif
