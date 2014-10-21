#ifndef EventItem_H
#define EventItem_H

#include <QObject>
#include <QGraphicsItem>
#include <QJsonObject>
#include "EventsScene.h"

class EventItem : public QGraphicsItem
{
public:
    EventItem(EventsScene* EventsScene, const QJsonObject& event, QGraphicsItem* parent = 0);
    virtual ~EventItem();

    QJsonObject& event();
    void setEvent(const QJsonObject& event);
    
    virtual QRectF boundingRect() const;
    void setMergeable(bool mergeable);
    void handleDrop(QGraphicsSceneDragDropEvent* e);
    
protected:
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* e);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* e);
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* e);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* e);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* e);
    virtual void dropEvent(QGraphicsSceneDragDropEvent* e);
    
    virtual QRectF toggleRect() const;
    
public:
    EventsScene* mEventsScene;
    QJsonObject mEvent;
    
    float mBorderWidth;
    float mTitleHeight;
    float mPhasesHeight;
    float mEltsMargin;
    float mEltsWidth;
    float mEltsHeight;
    
    bool mShowElts;
    bool mMergeable;
};

#endif
