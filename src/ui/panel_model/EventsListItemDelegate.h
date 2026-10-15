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

#ifndef EVENTSLISTITEMDELEGATE_H
#define EVENTSLISTITEMDELEGATE_H

#include "Event.h"
#include "../PluginAbstract.h"
#include "Painting.h"
#include "ModelUtilities.h"

#include <QItemDelegate>
#include <QtWidgets>

class EventsListItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    inline EventsListItemDelegate(QObject* parent = 0):QItemDelegate(parent){}

    inline QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex&) const
    {
        QFont font = option.font;
        font.setPointSizeF(pointSize(11));
        QFontMetrics metrics(font);

        int h = metrics.lineSpacing();
        return QSize(option.rect.width(), 2 * h + 6);
    }

    inline void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        //QItemDelegate::paint(painter, option, index);

        int x = option.rect.x();
        int y = option.rect.y();
        int w = option.rect.width();
        int h = option.rect.height();
        int iconW = 30;
        int iconH = 20;
        const int marginW = 5;

        painter->setRenderHint(QPainter::Antialiasing);

        if (option.state & QStyle::State_Selected) {
            //painter->fillRect(option.rect, option.palette.highlight());
            painter->fillRect(option.rect, QColor(220, 220, 220));
        }
        QString factName = index.model()->data(index, 0x0101).toString();
        QTextDocument td;
        td.setHtml(factName);

        int numDates = index.model()->data(index, 0x0103).toInt();
        //QString numDates = index.model()->data(index, 0x0103).toString();
        QColor factColor = QColor(index.model()->data(index, 0x0104).toInt(),
                                  index.model()->data(index, 0x0105).toInt(),
                                  index.model()->data(index, 0x0106).toInt());

        painter->setPen(Qt::black);
        painter->setBrush(factColor);
        if (numDates < 0) { 
            painter->drawEllipse(x + marginW, y + (h - iconH)/2, iconW, iconH);

        } else {
            painter->fillRect(x + marginW, y + (h - iconH)/2, iconW, iconH, factColor);
            painter->drawRect(x + marginW, y + (h - iconH)/2, iconW, iconH);

        }


        QFont font = option.font;
        font.setPointSizeF(pointSize(11));
        painter->setFont(font);

        painter->setPen(Qt::black);
        painter->drawText( x + 2*marginW + iconW, y, w - iconW - 2*marginW, h/2, Qt::AlignLeft | Qt::AlignVCenter, factName);

        if (numDates < 0) {
            painter->drawText(x + 2*marginW + iconW, y + h/2, w - iconW - 2*marginW, h/2, Qt::AlignLeft | Qt::AlignVCenter, " Bound");

        } else {
            painter->drawText(x + 2*marginW + iconW, y + h/2, w - iconW - 2*marginW, h/2, Qt::AlignLeft | Qt::AlignVCenter, QString::number(numDates) + " Data");
        }

        painter->setPen(QColor(200, 200, 200));
        painter->drawLine(x, y + h, x + w, y + h);
    }
};

#endif
