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

#include "AbstractItem.h"
#include "ArrowTmpItem.h"
#include "StateKeys.h"

#include <QtWidgets>

int AbstractItem::mBorderWidth  (2);
int AbstractItem::mEltsMargin  (4);
int AbstractItem::mItemWidth (180);
int AbstractItem::mTitleHeight (20);

AbstractItem::AbstractItem(QGraphicsItem* parent):
    QGraphicsObject(parent),
    mScene(0),
    mMergeable(false),
    mGreyedOut(false)
{

}

AbstractItem::AbstractItem(AbstractScene* scene, QGraphicsItem* parent):
    QGraphicsObject(parent),
    mScene(scene),
    mMergeable(false),
    mGreyedOut(false)
{

    setPos(0., 0.);
    setZValue(1.);
    setAcceptHoverEvents(true);
    setAcceptDrops(true);
    setFlags( QGraphicsItem::ItemIsSelectable |
                     QGraphicsItem::ItemIsMovable |
                     QGraphicsItem::ItemIsFocusable |
                     QGraphicsItem::ItemSendsScenePositionChanges |
                     QGraphicsItem::ItemSendsGeometryChanges);

    // Not supported with retina display in Qt 5.3
    // but now we are in Qt 6
//#ifndef Q_OS_MAC
/*    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
    shadow->setColor(Qt::black);
    shadow->setBlurRadius(30);
    shadow->setOffset(3, 3);
    setGraphicsEffect(shadow);
*/
//#endif
}

AbstractItem::~AbstractItem()
{
    mScene = nullptr;
}

QRectF AbstractItem::rectF() const
{
    return QRectF(-mSize.width()/2., -mSize.height()/2., mSize.width(), mSize.height());
}

QRectF AbstractItem::boundingRect() const
{
    return QRectF(-mSize.width()/2 - 10, -mSize.height()/2 - 10, mSize.width() + 20, mSize.height() + 20);
}

void AbstractItem::updateItemPosition(const QPointF& pos)
{
    mData[STATE_ITEM_X] = pos.x();
    mData[STATE_ITEM_Y] = pos.y();
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

void AbstractItem::setSelectedInData(bool selected)
{
    mData[STATE_IS_SELECTED] = selected;
}

void AbstractItem::setCurrentInData(bool current)
{
    mData[STATE_IS_CURRENT] = current;
}
// Events
void AbstractItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    //qDebug()<<"[AbstractItem::mousePressEvent]__________??";

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

        updateItemPosition(ptBefore);
        setPos(ptBefore);
        // [4] hors chemin chaud : qDebug()<<" AbstractItem"<<ptBefore;
        e->setPos(ptBefore);
    }
    if (mMoving) {
        mScene->itemReleased(this, e);
        // Must be changed AFTER "itemReleased" because used by this function :
        mMoving = false;
    }
    QGraphicsItem::mouseReleaseEvent(e);
}

void AbstractItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e)
{
    mScene->itemDoubleClicked(this, e);
    QGraphicsItem::mouseDoubleClickEvent(e);
}

void AbstractItem::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
   /* if (mScene->mShowGrid) {
        QPointF ptBefore = pos();
        qreal delta (mScene->deltaGrid());
        ptBefore = QPointF(round(ptBefore.rx()/delta) * delta, floor(ptBefore.ry()/ delta) * delta);
        e->setPos(ptBefore);
    }*/
    mMoving = true;
    QGraphicsItem::mouseMoveEvent(e); // move the graphic item, and change this position

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

    } /*
    else if (change == ItemSelectedHasChanged || change == ItemSelectedChange) {

            //qDebug()<<"AbstractItem::itemChange() ItemSelectedHasChange  d "<<mData.value(STATE_NAME).toString()<<value.toBool();
           // mScene->updateStateSelectionFromItem(); // selection is manage in the scene unit
        }
    else if (change == ItemChildAddedChange ) {

          //  qDebug()<<"AbstractItem::itemChange() ItemChildAddedChange  d "<<mData.value(STATE_NAME).toString()<<value;
          // mScene->sendUpdateProject(tr("item move"), true, false);
            // mScene->updateStateSelectionFromItem(); // selection is manage in the scene unit
        }
    */

    return QGraphicsItem::itemChange(change, value);
}

QFont AbstractItem::adjustFont(const QFont &ft, const QString &str, const  QRectF &r)
{
    if (!str.isEmpty() ) {
        const QFontMetrics fm (ft);
        const QRect textRect = fm.boundingRect(str);
        const qreal w = r.width() - 10;
        const qreal xfactor (textRect.width()> w ? textRect.width()/w : 1);
        const qreal h = r.height();
        const qreal yfactor (textRect.height()>h ? textRect.height()/h : 1) ;
        const qreal factor  = ( xfactor > yfactor ? xfactor : yfactor);
        QFont ftBack = ft;
        ftBack.setPointSizeF(qMax(ft.pointSizeF()/factor, 5.));
        return ftBack;
    } else
        return ft;
}


