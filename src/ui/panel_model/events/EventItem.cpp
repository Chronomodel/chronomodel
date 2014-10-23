#include "EventItem.h"
#include "Event.h"
#include "Phase.h"
#include "Date.h"
#include "Painting.h"
#include "DateItem.h"
#include "ProjectManager.h"
#include "Project.h"
#include "QtUtilities.h"
#include "Painting.h"
#include <QtWidgets>


EventItem::EventItem(EventsScene* EventsScene, const QJsonObject& event, QGraphicsItem* parent):QGraphicsItem(parent),
mEventsScene(EventsScene),
mEvent(event),
mBorderWidth(1.f),
mTitleHeight(20.f),
mPhasesHeight(10.f),
mEltsMargin(3.f),
mEltsWidth(15.f),
mEltsHeight(40.f),
mShowElts(true),
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
    
    setEvent(mEvent);
}

EventItem::~EventItem()
{
    
}

#pragma mark Event Managment
QJsonObject& EventItem::event()
{
    return mEvent;
}

void EventItem::setEvent(const QJsonObject& event)
{
    mEvent = event;
    
    // ----------------------------------------------
    //  Update item position and selection
    // ----------------------------------------------
    setSelected(mEvent[STATE_EVENT_IS_SELECTED].toBool());
    setPos(mEvent[STATE_EVENT_ITEM_X].toDouble(),
           mEvent[STATE_EVENT_ITEM_Y].toDouble());
    
    // ----------------------------------------------
    //  Delete Date Items
    // ----------------------------------------------
    QList<QGraphicsItem*> dateItems = childItems();
    for(int i=0; i<dateItems.size(); ++i)
    {
        mEventsScene->removeItem(dateItems[i]);
        delete dateItems[i];
        qDebug() << "== date removed";
    }
    
    // ----------------------------------------------
    //  Re-create Date Items
    // ----------------------------------------------
    QJsonObject state = ProjectManager::getProject()->state();
    
    QJsonArray dates = mEvent[STATE_EVENT_DATES].toArray();
    for(int i=0; i<dates.size(); ++i)
    {
        QJsonObject date = dates[i].toObject();
        QColor color(mEvent[STATE_EVENT_RED].toInt(),
                     mEvent[STATE_EVENT_GREEN].toInt(),
                     mEvent[STATE_EVENT_BLUE].toInt());
        
        DateItem* dateItem = new DateItem(mEventsScene, date, color, state[STATE_SETTINGS].toObject());
        dateItem->setParentItem(this);
        
        qDebug() << "== date added";
        
        QPointF pos(0,
                    boundingRect().y() +
                    mTitleHeight +
                    mBorderWidth +
                    2*mEltsMargin +
                    i * (mEltsHeight + mEltsMargin));
        dateItem->setPos(pos);
        dateItem->setOriginalPos(pos);
    }
    
    // ----------------------------------------------
    //  Repaint base on mEvent
    // ----------------------------------------------
    update();
}

void EventItem::setMergeable(bool mergeable)
{
    mMergeable = mergeable;
    update();
}

#pragma mark Events
void EventItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    QGraphicsItem::mousePressEvent(e);
    
    setZValue(2.);
    /*if(toggleRect().contains(e->pos()))
    {
        mShowElts = !mShowElts;
        QList<QGraphicsItem*> children = childItems();
        for(int i=0; i<children.size(); ++i)
            children[i]->setVisible(mShowElts);
        
        update();
        if(scene())
            scene()->update();
    }
    else
    {
        mEventsScene->eventClicked(this, e);
    }*/
    mEventsScene->eventClicked(this, e);
}

void EventItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)
{
    // Call this first to ensure correct item manipulation :
    QGraphicsItem::mouseReleaseEvent(e);
    
    setZValue(1.);
    
    mEvent[STATE_EVENT_ITEM_X] = pos().x();
    mEvent[STATE_EVENT_ITEM_Y] = pos().y();
    
    mEventsScene->eventReleased(this, e);
}

void EventItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e)
{
    mEventsScene->eventDoubleClicked(this, e);
    QGraphicsItem::mouseDoubleClickEvent(e);
}

void EventItem::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
    QGraphicsItem::mouseMoveEvent(e);
    mEventsScene->eventMoved(this, e);
}

void EventItem::hoverEnterEvent(QGraphicsSceneHoverEvent* e)
{
    mEventsScene->eventEntered(this, e);
    QGraphicsItem::hoverEnterEvent(e);
}

void EventItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* e)
{
    mEventsScene->eventLeaved(this, e);
    QGraphicsItem::hoverLeaveEvent(e);
}

void EventItem::dropEvent(QGraphicsSceneDragDropEvent* e)
{
    handleDrop(e);
}

void EventItem::handleDrop(QGraphicsSceneDragDropEvent* e)
{
    e->acceptProposedAction();
    Project* project = ProjectManager::getProject();
    QJsonObject event = mEvent;
    
    QJsonArray dates = event[STATE_EVENT_DATES].toArray();
    QList<Date> datesDragged = mEventsScene->decodeDataDrop(e);
    for(int i=0; i<datesDragged.size(); ++i)
    {
        QJsonObject date = datesDragged[i].toJson();
        date[STATE_DATE_ID] = project->getUnusedDateId(dates);
        dates.append(date);
    }
    event[STATE_EVENT_DATES] = dates;
    
    project->updateEvent(event, QObject::tr("Dates added to event (CSV drag)"));
}



void EventItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    
    QRectF rect = boundingRect();
    
    QColor eventColor = QColor(mEvent[STATE_EVENT_RED].toInt(),
                               mEvent[STATE_EVENT_GREEN].toInt(),
                               mEvent[STATE_EVENT_BLUE].toInt());
    painter->setPen(Qt::NoPen);
    painter->setBrush(eventColor);
    painter->drawRect(rect);
    
    
    // Phases
    
    /*int y = rect.height() - mBorderWidth - mPhasesHeight;
    QRectF r = rect.adjusted(0, y, 0, 0);
    QPainterPath clip;
    clip.addRoundedRect(rect, 5, 5);
    painter->setClipPath(clip);
    int numPhases = (int)mEvent->mPhases.size();
    float w = r.width()/numPhases;
    for(int i=0; i<numPhases; ++i)
    {
        QColor c = mEvent->mPhases[i]->mColor;
        painter->setPen(c);
        painter->setBrush(c);
        painter->drawRect(r.x() + i*w, r.y(), w, r.height());
    }
    painter->setClipRect(rect.adjusted(-1, -1, 1, 1));
    painter->setPen(QPen(eventColor, 1));
    painter->drawLine(rect.x(), rect.y() + y, rect.x() + rect.width(), rect.y() + y);*/
    
    
    // Name
    QRectF tr(rect.x() + mBorderWidth + 2*mEltsMargin + mTitleHeight,
              rect.y() + mBorderWidth + mEltsMargin,
              rect.width() - 2*mBorderWidth - 2*(mTitleHeight + 2*mEltsMargin),
              mTitleHeight);
    
    QFont font = qApp->font();
    painter->setFont(font);
    QFontMetrics metrics(font);
    QString name = mEvent[STATE_EVENT_NAME].toString();
    name = metrics.elidedText(name, Qt::ElideRight, tr.width());
    
    QColor frontColor = getContrastedColor(eventColor);
    painter->setPen(frontColor);
    painter->drawText(tr, Qt::AlignCenter, name);
    
    
    // Border
    painter->setBrush(Qt::NoBrush);
    if(mMergeable)
    {
        painter->setPen(QPen(mainColorLight, 3.f, Qt::DashLine));
        painter->drawRect(rect.adjusted(1, 1, -1, -1));
    }
    else if(isSelected())
    {
        painter->setPen(QPen(mainColorDark, 3.f));
        painter->drawRect(rect.adjusted(1, 1, -1, -1));
    }
    
    painter->restore();
}

#pragma mark Geometry
QRectF EventItem::boundingRect() const
{
    qreal penWidth = 1;
    qreal w = 100.;
    
    float h = mTitleHeight + mPhasesHeight + 2*mBorderWidth + 2*mEltsMargin;
    if(mShowElts)
    {
        QString name = mEvent[STATE_EVENT_NAME].toString();
        QJsonArray dates = mEvent[STATE_EVENT_DATES].toArray();
        
        QFont font = qApp->font();
        QFontMetrics metrics(font);
        w = metrics.width(name) + 2*mBorderWidth + 4*mEltsMargin + 2*mTitleHeight;
        
        int count = dates.size();
        
        if(count > 0)
            h += count * (mEltsHeight + mEltsMargin);
        else
            h += mEltsMargin + mEltsHeight;
        
        font.setPointSizeF(pointSize(11));
        metrics = QFontMetrics(font);
        for(int i=0; i<count; ++i)
        {
            QJsonObject date = dates[i].toObject();
            name = date[STATE_DATE_NAME].toString();
            int nw = metrics.width(name) + 2*mBorderWidth + 4*mEltsMargin;
            w = (nw > w) ? nw : w;
        }
        w = (w < 150) ? 150 : w;
    }
    return QRectF(-(w+penWidth)/2, -(h+penWidth)/2, w + penWidth, h + penWidth);
}

QRectF EventItem::toggleRect() const
{
    QRectF rect = boundingRect();
    QRectF r(rect.x() + rect.width() - mBorderWidth - mEltsMargin - mTitleHeight,
             rect.y() + mBorderWidth + mEltsMargin,
             mTitleHeight,
             mTitleHeight);
    return r;
}

