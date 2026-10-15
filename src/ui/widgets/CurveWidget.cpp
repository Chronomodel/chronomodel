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

#include "CurveWidget.h"
#include "Painting.h"



CurveWidget::CurveWidget(QWidget* parent):
    QWidget(parent)
{

}

void CurveWidget::paintEvent(QPaintEvent* )
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRectF r = rect();
    r.adjust(1, 1, -1, -1);
    
    QPen pen = painter.pen();
    pen.setColor(CURVE_COLOR_BORDER);
    pen.setWidth(2);
    
    painter.setPen(pen);
    painter.setBrush(CURVE_COLOR_BACK);
    painter.drawRoundedRect(r, 4, 4);
}
