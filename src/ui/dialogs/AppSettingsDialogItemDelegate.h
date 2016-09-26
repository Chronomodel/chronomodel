#ifndef APPSETTINGSDIALOGITEMDELEGATE_H
#define APPSETTINGSDIALOGITEMDELEGATE_H

#include <QItemDelegate>
#include <QtWidgets>
#include "PluginManager.h"
#include "../PluginAbstract.h"
#include "Painting.h"


class AppSettingsDialogItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    inline AppSettingsDialogItemDelegate(QObject* parent = 0):QItemDelegate(parent){}
    
    inline QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex&) const
    {
        return QSize(option.rect.width(), 45);
    }
    
    inline void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        int x = option.rect.x();
        int y = option.rect.y();
        int w = option.rect.width();
        int h = option.rect.height();
        int iconW = 30;
        int iconS = 20;
        
        painter->setRenderHint(QPainter::Antialiasing);
        if(option.state & QStyle::State_Selected){
            painter->fillRect(option.rect, Painting::mainColorLight);
        }
        
        QString name = index.model()->data(index, 0x0101).toString();
        QString pluginId = index.model()->data(index, 0x0102).toString();
        
        PluginAbstract* plugin = PluginManager::getPluginFromId(pluginId);
        QIcon icon(":/chronomodel.png");
        if(plugin)
            icon = plugin->getIcon();
        
        QPixmap pixmap = icon.pixmap(iconS, iconS);
        painter->drawPixmap(x + (iconW - iconS)/2, y + (h - iconS)/2, iconS, iconS, pixmap, 0, 0, pixmap.width(), pixmap.height());
        
        painter->setPen(option.state & QStyle::State_Selected ? Qt::white : Qt::black);
        painter->drawText(QRect(x + iconW, y, w - iconW, h), Qt::AlignLeft | Qt::AlignVCenter, name);
    }
};

#endif