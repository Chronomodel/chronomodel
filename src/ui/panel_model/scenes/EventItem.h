#ifndef EventItem_H
#define EventItem_H

#include "AbstractItem.h"
#include <QJsonObject>

class EventsScene;


class EventItem : public AbstractItem
{
public:
    EventItem(EventsScene* scene, const QJsonObject& event, QGraphicsItem* parent = 0);
    virtual ~EventItem();

    QJsonObject& event();
    void setEvent(const QJsonObject& event);
    
    virtual QRectF boundingRect() const;
    void handleDrop(QGraphicsSceneDragDropEvent* e);
    QJsonArray getPhases() const;
    
protected:
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* e);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* e);
    virtual void dropEvent(QGraphicsSceneDragDropEvent* e);
    
public:
    QJsonObject mEvent;
};

#endif
