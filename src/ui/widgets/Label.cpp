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

#include "Label.h"
#include "Painting.h"

#include <QApplication>

Label::Label(QWidget* parent):
    QLabel(parent),
    mIsTitle(false),
    mAdjustText(true)
{
    init();
}

Label::Label(const QString& text, QWidget* parent):
    QLabel(text, parent),
    mIsTitle(false)
{
    init();
}

Label::~Label()
{

}
void Label::adjustFont()
{
    if (!text().isEmpty() && mAdjustText) {
        const QFontMetrics fm (qApp->font());
        const QRect textRect = fm.boundingRect(text());
        const qreal wR = width() - 10;
        const qreal xfactor (textRect.width()> wR ? textRect.width()/wR : 1);
        const qreal yfactor (textRect.height()>height() ? textRect.height()/height() : 1) ;
        const qreal factor  = ( xfactor > yfactor ? xfactor : yfactor);
        QFont ft = qApp->font();
        ft.setPointSizeF(ft.pointSizeF()/factor);
        setFont(ft);
    }
}

void Label::init()
{
    setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    mPalette = qApp->palette();
    setFont(qApp->font());
}

void Label::setPalette(QPalette &palette)
{
    mPalette = palette;
}

void Label::setBackground(QColor color)
{
    mPalette.setColor(QPalette::Window, color);
}

void Label::setIsTitle(bool isTitle)
{
    mIsTitle = isTitle;

    if (mIsTitle) {
        mPalette.setColor(QPalette::Text, Qt::white);
        mPalette.setColor(QPalette::Window, Painting::mainColorGrey);
        setAlignment(Qt::AlignCenter);
     }
    else {
        mPalette = qApp->palette();
    }
}

void Label::setAdjustText(bool ad)
{
    mAdjustText = ad;
}
void Label::resizeEvent(QResizeEvent* e)
{
    (void) e;
    adjustFont();

}

void Label::paintEvent(QPaintEvent*)
{
   // adjustFont();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setFont(font());
    const QRectF r = rect();

    p.fillRect(r, mPalette.window().color());
    p.setPen(mPalette.text().color());

    p.drawText(r, alignment(), text());

}


void Label::setLight()
{
    mPalette.setColor(QPalette::Text, QColor(200, 200, 200));
}

void Label::setDark()
{
    mPalette.setColor(QPalette::Text, Qt::black);
}
