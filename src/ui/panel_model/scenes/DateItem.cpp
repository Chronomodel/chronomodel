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

#include "DateItem.h"

#include "CalibrationCurve.h"
#include "Date.h"
#include "EventItem.h"
#include "Project.h"
#include "PluginAbstract.h"

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

int DateItem::mTitleHeight (20);
int DateItem:: mEltsHeight (40);

DateItem::DateItem(EventsScene* EventsScene, const QJsonObject& date, const QColor& color, const QJsonObject& settings, QGraphicsItem* parent):QGraphicsObject(parent),
    mEventsScene (EventsScene),
    mDate (date),
    mColor (color),
    mGreyedOut (false)
{
    setZValue(1.);
    setAcceptHoverEvents(true);
    setAcceptDrops(true);
    setFlag(ItemIsMovable, false);

    // set the selection directly to the parent item, here the EventItem
    setFlag(ItemIsSelectable, false);

    mDatesAnimTimer = new QTimeLine(100);
    mDatesAnimTimer->setFrameRange(0, 2);

    mDatesAnim = new QGraphicsItemAnimation();
    mDatesAnim->setTimeLine(mDatesAnimTimer);

    // Date::fromJson doesn't create mCalibration
    Date d (date);
    const StudyPeriodSettings s = StudyPeriodSettings::fromJson(settings);

     if (d.mPlugin!= nullptr) {
        if (!d.mIsValid)
            mCalibThumb = QPixmap();

        else {

            // Date::calibrate() Controls the validity of the calibration and wiggle curves
            d.calibrate(s, *EventsScene->getProject(), true);

            if (d.mCalibration == nullptr) {
                date[STATE_DATE_VALID] = false;
                mCalibThumb = QPixmap();
                QString mes = tr("Calibration curve not find for the Event %1").arg(d.mName);
                //throw mes;
                QMessageBox message(QMessageBox::Critical,
                                    qApp->applicationName() + " " + qApp->applicationVersion(),
                                    mes,
                                    QMessageBox::Ok,
                                    qApp->activeWindow());
                message.exec();

                if (d.mWiggleCalibration != nullptr) {
                    d.mWiggleCalibration->mVector.clear();
                    d.mWiggleCalibration->mMap.clear();
                    d.mWiggleCalibration->mRepartition.clear();
                    d.mWiggleCalibration = nullptr;
                }
            } else if (d.mCalibration->mVector.size() < 6) {
                date[STATE_DATE_VALID] = false;
                mCalibThumb = QPixmap();
                const double newStep = d.mCalibration->mStep/5.;
                QString mes = tr("Insufficient resolution for the Event %1 \r Decrease the step in the study period box to %2").arg(d.mName, QString::number(newStep));
                //throw mes;
                QMessageBox message(QMessageBox::Critical,
                                    qApp->applicationName() + " " + qApp->applicationVersion(),
                                    mes,
                                    QMessageBox::Ok,
                                    qApp->activeWindow());
                message.exec();
                d.mCalibration->mVector.clear();
                d.mCalibration->mMap.clear();
                d.mCalibration->mRepartition.clear();
                d.mCalibration = nullptr;

                if (d.mWiggleCalibration) {
                    d.mWiggleCalibration->mVector.clear();
                    d.mWiggleCalibration->mMap.clear();
                    d.mWiggleCalibration->mRepartition.clear();
                    d.mWiggleCalibration = nullptr;
                }
            }

            if (d.mPlugin->getName() == "Unif" && d.mOrigin == Date::eSingleDate)
                mCalibThumb = d.generateUnifThumb(s);

             /* Can happen when there is trouble with the ref curve, for example with an Undo after
              * removing a refCurve
              */

            else if (d.mCalibration && !d.mCalibration->mVector.isEmpty()) {
               mCalibThumb = d.generateCalibThumb(s);

            } else
                mCalibThumb = QPixmap();

        }
    }
//blockSignals(false);
}

DateItem::~DateItem()
{
    mEventsScene= nullptr;
  //  mDate.~QJsonObject(); // Don't delete the JSON, we need it when we delete an event.
   // mColor.~QColor();
   // mCalibThumb.~QPixmap();
}

const QJsonObject& DateItem::date() const
{
    return mDate;
}

void DateItem::setOriginalPos(const QPointF pos)
{
    mOriginalPos = pos;
}

QRectF DateItem::boundingRect() const
{
    qreal x = 0;
    qreal y = 0;
    qreal w = AbstractItem::mItemWidth - 2*(AbstractItem::mBorderWidth + AbstractItem::mEltsMargin);
    qreal h = mTitleHeight + mEltsHeight;
    
    EventItem* eventItem = dynamic_cast<EventItem*>(parentItem());
    if (eventItem) {
       x = -AbstractItem::mItemWidth/2 + AbstractItem::mBorderWidth + AbstractItem::mEltsMargin;
    }
    
    return QRectF(x, y, w, h);
}

void DateItem::setGreyedOut(bool greyedOut)
{
    if (mGreyedOut != greyedOut) {
        mGreyedOut = greyedOut;
       // update(); //it is time comsuming and product slowing
   }
}

void DateItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setRenderHint(QPainter::Antialiasing);

    if (mGreyedOut)
        painter->setOpacity(qreal(0.35));
    else
        painter->setOpacity(qreal(1.));

    // background, it avoids small line between the box name and the thumbnail
    QRectF r = boundingRect();
    painter->fillRect(r, Qt::white);


    r.adjust(1, 1, -1, -1);
    // box name
    const QRectF rName = QRectF(-r.width()/2., 0, r.width(), mTitleHeight);

    painter->fillRect(rName, Qt::white);
    QFont font (qApp->font());
    //font.setPointSizeF(10.);
    font.setPixelSize(12.);

    QString name = mDate.value(STATE_NAME).toString();
   // QFont ftAdapt = AbstractItem::adjustFont(font, name, rName);
    //painter->setFont(ftAdapt);

    painter->setPen(Qt::black);

    //QFontMetrics metrics (ftAdapt);
    QFontMetrics metrics (font);
    name = metrics.elidedText(name, Qt::ElideRight, int (r.width() - 5));

  painter->setFont(font);
    painter->drawText(rName, Qt::AlignCenter, name);
    // thumbnail
    const QRectF rct = QRectF(-r.width()/2., mTitleHeight, r.width(), mEltsHeight);

    if (!mCalibThumb.isNull()) {
        // using matrix transformation, because antiAliasing don't work with pixmap
        qreal sx = rct.width()/mCalibThumb.width();
        qreal sy =  rct.height()/mCalibThumb.height();
//        QTransform mx;
//        mx.scale(sx, sy);

        const QPixmap ct2 = mCalibThumb.transformed(QTransform::fromScale(sx, sy), Qt::SmoothTransformation);

        painter->drawPixmap(int (rct.x()), int (rct.y()), ct2);

    } else {
        painter->fillRect(rct, Qt::white);
        painter->setPen(Qt::red);
        if (mDate.value(STATE_DATE_VALID).toBool())
            painter->drawText(rct, Qt::AlignCenter, tr("Outside study period"));
        else
            painter->drawText(rct, Qt::AlignCenter, tr("Not computable"));
    }
    // border
   // painter->setPen(mColor);
   // painter->drawRect(boundingRect());

    // restore default opacity
    painter->setOpacity(1.);

    // we don't need to refresh the Event

}


void DateItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    qDebug()<<"DateItem::mousePressEvent___________________ "<<e->modifiers();
    /* REMARK
     * On macOS Qt::MetaModifier map to the keyboard key "Ctrl"
     * and      Qt::ControlModifier to the keyboard key " cmd" = Command key (⌘)
     */
    if (e->modifiers() == Qt::ShiftModifier) {
    // if (       e->modifiers() == Qt::MetaModifier) { // don't work
        setFlag(ItemIsMovable, true);
        mEventsScene->clearSelection();
    } else
        setFlag(ItemIsMovable, false);

    EventItem* eventItem = dynamic_cast<EventItem*>(parentItem());

   if (eventItem && (mEventsScene->mDrawingArrow))  {
        eventItem->setZValue(2.);
        eventItem->mousePressEvent(e);
    }
   // must do it to send move the EventItem by heritage
    QGraphicsObject::mousePressEvent(e);
}


void DateItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)
{
    setFlag(ItemIsMovable, false);
    parentItem()->setZValue(1.);
    EventItem* hoveredEventItem = mEventsScene->dateReleased(this);
    if (hoveredEventItem) {
        // redraw find set the new position
        hoveredEventItem->redrawEvent();

        emit mEventsScene->eventsAreModified(tr("Date moved to event"), true, true);

    } else {
        if (pos() != mOriginalPos) {
            mDatesAnim->setItem(this);
            mDatesAnim->setPosAt(0, pos());
            mDatesAnim->setPosAt(1, mOriginalPos);
            mDatesAnimTimer->start();
            e->accept();

        } else {
            EventItem* eventItem = dynamic_cast<EventItem*>(parentItem());
            eventItem->setSelected(true);
            eventItem->mousePressEvent(e);
        }

    }

    // Must do it, this select the dateItem and automaticaly emit GraphicsScene::selectionChanged
    // which are connect to EventsScene::updateStateSelectionFromItem
    QGraphicsObject::mouseReleaseEvent(e);
}

void DateItem::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
    qDebug()<<"DateItem::mouseMoveEvent()";
    mEventsScene->dateMoved(this);
    e->accept();
    QGraphicsObject::mouseMoveEvent(e);

}

void DateItem::dropEvent(QGraphicsSceneDragDropEvent* e)
{
    EventItem* eventItem = dynamic_cast<EventItem*>(parentItem());
    if (eventItem)
        eventItem->handleDrop(e);

}
