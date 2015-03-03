#ifndef PhaseItem_H
#define PhaseItem_H

#include "AbstractItem.h"
#include "PhasesScene.h"
#include "Phase.h"


class PhaseItem : public AbstractItem
{
    Q_OBJECT
public:
    PhaseItem(AbstractScene* scene, const QJsonObject& phase, QGraphicsItem* parent = 0);
    virtual ~PhaseItem();
    
    QJsonObject& getPhase();
    void setPhase(const QJsonObject& phase);
    
    void setState(Qt::CheckState state);
    
    virtual void updateItemPosition(const QPointF& pos);
    
    void setControlsVisible(double visible);

protected:
    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    void mousePressEvent(QGraphicsSceneMouseEvent* e);
    
    QRectF checkRect() const;
    QRectF eyeRect() const;
    QJsonArray getEvents() const;
    QString getTauString() const;
    
public:
    Qt::CheckState mState;
    bool mEyeActivated;
    bool mControlsVisible;
};

#endif
