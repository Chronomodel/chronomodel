#ifndef EventsSceneArrowItem_H
#define EventsSceneArrowItem_H

#include <QObject>
#include <QGraphicsItem>

class EventsScene;
class EventConstraint;
class EventItem;


class EventsSceneArrowItem: public QGraphicsItem
{
public:
    EventsSceneArrowItem(EventsScene* EventsScene, EventConstraint* constraint = 0, QGraphicsItem* parent = 0);
    
    void setItemFrom(EventItem* itemFrom);
    void setItemTo(EventItem* itemTo);
    void updatePosition();

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e);
    
    QRectF getBubbleRect() const;
    
    EventConstraint* mConstraint;
    EventItem* mItemFrom;
    EventItem* mItemTo;
    
protected:
    EventsScene* mEventsScene;
    
    double mXStart;
    double mYStart;
    double mXEnd;
    double mYEnd;
    
    float mBubbleWidth;
    float mBubbleHeight;
    float mDeleteWidth;
    
    bool mEditing;
};

#endif
