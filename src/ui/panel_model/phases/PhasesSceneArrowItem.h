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

#ifndef PHASESSCENEARROWITEM_H
#define PHASESSCENEARROWITEM_H

#include <QObject>
#include <QGraphicsItem>

class PhasesScene;
class PhaseConstraint;
class PhasesItem;

class PhasesSceneArrowItem: public QGraphicsItem
{
public:
    PhasesSceneArrowItem(PhasesScene* phasesView, PhaseConstraint* constraint = 0, QGraphicsItem* parent = 0);

    void setItemFrom(PhasesItem* itemFrom);
    void setItemTo(PhasesItem* itemTo);
    void updatePosition();

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e);

    QRectF getBubbleRect() const;

    PhaseConstraint* mConstraint;
    PhasesItem* mItemFrom;
    PhasesItem* mItemTo;

protected:
    PhasesScene* mPhasesScene;

    double mXStart;
    double mYStart;
    double mXEnd;
    double mYEnd;

    float mBubbleWidth;
    float mBubbleHeight;
    float mDeleteWidth;

    bool mEditing;
};

#endif
