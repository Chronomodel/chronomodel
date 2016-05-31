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
    
    void setControlsVisible(const bool visible);
    void setControlsEnabled(const bool enabled);

protected:
    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    void mousePressEvent(QGraphicsSceneMouseEvent* e);
    
    void hoverEnterEvent(QGraphicsSceneHoverEvent* e);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* e);

    QRectF checkRect() const;
    QRectF eyeRect() const;
    QRectF extractRect() const;
    QJsonArray getEvents() const;
    QString getTauString() const;
    
public:
    Qt::CheckState mState;
    bool mEyeActivated;
    bool mControlsVisible;
    bool mControlsEnabled;
    
    QSize mSize;
};

#endif
