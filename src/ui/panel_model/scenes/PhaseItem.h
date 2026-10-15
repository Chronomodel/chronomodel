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

#ifndef PHASEITEM_H
#define PHASEITEM_H

#include "AbstractItem.h"

bool sortEvents(QPair<int, int> e1, QPair<int, int> e2);

class PhaseItem : public AbstractItem
{
    Q_OBJECT
// members
public:
    enum { Type = UserType + 21 };

    int type() const override
    {
        // Enable the use of qgraphicsitem_cast with this item.
        return Type;
    };

    bool mControlsVisible;
    bool mControlsEnabled;
    bool matLeastOneEventSelected;
    bool mOneEventSelectedOnScene;

    static int mTitleHeight;
    static int mEltsHeight;

 void redrawPhase();

//functions
protected:

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* e) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* e) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* e) override;

    // size and position of the buttons
    QRectF checkRect() const;
    QRectF insertRect() const;
    QRectF extractRect() const;

    QJsonArray getEvents() const;
    QString getTauString() const;
    // the two pixmaps of insert_Event and extract Event
    QPixmap* inPix;
    QPixmap* exPix;

public:
    PhaseItem(AbstractScene* scene, const QJsonObject& phase, QGraphicsItem* parent = nullptr);
    virtual ~PhaseItem();

    void setPhase(const QJsonObject& phase);

    void setControlsVisible(const bool visible);
    void setControlsEnabled(const bool enabled);


};

#endif
