#ifndef EventKnownItem_H
#define EventKnownItem_H

#include "EventItem.h"
#include "Event.h"


class EventKnownItem : public EventItem
{
public:
    EventKnownItem(EventsScene* eventsScene, const QJsonObject& event, const QJsonObject& settings, QGraphicsItem* parent = 0);
    virtual ~EventKnownItem();

    virtual QRectF boundingRect() const;
    
    void setEvent(const QJsonObject& event, const QJsonObject& settings);
    void setDatesVisible(bool visible);
    
protected:
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    virtual void dropEvent(QGraphicsSceneDragDropEvent* e);
    
    virtual QRectF toggleRect() const;
    
private:
    QImage mThumb;
    int mThumbH;
    bool mThumbVisible;
    int mPhasesHeight;
};

#endif
