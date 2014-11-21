#ifndef EventItem_H
#define EventItem_H

#include "AbstractItem.h"

class EventsScene;


class EventItem : public AbstractItem
{
public:
    EventItem(EventsScene* scene, const QJsonObject& event, const QJsonObject& settings, QGraphicsItem* parent = 0);
    virtual ~EventItem();

    virtual void setGreyedOut(bool greyedOut);
    
    QJsonObject& getEvent();
    virtual void setEvent(const QJsonObject& event, const QJsonObject& settings);
    
    virtual QRectF boundingRect() const;
    void handleDrop(QGraphicsSceneDragDropEvent* e);
    QJsonArray getPhases() const;
    
    virtual void updateItemPosition(const QPointF& pos);
    
protected:
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    virtual void dropEvent(QGraphicsSceneDragDropEvent* e);
    
    void updateGreyedOut();
};

#endif
