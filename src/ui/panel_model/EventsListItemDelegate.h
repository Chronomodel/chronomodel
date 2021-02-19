/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
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
        int iconS = 20;

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

        QPixmap pixmap(iconS, iconS);
        QPainter p(&pixmap);
        if (numDates < 0) {
            p.setPen(Qt::black);
            p.setBrush(factColor);
            p.drawEllipse(0+1, 0+1, iconS-2, iconS-2);
           // p.fillRect(0, 0, iconS, iconS, factColor);

        } else {
            p.fillRect(0, 0, iconS, iconS, factColor);
            p.setPen(Qt::black);
            p.drawRect(0, 0, iconS, iconS);
        }


        painter->drawPixmap(x + (iconW - iconS)/2, y + (h - iconS)/2, iconS, iconS, pixmap, 0, 0, pixmap.width(), pixmap.height());

        QFont font = option.font;
        font.setPointSizeF(pointSize(11));
        painter->setFont(font);

        painter->setPen(Qt::black);
        painter->drawText(x + iconW, y, w - iconW, h/2, Qt::AlignLeft | Qt::AlignVCenter, factName);

        if (numDates < 0) {
            painter->drawText(x + iconW, y + h/2, w - iconW, h/2, Qt::AlignLeft | Qt::AlignVCenter, " Bound");

        } else {
            painter->drawText(x + iconW, y + h/2, w - iconW, h/2, Qt::AlignLeft | Qt::AlignVCenter, QString::number(numDates) + " dates");
        }

        painter->setPen(QColor(200, 200, 200));
        painter->drawLine(x, y + h, x + w, y + h);
    }
};

#endif
