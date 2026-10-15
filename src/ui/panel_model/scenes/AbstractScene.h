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

#ifndef ABSTRACTSCENE_H
#define ABSTRACTSCENE_H


#include <QGraphicsScene>
#include "Project.h"

class AbstractItem;
class ArrowItem;
class ArrowTmpItem;


class AbstractScene: public QGraphicsScene
{
    Q_OBJECT


public:
    bool mDrawingArrow;
    ArrowTmpItem* mTempArrow;
    bool mSelectKeyIsDown; // used to add item in selection
    bool mShowGrid;

    AbstractScene(QGraphicsView* view, QObject* parent = nullptr);
    virtual ~AbstractScene();

    void adjustSceneRect();

    void setProject();

    QList<AbstractItem*> getItemsList() const  {return mItems;}
    bool showAllThumbs() const {return mShowAllThumbs;}


    qreal deltaGrid() const {return mDeltaGrid;}

public slots:
    void showGrid(bool show);

public:
    virtual void clean() = 0;

    virtual void constraintDoubleClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e) = 0;
    virtual void constraintClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e) = 0;

    virtual bool itemClicked(AbstractItem* eventItem, QGraphicsSceneMouseEvent* e);
    virtual void itemDoubleClicked(AbstractItem* eventItem, QGraphicsSceneMouseEvent* e);
    virtual void itemEntered(AbstractItem* eventItem, QGraphicsSceneHoverEvent* e);
    virtual void itemLeaved(AbstractItem* item, QGraphicsSceneHoverEvent* e);
    virtual void itemMoved(AbstractItem* item, QPointF newPos, bool merging);
    virtual void itemReleased(AbstractItem* item, QGraphicsSceneMouseEvent* e);

   // virtual void sendUpdateProject(const QString& reason, bool notify, bool async) = 0;
    virtual void sendUpdateProject(const Project::ReasonId reason, bool notify, bool async) = 0;


    void updateConstraintsPos(AbstractItem* movedItem, const QPointF& newPos);

protected:
    QGraphicsView* mView;
    QList<AbstractItem*> mItems;
    QList<ArrowItem*> mConstraintItems;

    bool mUpdatingItems;
    bool mAltIsDown;

    bool mShowAllThumbs;

    double mZoom;

    AbstractItem* mCurrentItem;
    qreal mDeltaGrid;

protected:
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* e);

    virtual void keyPressEvent(QKeyEvent* keyEvent);
    virtual void keyReleaseEvent(QKeyEvent* keyEvent);

    virtual AbstractItem* currentItem() = 0;
    virtual AbstractItem* collidingItem(const QGraphicsItem* item) = 0;
    virtual void deleteSelectedItems() = 0;

    virtual bool constraintAllowed(AbstractItem* itemFrom, AbstractItem* itemTo) = 0;
    virtual void createConstraint(AbstractItem* itemFrom, AbstractItem* itemTo) = 0;
    virtual void mergeItems(AbstractItem* itemFrom, AbstractItem* itemTo) = 0;

    void drawBackground(QPainter* painter, const QRectF& rect);



signals:
    void projectUpdated();

};

#endif
