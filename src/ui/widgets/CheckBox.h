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

#ifndef CHECKBOX_H
#define CHECKBOX_H

#include <QCheckBox>

class CheckBox: public QCheckBox
{
    Q_OBJECT
public:
    explicit CheckBox(QWidget* parent = nullptr);
    explicit CheckBox(const QString& text, QWidget* parent = nullptr);
    ~CheckBox();

    QColor mDotColor;
    bool mHasDot;

    void setDotColor(const QColor& color) { mDotColor = color; mHasDot = true; update(); }

protected:
    bool mMouseOver;
    void paintEvent(QPaintEvent* e);
    void enterEvent(QEnterEvent *e);
    void leaveEvent(QEvent * e);
};

#endif
