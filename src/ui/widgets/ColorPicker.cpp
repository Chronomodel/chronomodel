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

#include "ColorPicker.h"

#include <QColorDialog>
#include <QApplication>
#include <QPainter>
#include <QPainter>
#include <QPainterPath>

Q_PROPERTY(QColor mColor READ getColor WRITE setColor)

ColorPicker::ColorPicker(const QColor &color, QWidget* parent, Qt::WindowFlags flags):
    QWidget(parent, flags),
    mColor(color)
{
    setFixedHeight(20);
}

ColorPicker::~ColorPicker()
{

}

void ColorPicker::setColor(const QColor& color)
{
    mColor = color;
    update();
}

QColor ColorPicker::getColor() const
{
    return mColor;
}

void ColorPicker::openDialog()
{
    QColor color = QColorDialog::getColor(mColor, qApp->activeWindow(), tr("Select Colour"));
    if (color.isValid()) {
        setColor(color);
        emit colorChanged(color);
    }
}

void ColorPicker::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(rect().adjusted(1, 1,  -1, - 1), 4, 4);
    painter.fillPath(path, isEnabled() ? mColor : QColor(220, 220, 220));
    painter.strokePath(path, QColor(160, 160, 160));
}

void ColorPicker::mousePressEvent(QMouseEvent* e)
{
    Q_UNUSED(e);
    if (isEnabled())
        openDialog();

}
