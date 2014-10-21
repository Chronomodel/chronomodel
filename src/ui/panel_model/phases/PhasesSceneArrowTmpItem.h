#ifndef PhasesSceneArrowTmpItem_H
#define PhasesSceneArrowTmpItem_H

#include <QObject>
#include <QGraphicsItem>

class PhasesScene;


class PhasesSceneArrowTmpItem: public QGraphicsItem
{
public:
    enum State{
        eNormal,
        eAllowed,
        eForbidden
    };
    
    PhasesSceneArrowTmpItem(PhasesScene* phasesView, QGraphicsItem* parent = 0);
    
    void setFrom(double x, double y);
    void setTo(double x, double y);
    void setState(const State state);
    void setLocked(bool locked);
    
    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    
protected:
    PhasesScene* mPhasesScene;
    
    double mXFrom;
    double mYFrom;
    double mXTo;
    double mYTo;
    State mState;
    bool mLocked;
};

#endif
