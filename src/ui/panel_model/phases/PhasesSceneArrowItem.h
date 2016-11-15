#ifndef PhasesSceneArrowItem_H
#define PhasesSceneArrowItem_H

#include <QObject>
#include <QGraphicsItem>

class PhasesScene;
class PhaseConstraint;
class PhasesItem;


class PhasesSceneArrowItem: public QGraphicsItem
{
public:
    PhasesSceneArrowItem(PhasesScene* phasesView, PhaseConstraint* constraint = 0, QGraphicsItem* parent = 0);
    
    void setItemFrom(PhasesItem* itemFrom);
    void setItemTo(PhasesItem* itemTo);
    void updatePosition();

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e);
    
    QRectF getBubbleRect() const;
    
    PhaseConstraint* mConstraint;
    PhasesItem* mItemFrom;
    PhasesItem* mItemTo;
    
protected:
    PhasesScene* mPhasesScene;
    
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
