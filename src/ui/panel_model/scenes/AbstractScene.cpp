#include "AbstractScene.h"
#include "AbstractItem.h"
#include "ArrowItem.h"
#include "ArrowTmpItem.h"
#include "StateKeys.h"
#include <QtWidgets>


AbstractScene::AbstractScene(QGraphicsView* view, QObject* parent):QGraphicsScene(parent),
mDrawingArrow(false),
mProject(0),
mView(view),
mUpdatingItems(false),
mAltIsDown(false),
mShiftIsDown(false),
mShowGrid(false),
mShowAllThumbs(true),
mZoom(1.)
{
    mTempArrow = new ArrowTmpItem();
    addItem(mTempArrow);
    mTempArrow->setVisible(false);

}
#pragma mark Setter & Getter
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
    if(curItem)
        mTempArrow->setFrom(curItem->pos().x(), curItem->pos().y());
    
    if(movedItem)
    {
        int itemId = movedItem->mData[STATE_ID].toInt();
        double itemX = movedItem->mData[STATE_ITEM_X].toDouble();
        double itemY = movedItem->mData[STATE_ITEM_Y].toDouble();
        
        //qDebug() << "---------";
        //qDebug() << "Moving event id : " << itemId;
        
        for(int i=0; i<mConstraintItems.size(); ++i)
        {
            QJsonObject cData = mConstraintItems[i]->data();
            
            //int cId = cData[STATE_ID].toInt();
            int bwdId = cData[STATE_CONSTRAINT_BWD_ID].toInt();
            int fwdId = cData[STATE_CONSTRAINT_FWD_ID].toInt();
            
            if(bwdId == itemId)
            {
                //qDebug() << "Backward const. id : " << cId << " (link: "<<bwdId<<" -> "<< fwdId <<", setFrom: " << itemX << ", " << itemY << ")";
                mConstraintItems[i]->setFrom(itemX, itemY);
            }
            else if(fwdId == itemId)
            {
                //qDebug() << "Forward const. id : " << cId << " (link: "<<bwdId<<" -> "<< fwdId <<", setTo: " << itemX << ", " << itemY << ")";
                mConstraintItems[i]->setTo(itemX, itemY);
            }
            
            //mConstraintItems[i]->updatePosition();
        }
    }
}

#pragma mark Items events
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

        
    } else {
            return false;
    }

}

void AbstractScene::itemDoubleClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(item);
    Q_UNUSED(e);
}
// Arrive lorsque la souris passe sur un Event
void AbstractScene::itemEntered(AbstractItem* item, QGraphicsSceneHoverEvent* e)
{
    Q_UNUSED(e);
    qDebug() << "AbstractScene::itemEntered";
    AbstractItem* current = currentItem();

    if (mDrawingArrow && current && item && (item != current)) {

        mTempArrow->setTo(item->pos().x(), item->pos().y());

        if (constraintAllowed(current, item)) {
            mTempArrow->setState(ArrowTmpItem::eAllowed);           
            mTempArrow->setLocked(true);
            qDebug() << "AbstractScene::itemEntered constraintAllowed==true";
        } else {
            mTempArrow->setState(ArrowTmpItem::eForbidden);            
            mTempArrow->setLocked(true);
            qDebug() << "AbstractScene::itemEntered constraintAllowed==false";
        }
    }

}
// Arrive lorsque la souris sort d'un Event
/**
 * @brief AbstractScene::itemLeaved
 * @param item
 * @param e
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
            mItems[i]->setMergeable( (colliding != 0) && ( (mItems.at(i) == item) || (mItems.at(i) == colliding) ) );
        
    }

    // Bug moving multiple items out from the scene...
    //adjustSceneRect();
    
    QRectF r(newPos.x() - item->boundingRect().width()/2,
             newPos.y() - item->boundingRect().height()/2,
             item->boundingRect().size().width(),
             item->boundingRect().size().height());
    
    // Follow the moving item
    /* QList<QGraphicsView*> graphicsViews = views();
   for (int i=0; i<graphicsViews.size(); ++i)
    {
        graphicsViews[i]->ensureVisible(r, 30, 30);
        graphicsViews[i]->centerOn(item->scenePos());
    }*/
}

void AbstractScene::adjustSceneRect()
{
    // Ajust Scene rect to minimal
    setSceneRect(specialItemsBoundingRect().adjusted(-30, -30, 30, 30));
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
        } else
            sendUpdateProject(tr("item moved"), true, true);//  bool notify = true, bool storeUndoCommand = true
        
        // Ajust Scene rect to minimal (and also fix the scene rect)
        // After doing this, the scene no longer stetches when moving items!
        // It is possible to reset it by calling setSceneRect(QRectF()),
        // but the scene rect is reverted to the largest size it has had!
        
        //setSceneRect(specialItemsBoundingRect().adjusted(-30, -30, 30, 30));
        
        adjustSceneRect();
        
        update();
    }
}

QRectF AbstractScene::specialItemsBoundingRect(QRectF r) const
{
    QRectF rect = r;
    for (int i=0; i<mItems.size(); ++i) {
        const QRectF bRect = mItems.at(i)->boundingRect();
        QRectF r(mItems.at(i)->scenePos().x() - bRect.width()/2,
                 mItems.at(i)->scenePos().y() - bRect.height()/2,
                 bRect.size().width(), bRect.size().height());
        rect = rect.united(r);
    }
    return rect;
}

#pragma mark Mouse events
void AbstractScene::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
    if (mDrawingArrow) {
        //qDebug() << e->scenePos().x();
        mTempArrow->setVisible(true);
        mTempArrow->setTo(e->scenePos().x(), e->scenePos().y());
    }
    QGraphicsScene::mouseMoveEvent(e);
}

#pragma mark Key events
// Arrive quand on appuie sur une touche du clavier
void AbstractScene::keyPressEvent(QKeyEvent* keyEvent)
{
    QGraphicsScene::keyPressEvent(keyEvent);

    if (keyEvent->isAutoRepeat())
        keyEvent->ignore();

    if (keyEvent->key() == Qt::Key_Delete) {
        deleteSelectedItems();
    }  
    // Ici reperage de la touche Alt
   else if (keyEvent->modifiers() == Qt::AltModifier && selectedItems().count()==1) {
        qDebug() << "AbstractScene::keyPressEvent You Press: "<< "Qt::Key_Alt";
        mAltIsDown = true;
        QList<QGraphicsItem*> items = selectedItems();        
        
        AbstractItem* curItem = currentItem();
        // Controle si un item est déjà sélectionné
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
        mShiftIsDown = true;
  /*  else if (keyEvent->modifiers() == Qt::ControlModifier) {
        qDebug() << "AbstractScene::keyPressEvent You Press: "<< "Qt::ControlModifier";
    }*/
    else
        keyEvent->ignore();
    
}

void AbstractScene::keyReleaseEvent(QKeyEvent* keyEvent)
{
    if (keyEvent->isAutoRepeat() )
        keyEvent->ignore();


    if (keyEvent->key() == Qt::Key_Alt) {
             qDebug() << "AbstractScene::keyReleaseEvent You Released: "<<"Qt::Key_Alt";
             mDrawingArrow = false;
             mAltIsDown = false;
             mShiftIsDown = false;
             mTempArrow->setVisible(false);
             QGraphicsScene::keyReleaseEvent(keyEvent);
    }

}

void AbstractScene::drawBackground(QPainter* painter, const QRectF& rect)
{
    painter->fillRect(rect, QColor(230, 230, 230));
    
    QBrush backBrush;

    painter->setBrush(Qt::white);
    painter->setPen(Qt::NoPen);
    painter->drawRect(sceneRect());
    
    if (mShowGrid) {
        painter->setPen(QColor(220, 220, 220));
        int x = sceneRect().x();
        int y = sceneRect().y();
        int w = sceneRect().width();
        int h = sceneRect().height();
        int delta = 100;
        for (int i=0; i<w; i+=delta)
            painter->drawLine(x + i, y, x + i, y + h);
        
        for (int i=0; i<h; i+=delta)
            painter->drawLine(x, y + i, x+w, y + i);
    }
    
    // Cross for origin :
    painter->setPen(QColor(100, 100, 100));
    painter->drawLine(-10, 0, 10, 0);
    painter->drawLine(0, -10, 0, 10);
}

