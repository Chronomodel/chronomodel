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

#include "SwitchAction.h"
#include "Painting.h"
#include <QtWidgets>

SwitchWidget::SwitchWidget(QWidget* parent):
    QWidget(parent),
    mAction(nullptr),
    mText ("None"),
    mToggled(false)
{
    //setFixedSize(90, 40);

}

SwitchWidget::SwitchWidget(QWidget* parent, QWidgetAction* action):
    QWidget(parent),
    mAction(action),
    mToggled(false)
{
    //setFixedSize(90, 40);
}

void SwitchWidget::setToggled(bool isToggled)
{
    if (isToggled != mToggled) {
        mToggled = isToggled;
        if (mAction) {
            mAction->toggle();
        }
        emit toggled(isToggled);
        update();
    }
}

void SwitchWidget::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);
    setToggled(!mToggled);
}


void SwitchWidget::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRectF r = rect();
    r.adjust(1, 1, -1, -1);
    
    QColor colorBorder = mToggled ? CURVE_COLOR_BORDER : CHRONOMODEL_COLOR_BORDER;
    QColor colorBack = mToggled ? CURVE_COLOR_BACK : CHRONOMODEL_COLOR_BACK;
    QColor colorText = mToggled ? CURVE_COLOR_TEXT : CHRONOMODEL_COLOR_TEXT;
    
    QPen pen = painter.pen();
    pen.setColor(colorBorder);
    pen.setWidth(2);
    painter.setPen(pen);
    painter.setBrush(colorBack);
    painter.drawRoundedRect(r, 4, 4);
    
    pen.setColor(colorText);
    painter.setPen(pen);
    
    QFont f1(font());
    f1.setPointSizeF(8);
    painter.setFont(f1);
    //QFontMetrics fm1(f1);
    QString text1 = tr("ChronoCurve");
    //int text1W = fm1.horizontalAdvance(text1);
    
    QFont f2(font());
    f2.setPointSizeF(14);
    f2.setWeight(QFont::Bold);
    painter.setFont(f2);
    
    painter.setFont(f1);
    painter.drawText(r.adjusted(0, 5, 0, -2*r.height()/3 + 2), Qt::AlignHCenter | Qt::AlignVCenter, text1);
    
    painter.setFont(f2);
    painter.drawText(r.adjusted(0, r.height()/3, 0, 0), Qt::AlignHCenter | Qt::AlignVCenter, mText);
}


SwitchAction::SwitchAction(QObject* parent):QWidgetAction(parent)
{
    
}

QWidget* SwitchAction::createWidget(QWidget* parent)
{
    return new SwitchWidget(parent, this);
    //return QWidgetAction::createWidget(parent);
}
