#include "AbstractScene.h"
#include "AbstractItem.h"
#include "ArrowItem.h"
#include "ArrowTmpItem.h"
#include "StateKeys.h"
#include <QtWidgets>


AbstractScene::AbstractScene(QGraphicsView* view, QObject* parent):QGraphicsScene(parent),
mView(view),
mDrawingArrow(false),
mUpdatingItems(false),
mAltIsDown(false),
mShiftIsDown(false)
{
    mTempArrow = new ArrowTmpItem();
    addItem(mTempArrow);
    mTempArrow->setVisible(false);
    mTempArrow->setZValue(0);
    
    QBrush backBrush;
    backBrush.setTexture(QPixmap(":grid.png"));
    setBackgroundBrush(backBrush);
}

AbstractScene::~AbstractScene()
{
    
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
        float itemX = movedItem->mData[STATE_ITEM_X].toDouble();
        float itemY = movedItem->mData[STATE_ITEM_Y].toDouble();
        
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
bool AbstractScene::itemClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    if(mDrawingArrow)
    {
        AbstractItem* current = currentItem();
        if(current && item && (item != current))
        {
            createConstraint(current, item);
            mTempArrow->setVisible(false);
            return true;
        }
    }
    return false;
}

void AbstractScene::itemDoubleClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(item);
    Q_UNUSED(e);
}

void AbstractScene::itemEntered(AbstractItem* item, QGraphicsSceneHoverEvent* e)
{
    Q_UNUSED(e);
    if(mDrawingArrow)
    {
        mTempArrow->setState(ArrowTmpItem::eAllowed);
        mTempArrow->setTo(item->pos().x(), item->pos().y());
        mTempArrow->setLocked(true);
    }
}

void AbstractScene::itemLeaved(AbstractItem* item, QGraphicsSceneHoverEvent* e)
{
    Q_UNUSED(item);
    Q_UNUSED(e);
    mTempArrow->setLocked(false);
    mTempArrow->setState(ArrowTmpItem::eNormal);
}

void AbstractScene::itemMoved(AbstractItem* item, QPointF newPos, bool merging)
{
    Q_UNUSED(newPos);
    
    //qDebug() << " moved : " << newPos;
    
    
    
    if(merging)
    {
        AbstractItem* colliding = collidingItem(item);
        for(int i=0; i<mItems.size(); ++i)
        {
            mItems[i]->setMergeable(colliding != 0 && (mItems[i] == item || mItems[i] == colliding));
        }
    }
    
    // Ajust Scene rect to minimal
    //setSceneRect(itemsBoundingRect());
    
    // Follow the moving item
    /*QList<QGraphicsView*> graphicsViews = views();
     for(int i=0; i<graphicsViews.size(); ++i)
     {
     graphicsViews[i]->ensureVisible(item);
     graphicsViews[i]->centerOn(item->pos());
     }*/
}

void AbstractScene::itemReleased(AbstractItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    if(item->mMoving)
    {
        for(int i=0; i<mItems.size(); ++i)
            mItems[i]->setMergeable(false);
        
        if(e->modifiers() == Qt::ShiftModifier)
        {
            AbstractItem* colliding = collidingItem(item);
            if(colliding)
                mergeItems(item, colliding);
        }
        else
            sendUpdateProject(tr("item moved"), true, true);
    }
}

#pragma mark Mouse events
void AbstractScene::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
    if(mDrawingArrow)
    {
        //qDebug() << e->scenePos().x();
        mTempArrow->setVisible(true);
        mTempArrow->setTo(e->scenePos().x(), e->scenePos().y());
    }
    QGraphicsScene::mouseMoveEvent(e);
}

#pragma mark Key events
void AbstractScene::keyPressEvent(QKeyEvent* keyEvent)
{
    QGraphicsScene::keyPressEvent(keyEvent);
    
    if(keyEvent->key() == Qt::Key_Delete)
    {
        deleteSelectedItems();
    }
    else if(keyEvent->key() == Qt::Key_N)
    {
        // create ?
    }
    else if(keyEvent->modifiers() == Qt::AltModifier)
    {
        mAltIsDown = true;
        AbstractItem* curItem = currentItem();
        if(curItem)
        {
            mDrawingArrow = true;
            mTempArrow->setVisible(true);
            mTempArrow->setFrom(curItem->pos().x(), curItem->pos().y());
        }
    }
    else if(keyEvent->modifiers() == Qt::ShiftModifier)
    {
        mShiftIsDown = true;
    }
    else
    {
        keyEvent->ignore();
    }
}

void AbstractScene::keyReleaseEvent(QKeyEvent* keyEvent)
{
    mDrawingArrow = false;
    mAltIsDown = false;
    mShiftIsDown = false;
    mTempArrow->setVisible(false);
    QGraphicsScene::keyReleaseEvent(keyEvent);
}

