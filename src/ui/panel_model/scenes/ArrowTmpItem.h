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
    
    ArrowTmpItem(QGraphicsItem* parent = nullptr);
    
    void setFrom(const double& x, const double& y);
    void setTo(const double& x, const double& y);
    void setState(const State state);
    void setLocked(bool locked);
    
    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    
protected:

    QString getBubbleText() const;

    qreal mBubbleHeight;

    double mXFrom;
    double mYFrom;
    double mXTo;
    double mYTo;
    State mState;
    bool mLocked;
};

#endif
