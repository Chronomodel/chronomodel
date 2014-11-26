#include "DateItem.h"
#include "Date.h"
#include "Painting.h"
#include "EventItem.h"
#include "Project.h"
#include <QtWidgets>


DateItem::DateItem(EventsScene* EventsScene, const QJsonObject& date, const QColor& color, const QJsonObject& settings, QGraphicsItem* parent):QGraphicsObject(parent),
mEventsScene(EventsScene),
mDate(date),
mColor(color),
mGreyedOut(false)
{
    setZValue(1.);
    setAcceptHoverEvents(true);
    setAcceptDrops(true);
    setFlags(ItemIsMovable | ItemIsSelectable);
    
    Date d = Date::fromJson(date);
    
    float tmin = settings[STATE_SETTINGS_TMIN].toDouble();
    float tmax = settings[STATE_SETTINGS_TMAX].toDouble();
    float step = settings[STATE_SETTINGS_STEP].toDouble();
    
    d.calibrate(tmin, tmax, step);
    mCalibThumb = d.generateCalibThumb(tmin, tmax);
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
    QGraphicsItem* parent = parentItem();
    if(parent)
    {
        EventItem* eventItem = dynamic_cast<EventItem*>(parent);
        if(eventItem)
        {
            QRectF pr = eventItem->boundingRect();
            QRectF r(-pr.width()/2 + eventItem->mBorderWidth + eventItem->mEltsMargin,
                     0,
                     pr.width() - 2*(eventItem->mBorderWidth + eventItem->mEltsMargin),
                     eventItem->mEltsHeight);
            return r;
        }
    }
    return QRectF(0, 0, 100, 30);
}

void DateItem::setGreyedOut(bool greyedOut)
{
    mGreyedOut = greyedOut;
    update();
}

void DateItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    QRectF r = boundingRect();
    painter->fillRect(r, mColor);
    
    r.adjust(2, 2, -2, -2);
    painter->fillRect(r, Qt::white);
    
    QFont font = qApp->font();
    font.setPointSizeF(pointSize(11));
    painter->setFont(font);
    
    painter->setPen(Qt::black);
    painter->drawText(r.adjusted(0, 0, 0, -r.height()/2), Qt::AlignCenter, mDate[STATE_NAME].toString());
    
    painter->drawPixmap(r.adjusted(0, r.height()/2, 0, 0),
                        mCalibThumb,
                        mCalibThumb.rect());
    
    /*if(mGreyedOut)
    {
        painter->setPen(Painting::greyedOut);
        painter->setBrush(Painting::greyedOut);
        painter->drawRect(boundingRect());
    }*/
}

void DateItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    parentItem()->setZValue(2.);
    EventItem* eventItem = dynamic_cast<EventItem*>(parentItem());
    if(eventItem)
    {
        eventItem->setSelected(true);
        mEventsScene->itemClicked(eventItem, e);
        e->accept();
    }
    QGraphicsObject::mousePressEvent(e);
}

void DateItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)
{
    parentItem()->setZValue(1.);
    mEventsScene->dateReleased(this, e);
    QGraphicsObject::mouseReleaseEvent(e);
}

void DateItem::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
    mEventsScene->dateMoved(this, e);
    QGraphicsObject::mouseMoveEvent(e);
}

void DateItem::dropEvent(QGraphicsSceneDragDropEvent* e)
{
    EventItem* eventItem = dynamic_cast<EventItem*>(parentItem());
    if(eventItem)
    {
        eventItem->handleDrop(e);
    }
}
