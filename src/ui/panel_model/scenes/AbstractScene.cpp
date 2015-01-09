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
mShiftIsDown(false),
mShowGrid(false)
{
    mTempArrow = new ArrowTmpItem();
    addItem(mTempArrow);
    mTempArrow->setVisible(false);
    mTempArrow->setZValue(0);
    
    
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
    // Could be used to adjust the scene rect and to follow the moving item
    // But at the moment a bug occurs sometimes when moving an event out from the scene:
    // Its bounding rect seems to be not correct...
    
    Q_UNUSED(newPos);
    
    if(merging)
    {
        AbstractItem* colliding = collidingItem(item);
        for(int i=0; i<mItems.size(); ++i)
        {
            mItems[i]->setMergeable(colliding != 0 && (mItems[i] == item || mItems[i] == colliding));
        }
    }

    // Bug moving multiple items out from the scene...
    //adjustSceneRect();
    
    QRectF r(newPos.x() - item->boundingRect().width()/2,
             newPos.y() - item->boundingRect().height()/2,
             item->boundingRect().size().width(),
             item->boundingRect().size().height());
    
    // Follow the moving item
    QList<QGraphicsView*> graphicsViews = views();
    for(int i=0; i<graphicsViews.size(); ++i)
    {
        //graphicsViews[i]->ensureVisible(r, 30, 30);
        //graphicsViews[i]->centerOn(item->scenePos());
    }
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
    for(int i=0; i<mItems.size(); ++i)
    {
        QRectF r(mItems[i]->scenePos().x() - mItems[i]->boundingRect().width()/2,
                 mItems[i]->scenePos().y() - mItems[i]->boundingRect().height()/2,
                 mItems[i]->boundingRect().size().width(),
                 mItems[i]->boundingRect().size().height());
        rect = rect.united(r);
    }
    //QRectF rect2 = itemsBoundingRect();
    return rect;
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

void AbstractScene::drawBackground(QPainter* painter, const QRectF& rect)
{
    painter->fillRect(rect, QColor(230, 230, 230));
    
    QBrush backBrush;
    if(mShowGrid)
    {
        backBrush.setTexture(QPixmap(":grid.png"));
        painter->setBrush(backBrush);
    }
    else
    {
        painter->setBrush(Qt::white);
    }
    painter->setPen(Qt::NoPen);
    painter->drawRect(sceneRect());
    
    painter->setPen(QColor(100, 100, 100));
    painter->drawLine(-10, 0, 10, 0);
    painter->drawLine(0, -10, 0, 10);
}
