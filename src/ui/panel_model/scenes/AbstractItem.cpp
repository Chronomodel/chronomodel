/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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

#include "AbstractItem.h"
#include "ArrowTmpItem.h"
#include "StateKeys.h"
#include "Painting.h"
#include <QtWidgets>

 int AbstractItem::mBorderWidth  (2);
 int AbstractItem::mEltsMargin  (4);
 int AbstractItem::mItemWidth (180);

AbstractItem::AbstractItem(AbstractScene* scene, QGraphicsItem* parent):QGraphicsObject(parent),
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
    mScene = nullptr;
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
    //qDebug()<<"AbstractItem::mousePressEvent__________??";

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
    auto ptInit = pos();
    if (mScene->mShowGrid) {
        QPointF ptBefore = pos();
        qreal delta (mScene->deltaGrid());
        ptBefore = QPointF(round(ptBefore.rx()/delta) * delta, floor(ptBefore.ry()/ delta) * delta);
        setPos(ptBefore);
        e->setPos(ptBefore);
    }

    QGraphicsItem::mouseMoveEvent(e);
    mMoving = !(ptInit==pos());

//qDebug() <<"AbstractItem::mouseMoveEvent() mMoving="<<mMoving;
  //  if (e->pos().x()==0 || e->pos().y()==0)
 //qDebug()<<"AbstractItem::mouseMoveEvent() mData"<<this->mData.value(STATE_ITEM_X).toDouble()<<mData.value(STATE_ITEM_Y).toDouble();
 //()<<"AbstractItem::mouseMoveEvent() e->pos3"<<e->pos()<<e->scenePos()<<pos();


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


