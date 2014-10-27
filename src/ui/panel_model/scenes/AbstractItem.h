#ifndef AbstractItem_H
#define AbstractItem_H

#include <QGraphicsObject>
#include <QJsonObject>
#include "AbstractScene.h"

class AbstractItem : public QGraphicsObject
{
public:
    AbstractItem(AbstractScene* scene, QGraphicsItem* parent = 0);
    virtual ~AbstractItem();

    void setMergeable(bool mergeable);
    
protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* e);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* e);
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* e);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* e);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* e);
    
public:
    AbstractScene* mScene;
    
    float mBorderWidth;
    float mTitleHeight;
    float mPhasesHeight;
    float mEltsMargin;
    float mEltsWidth;
    float mEltsHeight;
    
    bool mMoving;
    bool mMergeable;
};

#endif
