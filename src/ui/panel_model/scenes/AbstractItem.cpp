#include "AbstractItem.h"
#include <QtWidgets>


AbstractItem::AbstractItem(AbstractScene* scene, QGraphicsItem* parent):QGraphicsObject(parent),
mScene(scene),
mBorderWidth(1.f),
mTitleHeight(20.f),
mPhasesHeight(10.f),
mEltsMargin(3.f),
mEltsWidth(15.f),
mEltsHeight(40.f),
mMoving(false),
mMergeable(false)
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

void AbstractItem::setMergeable(bool mergeable)
{
    mMergeable = mergeable;
    update();
}

#pragma mark Events
void AbstractItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    if(!mScene->itemClicked(this, e))
    {
        setZValue(2.);
        QGraphicsItem::mousePressEvent(e);
    }
}

void AbstractItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)
{
    // Call this first to ensure correct item manipulation :
    QGraphicsItem::mouseReleaseEvent(e);
    setZValue(1.);
    mScene->itemReleased(this, e);
    mMoving = false;
}

void AbstractItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e)
{
    mScene->itemDoubleClicked(this, e);
    QGraphicsItem::mouseDoubleClickEvent(e);
}

void AbstractItem::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
    QGraphicsItem::mouseMoveEvent(e);
    mMoving = true;
    mScene->itemMoved(this, e);
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

