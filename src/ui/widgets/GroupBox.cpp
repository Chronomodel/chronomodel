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

#include "GroupBox.h"

#include <QPainter>

int GroupBox::sTitleHeight = 30;

GroupBox::GroupBox(const QString& title, QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mMargin(5),
mLineH(20),
mTitle(title)
{

}

GroupBox::~GroupBox()
{

}

void GroupBox::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);

    p.setPen(QColor(100, 100, 100));
    p.setBrush(QColor(180, 180, 180));
    p.drawRect(rect());

    p.fillRect(0, 0, width(), sTitleHeight, QColor(100, 100, 100));
    p.setPen(QColor(200, 200, 200));
    p.drawText(mMargin, mMargin, width() - 2*mMargin, sTitleHeight - 2*mMargin, Qt::AlignVCenter | Qt::AlignLeft, mTitle);
}
