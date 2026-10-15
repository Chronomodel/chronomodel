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

#ifndef DATEITEM_H
#define DATEITEM_H

#include "EventsScene.h"

#include <QObject>
#include <QGraphicsObject>
#include <QJsonObject>
#include <QColor>



class DateItem : public QGraphicsObject
{
    Q_OBJECT
public:
    DateItem(EventsScene* EventsScene, const QJsonObject& date, const QColor& color, const QJsonObject& settings, QGraphicsItem* parent = nullptr);
    virtual ~DateItem();

    enum { Type = UserType + 12 };

    int type() const override
    {
        // Enable the use of qgraphicsitem_cast with this item.
        return Type;
    };

    const QJsonObject& date() const;
    void setOriginalPos(const QPointF pos);

    void setGreyedOut(bool greyedOut);

    QRectF boundingRect() const override;

    static int mTitleHeight;
    static int mEltsHeight;
protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* e) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* e) override;
    void dropEvent(QGraphicsSceneDragDropEvent* e) override;

public:
    EventsScene* mEventsScene;
    QJsonObject mDate;
    QColor mColor;
    QPointF mOriginalPos;
    QPixmap mCalibThumb;
    bool mGreyedOut;


private:
    QGraphicsItemAnimation* mDatesAnim;
    QTimeLine* mDatesAnimTimer;


};

#endif
