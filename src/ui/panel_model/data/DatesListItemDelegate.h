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

#ifndef DATESLISTITEMDELEGATE_H
#define DATESLISTITEMDELEGATE_H

#include "AppSettings.h"
#include "PluginManager.h"
#include "PluginAbstract.h"
#include "Painting.h"

#include <QItemDelegate>
#include <QtWidgets>

class DatesListItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    inline DatesListItemDelegate(QObject* parent = nullptr):QItemDelegate(parent){}

    inline QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex&) const
    {
        QFontMetrics metrics(qApp->font());

        const int mm (2);
        int mh = metrics.height();
        return QSize(option.rect.width(), 4*mh + 5*mm);
    }

   inline  QRectF boundingRect() const
    {
       QFontMetrics metrics(qApp->font());

       const int mm (2);
       int mh = metrics.height() *5;
       return QRectF(0, 0, 100, 4*mh + 5*mm);

    }

    inline void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        int mm (2);
        int x = option.rect.x();
        int y = option.rect.y();
        int w = option.rect.width();
        int h = option.rect.height();

        int iconW (30);
        int iconS (20);

        painter->setRenderHint(QPainter::Antialiasing);

        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, QColor(230, 230, 230));
            painter->setPen(QColor(245, 245, 245));
            painter->drawLine(x, y, x + w, y);
            painter->setPen(QColor(225, 225, 225));
            painter->drawLine(x, y + h -1, x + w, y + h -1);
        }
        QString dateName = index.model()->data(index, 0x0101).toString();
        QString pluginId = index.model()->data(index, 0x0102).toString();
        QString dateDesc = index.model()->data(index, 0x0103).toString();
        // item->setData(0x0104, d.mId););
        QString deltaDesc = index.model()->data(index, 0x0105).toString();
        QString dateMethodStr = index.model()->data(index, 0x0106).toString();
        bool isValid = index.model()->data(index, 0x0107).toBool();
        bool isCombined = index.model()->data(index, 0x0108).toBool();
        bool dateOrigin = index.model()->data(index, 0x0109).toInt();
        /*
         item->setData(0x0110, d.mUUID);
         item->setData(0x0111, d.getWiggleDesc());
         */

        PluginAbstract* plugin = PluginManager::getPluginFromId(pluginId);

        if (plugin)  {
            const int im (5);
            int is = (h - 3*im)/2;
            int ix = x + w - im - is;
            int iy = y + im;

            painter->save();
            QFont font = qApp->font();

            if (isCombined) {
                font.setPointSize( qApp->font().pixelSize()* 1.5);
                painter->setFont(font);
                painter->setBrush(Painting::mainColorDark);
                painter->setPen(Painting::mainColorDark);
                painter->drawEllipse(ix, iy + is + im, is, is);
                painter->setPen(Qt::white);
                painter->drawText(ix, iy + is + im, is, is, Qt::AlignCenter, "C");
            }
            painter->restore();

            QIcon icon = plugin->getIcon();
            QPixmap pixmap = icon.pixmap(iconS, iconS);
            painter->drawPixmap(x + (iconW - iconS)/2, y + (h - iconS)/2, iconS, iconS, pixmap, 0, 0, pixmap.width(), pixmap.height());

            painter->setFont(qApp->font());
            QFontMetrics metrics(qApp->font());

            int mh = metrics.height();

            painter->setPen(Qt::black);
            painter->drawText(x + iconW, y + mm, w - iconW, mh, Qt::AlignLeft | Qt::AlignVCenter, dateName);

            painter->setPen(QColor(120, 120, 120));
            
            if (dateOrigin == Date::eSingleDate) {
#ifdef FIXEDPRIOR
                painter->drawText(x + iconW, y + 2*mm + mh, w - iconW, mh, Qt::AlignLeft | Qt::AlignVCenter, tr("Plugin: %1").arg(plugin->getName()));
#else
                painter->drawText(x + iconW, y + 2*mm + mh, w - iconW, mh, Qt::AlignLeft | Qt::AlignVCenter, tr("Type : %1 | MCMC %2").arg(plugin->getName(), dateMethodStr));
#endif
                painter->drawText(x + iconW, y + 3*mm + 2*mh, w - iconW, mh, Qt::AlignLeft | Qt::AlignVCenter, dateDesc);

                painter->setPen(Painting::mainColorLight);
                painter->drawText(x + iconW, y + 4*mm + 3*mh, w - iconW, mh, Qt::AlignLeft | Qt::AlignVCenter, deltaDesc);
            } else {
#ifdef FIXEDPRIOR
                painter->drawText(x + iconW, y + 2*mm + mh, w - iconW, mh, Qt::AlignLeft | Qt::AlignVCenter, tr("Plugin: %1").arg(plugin->getName()));
#else
                painter->drawText(x + iconW, y + 2*mm + mh, w - iconW, mh, Qt::AlignLeft | Qt::AlignVCenter, tr("Type : %1 | MCMC %2").arg(plugin->getName(), dateMethodStr));
#endif
                painter->drawText(x + iconW, y + 3*mm + 2*mh, w - iconW, mh, Qt::AlignLeft | Qt::AlignVCenter, "Combine Calcul");
            }
            if (!isValid) {
                painter->setPen(Qt::black);
                QString str = tr("Individual calibration not digitally computable ...");
                painter->drawText(x + iconW, y + 4*mm + 3*mh, w - iconW, mh, Qt::AlignRight | Qt::AlignVCenter, str );
            }
            painter->setPen(QColor(200, 200, 200));
            painter->drawLine(x, y + h, x + w, y + h);


        }
    }
};

#endif
