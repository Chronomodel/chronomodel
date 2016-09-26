#ifndef EventsSceneArrowTmpItem_H
#define EventsSceneArrowTmpItem_H

#include <QObject>
#include <QGraphicsItem>

class EventsScene;


class EventsSceneArrowTmpItem: public QGraphicsItem
{
public:
    enum State{
        eNormal,
        eAllowed,
        eForbidden
    };
    
    EventsSceneArrowTmpItem(EventsScene* EventsScene, QGraphicsItem* parent = 0);
    
    void setFrom(double x, double y);
    void setTo(double x, double y);
    void setState(const State state);
    void setLocked(bool locked);
    
    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    
protected:
    EventsScene* mEventsScene;
    
    double mXFrom;
    double mYFrom;
    double mXTo;
    double mYTo;
    State mState;
    bool mLocked;
};

#endif
