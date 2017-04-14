#include "AbstractItem.h"
#include "ArrowTmpItem.h"
#include "StateKeys.h"
#include <QtWidgets>


AbstractItem::AbstractItem(AbstractScene* scene, QGraphicsItem* parent):QGraphicsObject(parent),
mScene(scene),
mBorderWidth(1.f),
mTitleHeight(20.f),
mPhasesHeight(20.f),
mEltsMargin(3.f),
mEltsWidth(15.f),
mEltsHeight(40.f),
mMoving(false),
mMergeable(false),
mGreyedOut(false)
{
    setZValue(1.);
    setAcceptHoverEvents(true);
    setAcceptDrops(true);
    setFlags(QGraphicsItem::ItemIsSelectable |
             QGraphicsItem::ItemIsMovable |
             QGraphicsItem::ItemIsFocusable |
             QGraphicsItem::ItemSendsScenePositionChanges |
             QGraphicsItem::ItemSendsGeometryChanges);
    
    // Not yet supported with retina display in Qt 5.3
#ifndef Q_OS_MAC
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
    shadow->setColor(Qt::black);
    shadow->setBlurRadius(4);
    shadow->setOffset(1, 1);
    setGraphicsEffect(shadow);
#endif
}

AbstractItem::~AbstractItem()
{
    
}

void AbstractItem::setMergeable(bool mergeable, bool shouldRepaint)
{
    mMergeable = mergeable;
    if (shouldRepaint)
        update();
}

void AbstractItem::setGreyedOut(const bool greyedOut)
{
    if (mGreyedOut != greyedOut)
        mGreyedOut = greyedOut;
}

void AbstractItem::setSelectedInData(const bool selected)
{
    mData[STATE_IS_SELECTED] = selected;
}

void AbstractItem::setCurrentInData(const bool current)
{
    mData[STATE_IS_CURRENT] = current;
}
//#pragma mark Events
void AbstractItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    qDebug()<<"AbstractItem::mousePressEvent__________??";
        
    if (!mScene->itemClicked(this, e)) {
        setZValue(2.);
        QGraphicsItem::mousePressEvent(e);
    } else
        mScene->mTempArrow->setFrom(pos().x(), pos().y());

}

void AbstractItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)
{
    setZValue(1.);
    mScene->itemReleased(this, e);
    // Must be changed AFTER "itemReleased" because used by this function :
    mMoving = false;
    QGraphicsItem::mouseReleaseEvent(e);
}

void AbstractItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e)
{
    mScene->itemDoubleClicked(this, e);
    QGraphicsItem::mouseDoubleClickEvent(e);
}


void AbstractItem::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
    mMoving = true;
    QGraphicsItem::mouseMoveEvent(e);
}

void AbstractItem::hoverEnterEvent(QGraphicsSceneHoverEvent* e)
{
    mScene->itemEntered(this, e);
    QGraphicsItem::hoverEnterEvent(e);
}

void AbstractItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* e)
{
    mScene->itemLeaved(this, e);
    QGraphicsItem::hoverLeaveEvent(e);
}

QVariant AbstractItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionChange && scene()) {
        // value is the new position.
        QPointF newPos = value.toPointF();

        // See comment in itemMoved function!
        mScene->itemMoved(this, newPos, false);
        
        // Save item position in project state : constraints need it to update their position.
        // Dot not save this as an undo command and don't notify views for update
        //mScene->sendUpdateProject(tr("item moved"), false, false);
        updateItemPosition(newPos);

        // Update constraints positions
        mScene->updateConstraintsPos(this, newPos);
        
        return newPos;
        
        // Migth be useful one day to constrain event inside the current scene...
        /*QRectF rect = scene()->sceneRect();
        if (!rect.contains(newPos)) {
            // Keep the item inside the scene rect.
            newPos.setX(qMin(rect.right(), qMax(newPos.x(), rect.left())));
            newPos.setY(qMin(rect.bottom(), qMax(newPos.y(), rect.top())));
            return newPos;
        }*/
    }
    return QGraphicsItem::itemChange(change, value);
}

