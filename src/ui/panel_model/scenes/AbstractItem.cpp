#include "AbstractItem.h"
#include "ArrowTmpItem.h"
#include "StateKeys.h"
#include <QtWidgets>


AbstractItem::AbstractItem(AbstractScene* scene, QGraphicsItem* parent):QGraphicsObject(parent),
mScene(scene),
mBorderWidth(2.),
mTitleHeight(15.),
mPhasesHeight(20.),
mEltsMargin(3.),
mEltsWidth(15.),
mEltsHeight(40.),
mItemWidth(150.),
//mMoving(false),
mMergeable(false),
mGreyedOut(false)
{
    setPos(0., 0.);
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
// Events
void AbstractItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    qDebug()<<"AbstractItem::mousePressEvent__________??";
  /*  if (mScene->mShowGrid) {
        QPointF ptBefore = pos();
        ptBefore = QPointF(floor(ptBefore.rx()/10.) *10, floor(ptBefore.ry()/10.) *10);
        setPos(ptBefore);
        e->setPos(ptBefore);
    }*/
    if (!mScene->itemClicked(this, e)) {
        setZValue(2.);

        QGraphicsItem::mousePressEvent(e);
    } else
        mScene->mTempArrow->setFrom(pos().x(), pos().y());

}

void AbstractItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)
{
    setZValue(1.);
    if (mScene->mShowGrid) {
        qreal delta (mScene->deltaGrid());
        QPointF ptBefore = scenePos();
        ptBefore = QPointF(round(ptBefore.rx()/delta) * delta, round(ptBefore.ry()/delta) * delta);
        setPos(ptBefore);
        e->setPos(ptBefore);
    }
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
    //qDebug()<<"AbstractItem::mouseMoveEvent() pos()"<<pos();
    //qDebug()<<"AbstractItem::mouseMoveEvent()1 e->pos"<<e->pos()<<e->scenePos()<<pos();
    //updateItemPosition(e->scenePos());
    qDebug()<<"AbstractItem::mouseMoveEvent() e->pos avant mouseMove2"<<e->pos()<<e->scenePos()<<pos();
    if (mScene->mShowGrid) {
        QPointF ptBefore = pos();
        qreal delta (mScene->deltaGrid());
        ptBefore = QPointF(round(ptBefore.rx()/delta) * delta, floor(ptBefore.ry()/ delta) * delta);
        setPos(ptBefore);
        e->setPos(ptBefore);
    }
    //setSelected(true);

    QGraphicsItem::mouseMoveEvent(e);


  //  if (e->pos().x()==0 || e->pos().y()==0)
 qDebug()<<"AbstractItem::mouseMoveEvent() mData"<<this->mData.value(STATE_ITEM_X).toDouble()<<mData.value(STATE_ITEM_Y).toDouble();
 qDebug()<<"AbstractItem::mouseMoveEvent() e->pos3"<<e->pos()<<e->scenePos()<<pos();


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
//qDebug()<<"AbstractItem::itemChange()"<<change<<value;
    if (change == ItemPositionChange && scene()) {
        // value is the new position.
        QPointF newPos = value.toPointF();

        // See comment in itemMoved function!
        // used to merge date
        mScene->itemMoved(this, newPos, false);
        
        // Save item position in project state : constraints need it to update their position.
        // Dot not save this as an undo command and don't notify views for update
        //mScene->sendUpdateProject(tr("item moved"), false, false);
       // updateItemPosition(newPos);

        // Update constraints positions
        mScene->updateConstraintsPos(this, newPos);
        
 //       return newPos;
        
        // Migth be useful one day to constrain event inside the current scene...
        /*QRectF rect = scene()->sceneRect();
        if (!rect.contains(newPos)) {
            // Keep the item inside the scene rect.
            newPos.setX(qMin(rect.right(), qMax(newPos.x(), rect.left())));
            newPos.setY(qMin(rect.bottom(), qMax(newPos.y(), rect.top())));
            return newPos;
        }*/
    } else  if (change == ItemPositionHasChanged && scene()) {

      //  qDebug()<<"AbstractItem::itemChange() ItemPositionHasChanged "<<value.toPointF()<<"pos()"<<pos();
        updateItemPosition(value.toPointF());
    }
    else if (change == ItemSelectedHasChanged || change == ItemSelectedChange) {

            qDebug()<<"AbstractItem::itemChange() ItemSelectedHasChange  d "<<mData.value(STATE_NAME).toString()<<value.toBool();
           // mScene->updateStateSelectionFromItem(); // selection is manage in the scene unit
        }
    else if (change == ItemChildAddedChange ) {

          //  qDebug()<<"AbstractItem::itemChange() ItemChildAddedChange  d "<<mData.value(STATE_NAME).toString()<<value;
          // mScene->sendUpdateProject(tr("item move"), true, false);
            // mScene->updateStateSelectionFromItem(); // selection is manage in the scene unit
        }

    return QGraphicsItem::itemChange(change, value);
}

