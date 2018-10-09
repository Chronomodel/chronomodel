#ifndef AbstractItem_H
#define AbstractItem_H

#include <QGraphicsObject>
#include <QJsonObject>
#include "AbstractScene.h"

class AbstractItem : public QGraphicsObject
{
public:
    AbstractItem(AbstractScene* scene, QGraphicsItem* parent = nullptr);
    virtual ~AbstractItem();

    void setMergeable(bool mergeable, bool shouldRepaint = true);
    virtual void setGreyedOut(const bool greyedOut);
    
    virtual void updateItemPosition(const QPointF& pos) = 0;
    void setSelectedInData(const bool selected);
    void setCurrentInData(const bool current);
    
protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* e);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* e);
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e);

    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* e);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* e);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* e);
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value);
    
public:
    QJsonObject mData;

    AbstractScene* mScene;
    
    int mBorderWidth;
    int mTitleHeight;
    int mPhasesHeight;
    int mEltsMargin;
    int mEltsWidth;
    int mEltsHeight;
    int mItemWidth;
    
    bool mMoving; // used in AbstractScene::itemReleased() to merge item like eventItem and phaseItem
    bool mMergeable;
    bool mGreyedOut;



};

#endif
