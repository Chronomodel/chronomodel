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

#include "AbstractScene.h"
#include "AbstractItem.h"
#include "ArrowItem.h"
#include "ArrowTmpItem.h"
#include "StateKeys.h"

#include <QtWidgets>

AbstractScene::AbstractScene(QGraphicsView* view, QObject* parent):
    QGraphicsScene(parent),
    mDrawingArrow(false),
    mSelectKeyIsDown(false),
    mShowGrid(false),
    mView(view),
    mUpdatingItems(false),
    mAltIsDown(false),
    mShowAllThumbs(true),
    mZoom(1.),
    mDeltaGrid ( AbstractItem::mItemWidth /4.) // 150 is the width of the Event and phase Item
{
    mTempArrow = new ArrowTmpItem();
    addItem(mTempArrow);
    mTempArrow->setVisible(false);
}


AbstractScene::~AbstractScene()
{
    // [3] Destruction ordonnée tant que la partie AbstractScene est encore vivante :
    // ~EventItem utilise mScene->getItemsList(), ce qui serait indéfini pendant ~QGraphicsScene.
    blockSignals(true); // pas de selectionChanged vers les slots des classes dérivées déjà détruites
    mItems.clear();
    for (auto* ci : mConstraintItems)
        delete ci;
    mConstraintItems.clear();

    // mTempArrow est toujours dans la scène : clear() le détruit avec les autres items
    clear();
    mTempArrow = nullptr;
}

void AbstractScene::showGrid(bool show)
{
    mShowGrid = show;
    update();
}

void AbstractScene::updateConstraintsPos(AbstractItem* movedItem, const QPointF& newPos)
{
    Q_UNUSED(newPos);

    auto curItem = currentItem();
    if (curItem)
        mTempArrow->setFrom(curItem->pos().x(), curItem->pos().y());

    if (movedItem) {
        const int itemId = movedItem->mData[STATE_ID].toInt();
        //const double itemX = movedItem->mData[STATE_ITEM_X].toDouble();
        //const double itemY = movedItem->mData[STATE_ITEM_Y].toDouble();

        //qDebug() << "---------";
        //qDebug() << "Moving event id : " << itemId;

        for (auto & ci : mConstraintItems) {
            const QJsonObject &cData = ci->data();

            int bwdId = cData[STATE_CONSTRAINT_BWD_ID].toInt();
            int fwdId = cData[STATE_CONSTRAINT_FWD_ID].toInt();

            if (bwdId == itemId || fwdId == itemId) {
                //qDebug() << "Backward const. id : " << cId << " (link: "<<bwdId<<" -> "<< fwdId <<", setFrom: " << itemX << ", " << itemY << ")";
                //ci->setFrom(itemX, itemY);
                ci->updatePosition();
            }
            /*else if (fwdId == itemId) {
                //qDebug() << "Forward const. id : " << cId << " (link: "<<bwdId<<" -> "<< fwdId <<", setTo: " << itemX << ", " << itemY << ")";
                ci->setTo(itemX, itemY);
            }*/

        }
    }
}

// Items events
/**
 * @brief AbstractScene::itemClicked
 * @param item ie anEvent or a Phase
 * @param e
 * @return
 * Arrive with a click on item (ie an Event or a Phase),
 * Becareful with the linux system Alt+click can't be detected,
 *  in this case the user must combine Alt+Shift+click to valided a constraint
 * This function is overwrite in EventScene
 */
bool AbstractScene::itemClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);

    auto current = currentItem();

    // if mDrawingArrow is true, an Event is already selected and we can create a Constraint.
    if (mDrawingArrow && current && item && (item != current)) {

        if (constraintAllowed(current, item)) {
            createConstraint(current, item);
            mTempArrow->setVisible(false);
            mDrawingArrow=false;
            return true;

        } else
            return false;


    } else
            return false;


}

void AbstractScene::itemDoubleClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(item);
    Q_UNUSED(e);
}

/**
 * @brief AbstractScene::itemEntered
 * @param item
 * @param e
 * Happens when the mouse hovers over an event
 */
void AbstractScene::itemEntered(AbstractItem* item, QGraphicsSceneHoverEvent* e)
{
    Q_UNUSED(e);
    // [4] hors chemin chaud : qDebug() << "[AbstractScene::itemEntered]";
    auto current = currentItem();

    if (mDrawingArrow && current && item && (item != current)) {

        mTempArrow->setTo(item->pos().x(), item->pos().y());

        if (constraintAllowed(current, item)) {
            mTempArrow->setState(ArrowTmpItem::eAllowed);
            mTempArrow->setLocked(true);
            // [4] hors chemin chaud : qDebug() << "[AbstractScene::itemEntered] constraintAllowed == true";
        } else {
            mTempArrow->setState(ArrowTmpItem::eForbidden);
            mTempArrow->setLocked(true);
            // [4] hors chemin chaud : qDebug() << "[AbstractScene::itemEntered] constraintAllowed == false";
        }
    }

}

/**
 * @brief AbstractScene::itemLeaved
 * @param item
 * @param e
 * Happen when leaving an item (ei an Event)
 *
 */
void AbstractScene::itemLeaved(AbstractItem* item, QGraphicsSceneHoverEvent* e)
{
    Q_UNUSED(item);
    Q_UNUSED(e);
    if (mTempArrow) {
        mTempArrow->setLocked(false);
        mTempArrow->setState(ArrowTmpItem::eNormal);
    }
}

void AbstractScene::itemMoved(AbstractItem* item, QPointF newPos, bool merging)
{
    // Could be used to adjust the scene rect and to follow the moving item
    // But at the moment a bug occurs sometimes when moving an event out from the scene:
    // Its bounding rect seems to be not correct...

    Q_UNUSED(newPos);

    if (merging) {
        auto colliding = collidingItem(item);
        for (qsizetype i=0; i<mItems.size(); ++i)
            mItems[i]->setMergeable( (colliding != nullptr) && ( (mItems.at(i) == item) || (mItems.at(i) == colliding) ) );

    }

    // Bug moving multiple items out from the scene...
    //adjustSceneRect();
  /*
    QRectF r(newPos.x() - item->boundingRect().width()/2,
             newPos.y() - item->boundingRect().height()/2,
             item->boundingRect().size().width(),
             item->boundingRect().size().height());

    // Follow the moving item
     QList<QGraphicsView*> graphicsViews = views();
   for (int i=0; i<graphicsViews.size(); ++i)
    {
        graphicsViews[i]->ensureVisible(r, 30, 30);
        graphicsViews[i]->centerOn(item->scenePos());
    }*/
}

void AbstractScene::adjustSceneRect()
{
    // Ajust Scene rect to minimal
    setSceneRect(itemsBoundingRect().adjusted(-30, -30, 30, 30));
    update(sceneRect());
}

void AbstractScene::itemReleased(AbstractItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    if (item->mMoving) {
        for (qsizetype i=0; i<mItems.size(); ++i)
            mItems[i]->setMergeable(false);

        if (e->modifiers() == Qt::ShiftModifier) {
            auto colliding = collidingItem(item);
            if (colliding)
                mergeItems(item, colliding);
        } else {
            qDebug()<<"[AbstractScene::itemReleased] sendUpdateProject(Item moved)";
            sendUpdateProject(Project::ReasonId::ItemMoved, true, true);//  bool notify = true, bool storeUndoCommand = true
        }
    }
}


// Mouse events
void AbstractScene::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
    if (mDrawingArrow) {
        mTempArrow->setVisible(true);
        mTempArrow->setTo(e->scenePos().x(), e->scenePos().y());
    }
    QGraphicsScene::mouseMoveEvent(e);
}

// Key events
/**
 * @brief AbstractScene::keyPressEvent
 * @param keyEvent
 * Happen when we hit a key on the keyboard, for example "delete"
 */
void AbstractScene::keyPressEvent(QKeyEvent* keyEvent)
{
    if (keyEvent->isAutoRepeat())
        keyEvent->ignore();

    if (keyEvent->key() == Qt::Key_Delete)
        deleteSelectedItems();

    if (selectedItems().count() == 0) {
        qDebug() << "[AbstractScene::keyPressEvent] No item selected";
        if (mDrawingArrow == true) {
            mDrawingArrow = false;
            mTempArrow->setVisible(false);
            clearSelection();
        }

    }

    //key "Alt" detection
   else if (keyEvent->modifiers() == Qt::AltModifier && selectedItems().count()==1) {
        mAltIsDown = true;

        auto curItem = currentItem();
        // Check if an item is already selected
        if (curItem) {
            mDrawingArrow = true;
            mTempArrow->setVisible(true);
            mTempArrow->setFrom(curItem->pos().x(), curItem->pos().y());

        } else {
            mDrawingArrow = false;
            mTempArrow->setVisible(false);
            clearSelection();
        }
    }
    //else if(keyEvent->modifiers() == Qt::ShiftModifier)
    else if (keyEvent->key() == Qt::Key_Shift) {
        mSelectKeyIsDown = true;
    }
    else if (keyEvent->key() == Qt::Key_Escape && mDrawingArrow == true) {
        mDrawingArrow = false;
        mTempArrow->setVisible(false);
        clearSelection();

    }
  /*  else if (keyEvent->modifiers() == Qt::ControlModifier) {
        qDebug() << "[AbstractScene::keyPressEvent] You Press: "<< "Qt::ControlModifier";
    }*/
    else
        keyEvent->ignore();

    QGraphicsScene::keyPressEvent(keyEvent);
}

void AbstractScene::keyReleaseEvent(QKeyEvent* keyEvent)
{
    if (keyEvent->isAutoRepeat() )
        keyEvent->ignore();

    if (keyEvent->key() == Qt::Key_Alt) {
            // qDebug() << "[AbstractScene::keyReleaseEvent] You Released: "<<"Qt::Key_Alt";
             mDrawingArrow = false;
             mAltIsDown = false;
             mSelectKeyIsDown = false;
             mTempArrow->setVisible(false);
             QGraphicsScene::keyReleaseEvent(keyEvent);
    }

}

void AbstractScene::drawBackground(QPainter* painter, const QRectF& rect)
{
    painter->fillRect(rect, Qt::white);

    painter->setBrush(Qt::white);
    painter->setPen(Qt::NoPen);
    painter->drawRect(sceneRect());

    if (mShowGrid) {
        painter->setPen(QColor(220, 220, 220));
        const qreal x (sceneRect().x());
        const qreal y (sceneRect().y());
        const qreal w (sceneRect().width());
        const qreal h (sceneRect().height());
        const qreal delta (4 * mDeltaGrid );

        qreal xi (ceil(x/delta) * delta);
        while (xi< x + w) {
            painter->drawLine(QLineF(xi , y, xi, y + h));
            xi += delta;
        }

        qreal yi (ceil(y/delta) * delta);
        while (yi< y + h) {
            painter->drawLine(QLineF(x , yi, x + w, yi));
            yi += delta;
        }

    }

}
