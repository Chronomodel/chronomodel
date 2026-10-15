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

#include "HelpWidget.h"

#include <QLabel>
#include <QPainter>

HelpWidget::HelpWidget(QWidget* parent):QWidget(parent),
    mText(QString())
{
    construct();
}

HelpWidget::HelpWidget(const QString &text, QWidget* parent):QWidget(parent),
    mText(text)
{
    construct();
}

void HelpWidget::construct()
{
    mFont = font();

    mHyperLink = new QLabel(this);
    mHyperLink->setTextFormat(Qt::RichText);
    mHyperLink->setFont(mFont);
    mHyperLink->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    mHyperLink->setTextInteractionFlags(Qt::TextBrowserInteraction);
    mHyperLink->setOpenExternalLinks(true);

    // Not yet supported with retina display in Qt 5.3
#ifndef Q_OS_MAC
   /* QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
    shadow->setColor(Qt::black);
    shadow->setBlurRadius(4);
    shadow->setOffset(1, 1);
    setGraphicsEffect(shadow);
    */
#endif
}

HelpWidget::~HelpWidget()
{

}

void HelpWidget::setText(const QString &text)
{
    mText = text;
    update();
}

void HelpWidget::setLink(const QString &url)
{
    mHyperLink->setText("<a href=\"" + url + "\">More...</a>");
}

int HelpWidget::heightForWidth(const int w) const
{
    const QFontMetrics metrics(mFont);
    const QRect rect = metrics.boundingRect(QRect(0, 0, w - 10, 1000),
                                      Qt::TextWordWrap | Qt::AlignVCenter | Qt::AlignLeft,
                                      mText);
    return rect.height() + 10 + 5 + mFont.pointSize(); // 15 is the height of the link, and 5 its margin
}

void HelpWidget::paintEvent(QPaintEvent* )
{
    const QRectF r = rect().adjusted(1, 1, -1, -1);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QColor(187, 174, 105));
    p.setBrush(QColor(242, 238, 184));
    p.drawRoundedRect(r, 4, 4);

    p.setPen(QColor(104, 74, 64));
    p.setFont(mFont);
    QTextOption options;
    options.setWrapMode(QTextOption::WordWrap);
    options.setAlignment(Qt::AlignLeft);// | Qt::AlignVCenter);
    p.drawText(rect().adjusted(5, 5, -5, -5), mText, options);
}

void HelpWidget::resizeEvent(QResizeEvent*)
{
    const QFontMetrics metrics(mFont);
    mHyperLink->setFont(mFont);
    mHyperLink->setGeometry(5, height() - 2*mFont.pointSize() -5, width() - metrics.horizontalAdvance("More..") -5, 2*mFont.pointSize());
}
