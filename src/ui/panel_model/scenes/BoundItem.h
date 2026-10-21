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

#ifndef BOUNDITEM_H
#define BOUNDITEM_H

#include "EventItem.h"

class BoundItem : public EventItem
{
    Q_OBJECT
public:
    BoundItem(EventsScene* eventsScene, const QJsonObject &eventObj, const QJsonObject &settings, QGraphicsItem* parent = 0);
    virtual ~BoundItem();

    enum { Type = UserType + 13 };

    int type() const override
    {
        // Enable the use of qgraphicsitem_cast with this item.
        return Type;
    };

    void setEvent(const QJsonObject &eventObj, const QJsonObject &settings) override;
    void setDatesVisible(const bool visible) override;

protected:
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;
    virtual void dropEvent(QGraphicsSceneDragDropEvent* e) override;

    virtual QRectF toggleRect() const;

private:
    QImage mThumb;
    qreal mThumbH;
    // [4] paramètres de la miniature en cache : évite de recréer un GraphView (QWidget)
    // à chaque setEvent(), donc à chaque changement de sélection
    bool mThumbCached = false;
    double mThumbTmin = 0.;
    double mThumbTmax = 0.;
    double mThumbFixed = 0.;


};

#endif
