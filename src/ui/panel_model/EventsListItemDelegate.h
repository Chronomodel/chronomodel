#ifndef EventsListItemDelegate_H
#define EventsListItemDelegate_H

#include <QItemDelegate>
#include <QtWidgets>
#include "Event.h"
#include "../PluginAbstract.h"
#include "Painting.h"
#include "ModelUtilities.h"


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
        
        if(option.state & QStyle::State_Selected)
        {
            //painter->fillRect(option.rect, option.palette.highlight());
            painter->fillRect(option.rect, QColor(220, 220, 220));
        }
        QString factName = index.model()->data(index, 0x0101).toString();
        int numDates = index.model()->data(index, 0x0103).toInt();
        QColor factColor = QColor(index.model()->data(index, 0x0104).toInt(),
                                  index.model()->data(index, 0x0105).toInt(),
                                  index.model()->data(index, 0x0106).toInt());
        
        QPixmap pixmap(iconS, iconS);
        QPainter p(&pixmap);
        p.fillRect(0, 0, iconS, iconS, factColor);
        p.setPen(Qt::black);
        p.drawRect(0, 0, iconS, iconS);
        painter->drawPixmap(x + (iconW - iconS)/2, y + (h - iconS)/2, iconS, iconS, pixmap, 0, 0, pixmap.width(), pixmap.height());
        
        QFont font = option.font;
        font.setPointSizeF(pointSize(11));
        painter->setFont(font);
        
        painter->setPen(Qt::black);
        painter->drawText(x + iconW, y, w - iconW, h/2, Qt::AlignLeft | Qt::AlignVCenter, factName);
        
        painter->drawText(x + iconW, y + h/2, w - iconW, h/2, Qt::AlignLeft | Qt::AlignVCenter, QString::number(numDates) + " dates");
        
        painter->setPen(QColor(200, 200, 200));
        painter->drawLine(x, y + h, x + w, y + h);
    }
};

#endif