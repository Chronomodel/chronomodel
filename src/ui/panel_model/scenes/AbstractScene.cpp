/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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

#include "AbstractScene.h"
#include "AbstractItem.h"
#include "ArrowItem.h"
#include "ArrowTmpItem.h"
#include "StateKeys.h"

#include <QtWidgets>

AbstractScene::AbstractScene(QGraphicsView* view, QObject* parent):QGraphicsScene(parent),
    mDrawingArrow(false),
    mSelectKeyIsDown(false),
    mShowGrid(false),
    mProject(nullptr),
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

// Setter & Getter
void AbstractScene::setProject(Project* project)
{
    mProject = project;
}

Project* AbstractScene::getProject() const
{
   return  mProject;
}

AbstractScene::~AbstractScene()
{

}

void AbstractScene::showGrid(bool show)
{
    mShowGrid = show;
    update();
}

void AbstractScene::updateConstraintsPos(AbstractItem* movedItem, const QPointF& newPos)
{
    Q_UNUSED(newPos);

    AbstractItem* curItem = currentItem();
    if (curItem)
        mTempArrow->setFrom(curItem->pos().x(), curItem->pos().y());

    if (movedItem) {
        const int itemId = movedItem->mData[STATE_ID].toInt();
        const double itemX = movedItem->mData[STATE_ITEM_X].toDouble();
        const double itemY = movedItem->mData[STATE_ITEM_Y].toDouble();

        //qDebug() << "---------";
        //qDebug() << "Moving event id : " << itemId;

        for (auto & ci : mConstraintItems) {
            const QJsonObject &cData = ci->data();

            int bwdId = cData[STATE_CONSTRAINT_BWD_ID].toInt();
            int fwdId = cData[STATE_CONSTRAINT_FWD_ID].toInt();

            if (bwdId == itemId) {
                //qDebug() << "Backward const. id : " << cId << " (link: "<<bwdId<<" -> "<< fwdId <<", setFrom: " << itemX << ", " << itemY << ")";
                ci->setFrom(itemX, itemY);
            }
            else if (fwdId == itemId) {
                //qDebug() << "Forward const. id : " << cId << " (link: "<<bwdId<<" -> "<< fwdId <<", setTo: " << itemX << ", " << itemY << ")";
                ci->setTo(itemX, itemY);
            }

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

    AbstractItem* current = currentItem();

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
    qDebug() << "[AbstractScene::itemEntered]";
    AbstractItem* current = currentItem();

    if (mDrawingArrow && current && item && (item != current)) {

        mTempArrow->setTo(item->pos().x(), item->pos().y());

        if (constraintAllowed(current, item)) {
            mTempArrow->setState(ArrowTmpItem::eAllowed);
            mTempArrow->setLocked(true);
            qDebug() << "[AbstractScene::itemEntered] constraintAllowed == true";
        } else {
            mTempArrow->setState(ArrowTmpItem::eForbidden);
            mTempArrow->setLocked(true);
            qDebug() << "[AbstractScene::itemEntered] constraintAllowed == false";
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
        AbstractItem* colliding = collidingItem(item);
        for (int i=0; i<mItems.size(); ++i)
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
   // setSceneRect(specialItemsBoundingRect().adjusted(-30, -30, 30, 30));
    update(sceneRect());
}

void AbstractScene::itemReleased(AbstractItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    if (item->mMoving) {
        for (int i=0; i<mItems.size(); ++i)
            mItems[i]->setMergeable(false);

        if (e->modifiers() == Qt::ShiftModifier) {
            AbstractItem* colliding = collidingItem(item);
            if (colliding)
                mergeItems(item, colliding);
        } else {
            qDebug()<<"[AbstractScene::itemReleased] sendUpdateProject(Item moved)";
            sendUpdateProject(tr("Item moved"), true, true);//  bool notify = true, bool storeUndoCommand = true
        }
        // Ajust Scene rect to minimal (and also fix the scene rect)
        // After doing this, the scene no longer stetches when moving items!
        // It is possible to reset it by calling setSceneRect(QRectF()),
        // but the scene rect is reverted to the largest size it has had!

        //setSceneRect(specialItemsBoundingRect().adjusted(-30, -30, 30, 30));

        //adjustSceneRect();

       // update();
    }
}

// Obsolete
QRectF AbstractScene::specialItemsBoundingRect(QRectF r) const
{
    QRectF rect = r;
    for (const auto& item : mItems) {
        const QRectF bRect = item->boundingRect();
        QRectF r(item->scenePos().x() - bRect.width()/2,
                 item->scenePos().y() - bRect.height()/2,
                 bRect.size().width(), bRect.size().height());
        rect = rect.united(r);
    }
    return rect;
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

    //key "Alt" detection
   else if (keyEvent->modifiers() == Qt::AltModifier && selectedItems().count()==1) {
        mAltIsDown = true;

        AbstractItem* curItem = currentItem();
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
    else if (keyEvent->key() == Qt::Key_Shift)
        mSelectKeyIsDown = true;
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
    //painter->fillRect(rect, QColor(230, 230, 230));
    painter->fillRect(rect, Qt::white);

    painter->setBrush(Qt::white);
    painter->setPen(Qt::NoPen);
    painter->drawRect(sceneRect());

    if (mShowGrid) {
        painter->setPen(QColor(220, 220, 220));
        const  qreal x (sceneRect().x());
        const qreal y (sceneRect().y());
        const qreal w (sceneRect().width());
        const qreal h (sceneRect().height());
        const qreal delta (4 * mDeltaGrid );

        qreal xi (ceil(x/delta) * delta);
        while (xi< x + w) {
            painter->drawLine(QLineF(xi , y, xi, y + h));
            xi += delta;
        }

        qreal yi (floor(y/delta) * delta);
        while (yi< y + h) {
            painter->drawLine(QLineF(x , yi, x + w, yi));
            yi += delta;
        }

    }

    // Cross for origin :
   /* painter->setPen(QColor(100, 100, 100));
    painter->drawLine(-10, 0, 10, 0);
    painter->drawLine(0, -10, 0, 10);
    */
}
