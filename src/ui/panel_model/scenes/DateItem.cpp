#include "DateItem.h"
#include "Date.h"
#include "Painting.h"
#include "EventItem.h"
#include "Project.h"
#include "PluginAbstract.h"

//#include <QtWidgets>

DateItem::DateItem(EventsScene* EventsScene, const QJsonObject& date, const QColor& color, const QJsonObject& settings, QGraphicsItem* parent):QGraphicsObject(parent),
    mEventsScene(EventsScene),
    mDate(date),
    mColor(color),
    mGreyedOut(false),
    mTitleHeight (15),
    mEltsHeight (30)
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
    Date d;
    d.fromJson(date);
    ProjectSettings s = ProjectSettings::fromJson(settings);

    d.mSettings.mTmin = s.mTmin;
    d.mSettings.mTmax = s.mTmax;
    d.mSettings.mStep = s.mStep;
    
    if (d.mPlugin!= nullptr) {
        if (!d.mIsValid)
            mCalibThumb = QPixmap();

        else {
            if (d.mCalibration == nullptr)
                d.calibrate(s, EventsScene->getProject());

            if (d.mPlugin->getName() == "Typo")
                mCalibThumb = d.generateTypoThumb();

             /* Can happen when there is trouble with the ref curve, for example with un Undo after
              * removing a refCurve
              */

            else if (d.mCalibration && !d.mCalibration->mCurve.isEmpty())
                mCalibThumb = d.generateCalibThumb();

            else
                mCalibThumb = QPixmap();

        }
    }
    
}

DateItem::~DateItem()
{

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
   EventItem* eventItem = dynamic_cast<EventItem*>(parentItem());
   if (eventItem) {
               QRectF r( -AbstractItem::mItemWidth/2 + AbstractItem::mBorderWidth + AbstractItem::mEltsMargin,
                         0,
                        AbstractItem::mItemWidth - 2*(AbstractItem::mBorderWidth + AbstractItem::mEltsMargin),
                        mTitleHeight +mEltsHeight);
        return r;
    } else
        return QRectF(0, 0, AbstractItem::mItemWidth - 2*(AbstractItem::mBorderWidth + AbstractItem::mEltsMargin) , mTitleHeight +mEltsHeight);
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
    font.setPointSizeF(10.);

    QString name = mDate.value(STATE_NAME).toString();
    QFont ftAdapt = AbstractItem::adjustFont(font, name, rName);
    painter->setFont(ftAdapt);

    painter->setPen(Qt::black);

    QFontMetrics metrics (ftAdapt);
    name = metrics.elidedText(name, Qt::ElideRight, int (r.width() - 5));

    painter->drawText(rName, Qt::AlignCenter, name);
    // thumbnail
    const QRectF rct = QRectF(-r.width()/2., mTitleHeight, r.width(), mEltsHeight);

    if (!mCalibThumb.isNull()) {
        // using matrix transformation, because antiAliasing don't work with pixmap
        qreal sx = rct.width()/mCalibThumb.width();
        qreal sy =  rct.height()/mCalibThumb.height();
        QMatrix mx = QMatrix();
        mx.scale(sx, sy);

        const QPixmap ct2 = mCalibThumb.transformed(mx, Qt::SmoothTransformation);

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

