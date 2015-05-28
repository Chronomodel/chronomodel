#ifndef ArrowTmpItem_H
#define ArrowTmpItem_H

#include <QGraphicsItem>


class ArrowTmpItem: public QGraphicsItem
{
public:
    enum State{
        eNormal,
        eAllowed,
        eForbidden
    };
    
    ArrowTmpItem(QGraphicsItem* parent = 0);
    
    void setFrom(double x, double y);
    void setTo(double x, double y);
    void setState(const State state);
    void setLocked(bool locked);
    
    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    
protected:
    double mXFrom;
    double mYFrom;
    double mXTo;
    double mYTo;
    State mState;
    bool mLocked;
};

#endif
