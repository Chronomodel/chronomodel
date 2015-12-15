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

    void setMergeable(bool mergeable, bool shouldRepaint = true);
    virtual void setGreyedOut(bool greyedOut, bool shouldRepaint = true);
    
    virtual void updateItemPosition(const QPointF& pos) = 0;
    
protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* e);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* e);
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e);
    //virtual void mouseClickEvent(QGraphicsSceneMouseEvent* e);//Added by PhD
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* e);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* e);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* e);
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value);
    
public:
    QJsonObject mData;

    AbstractScene* mScene;
    
    double mBorderWidth;
    double mTitleHeight;
    double mPhasesHeight;
    double mEltsMargin;
    double mEltsWidth;
    double mEltsHeight;
    
    bool mMoving;
    bool mMergeable;
    bool mGreyedOut;


};

#endif
