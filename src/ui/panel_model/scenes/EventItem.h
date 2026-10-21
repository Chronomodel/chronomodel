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

#ifndef EVENTITEM_H
#define EVENTITEM_H

#include "AbstractItem.h"
#include "CurveSettings.h"

class EventsScene;

class EventItem : public AbstractItem
{
    Q_OBJECT
protected:

    QJsonObject mStudyPeriodSettings;
    bool mWithSelectedPhase;
    bool mThumbVisible;

    bool mIsNode = false; // [S2] non initialisé dans le chemin BoundItem

    const qreal mNodeSkin;

    qreal mPhasesHeight;
    qreal mCurveLineHeight;
    qreal mCurveTextHeight = 0.;
    // [4] réglages de courbe mis en cache dans setEvent()/resizeEventItem() pour ne pas
    // relire et reparser le JSON du projet à chaque paint()
    CurveSettings mCurveSettings;

public:
    EventItem(QGraphicsItem* parent = nullptr);

    explicit EventItem(EventsScene* scene, const QJsonObject &eventObj, const QJsonObject &StudyPeriodSettings, QGraphicsItem* parent = nullptr);
    virtual ~EventItem();

    enum { Type = UserType + 11 };

    int type() const override
    {
        // Enable the use of qgraphicsitem_cast with this item.
        return Type;
    };

    void remove_dateItems();
    virtual void setGreyedOut(const bool greyedOut) override;

    inline void setWithSelectedPhase(const bool selected) {mWithSelectedPhase = selected;}
    inline bool withSelectedPhase() { return mWithSelectedPhase;}

    virtual void setEvent(const QJsonObject& event, const QJsonObject& StudyPeriodSettings);

    void handleDrop(QGraphicsSceneDragDropEvent* e);
    QJsonArray getPhases() const;
    inline const QJsonObject &getSettings() const {return mStudyPeriodSettings;}

    virtual void setDatesVisible(bool visible);
    void redrawEvent();

    bool withSelectedDate() const;
    bool isCurveNode() const;

    void mousePressEvent(QGraphicsSceneMouseEvent* e) override;

    void updateGreyedOut();

protected:
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    virtual void dropEvent(QGraphicsSceneDragDropEvent* e) override;

    int getNumberCurveLines(const CurveSettings &cs) const;
    void paintBoxCurveParameter (QPainter *painter, const QRectF &rectBox, const CurveSettings &cs);

    void paintBoxPhases (QPainter *painter, const QRectF &rectBox);

    void resizeEventItem();
    void repositionDateItems();

    
};

#endif
