#ifndef EventKnownItem_H
#define EventKnownItem_H

#include "EventItem.h"
#include "Event.h"

class EventKnownItem : public EventItem
{
public:
    EventKnownItem(EventsScene* eventsScene, const QJsonObject& event, QGraphicsItem* parent = 0);
    virtual ~EventKnownItem();

    virtual QRectF boundingRect() const;
    
protected:
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    virtual void dropEvent(QGraphicsSceneDragDropEvent* e);
    
    virtual QRectF toggleRect() const;
};

#endif
