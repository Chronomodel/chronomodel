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

#include "Marker.h"

#include <QPainter>

Marker::Marker(QWidget* parent, Qt::WindowFlags flags):
    QWidget(parent, flags),
    mPen(Qt::red),
    mBrush(Qt::red)
{
    mPen.setStyle(Qt::SolidLine);
    mBrush.setStyle(Qt::SolidPattern);

    setMouseTracking(false);
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

Marker::~Marker()
{

}
void Marker::hideMarker()
{
    QWidget::setVisible(false);
    update();
}
void Marker::showMarker()
{
    QWidget::setVisible(true);
    update();
}

int Marker::thickness() const
{
    return 1;
}

void Marker::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.setPen(mPen);
    p.setBrush(mBrush);
    p.drawRect(rect());

}
