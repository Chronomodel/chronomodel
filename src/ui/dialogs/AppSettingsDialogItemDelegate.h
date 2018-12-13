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
