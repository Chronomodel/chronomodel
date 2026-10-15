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

#ifndef ARROWITEM_H
#define ARROWITEM_H

#pragma once

#include <QGraphicsItem>
#include <QJsonObject>

class AbstractScene;
class AbstractItem;
class EventItem;
class PhaseItem;

/* ---------------------------------------------------------------------
   ArrowItem représente une contrainte (flèche) entre deux items
   (EventItem ou PhaseItem).
   --------------------------------------------------------------------- */

enum ArrowType {
    eEventsScene = 0,
    ePhasesScene = 1
};

class ArrowItem : public QGraphicsItem
{
public:



    explicit ArrowItem(AbstractScene* scene,
                       ArrowType type_from,
                       const QJsonObject& constraint,
                       QGraphicsItem* parent = nullptr);

    ~ArrowItem() override = default;   // QObject s’occupe de la destruction

    enum { Type = UserType + 1 };

    int type() const override
    {
        // Enable the use of qgraphicsitem_cast with this item.
        return Type;
    };


    QRectF boundingRect() const override;
    QPainterPath shape() const override;

    QPointF contactPos(const double theta, AbstractItem *e);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    // -----------------------------------------------------------------
    //  Interaction
    // -----------------------------------------------------------------
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* e) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* e) override;

    // -----------------------------------------------------------------
    //  API métier
    // -----------------------------------------------------------------
    void updatePosition();
    void setFrom(const double x, const double y);
    void setTo(const double x, const double y);
    void setGreyedOut(bool greyedOut);

    QRectF getBubbleRect(const QString& text = QString()) const;
    QSizeF getBubbleSize(const QString& text = QString()) const;
    QString getBubbleText() const;

    void setData(const QJsonObject& c);
    inline QJsonObject& data() { return mData; }

    EventItem* findEventItemWithJsonId(const int id) const;
    PhaseItem* findPhaseItemWithJsonId(const int id) const;

    // -----------------------------------------------------------------
    //  Données publiques (pour un accès rapide depuis la scène)
    // -----------------------------------------------------------------
    ArrowType  mArrowType ; // old mType_from
    QJsonObject mData;
    AbstractScene* mScene = nullptr;
    QPointF mStart;
    QPointF mEnd;
    QPointF mStartContact;
    QPointF mEndContact;
    qreal   mBubbleWidth  = 0;
    qreal   mBubbleHeight = 0;
    bool    mEditing   = false;
    bool    mShowDelete = false;
    bool    mGreyedOut = false;
};

#endif // ARROWITEM_H
