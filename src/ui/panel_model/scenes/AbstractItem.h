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

#ifndef ABSTRACTITEM_H
#define ABSTRACTITEM_H

#include "AbstractScene.h"

#include <QGraphicsObject>
#include <QJsonObject>

class AbstractItem : public QGraphicsObject
{
public:
    AbstractItem(QGraphicsItem* parent = nullptr);
    explicit AbstractItem(AbstractScene* scene, QGraphicsItem* parent = nullptr);
    virtual ~AbstractItem();

    enum { Type = UserType + 1 };

    int type() const override
    {
        // Enable the use of qgraphicsitem_cast with this item.
        return Type;
    };

    void setMergeable(bool mergeable, bool shouldRepaint = true);
    virtual void setGreyedOut(const bool greyedOut);

    virtual void updateItemPosition(const QPointF& pos);
    void setSelectedInData(bool selected);
    void setCurrentInData(bool current);
    QJsonObject& getData() {return mData;};

    static QFont adjustFont(const QFont &ft, const QString &str, const QRectF &r);

    virtual QSizeF sizeF() {return mSize;};
    virtual QRectF rectF() const;
    virtual QRectF boundingRect() const override;

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* e) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* e) override;
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e) override;

    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* e) override;
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* e) override;
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* e) override;
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

public:
    static int mBorderWidth;
    static int mEltsMargin;
    static int mItemWidth;
    static int mTitleHeight;
    QSizeF mSize;

    QJsonObject mData;
    AbstractScene* mScene;

    int mEltsHeight;

    bool mMoving = false; // [S2] lu dans mouseReleaseEvent() sans avoir été initialisé ; // used in AbstractScene::itemReleased() to merge item like eventItem and phaseItem
    bool mMergeable;
    bool mGreyedOut;

};

#endif
