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
    
    QJsonObject& phase();
    void setPhase(const QJsonObject& phase);
    
    void setState(Qt::CheckState state);

protected:
    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    void mousePressEvent(QGraphicsSceneMouseEvent* e);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* e);
    
    QRectF checkRect() const;
    QRectF eyeRect() const;
    QJsonArray getEvents() const;
    
private slots:
    void stateChanged(bool checked);
    
public:
    QJsonObject mPhase;
    Qt::CheckState mState;
    bool mEyeActivated;
};

#endif
