#ifndef PhasesItem_H
#define PhasesItem_H

#include <QGraphicsObject>
#include "PhasesScene.h"
#include "Phase.h"


class PhasesItem : public QGraphicsObject
{
    Q_OBJECT
public:
    PhasesItem(PhasesScene* phasesView, const QJsonObject& phase, QGraphicsItem* parent = 0);
    virtual ~PhasesItem();
    
    QJsonObject& phase();
    void setPhase(const QJsonObject& phase);
    
    void setState(Qt::CheckState state);

protected:
    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    void mousePressEvent(QGraphicsSceneMouseEvent* e);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* e);
    void hoverEnterEvent(QGraphicsSceneHoverEvent* e);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* e);
    
    QRectF insideRect() const;
    QRectF toggleRect() const;
    QRectF checkRect() const;
    QRectF colorRect() const;
    
private slots:
    void stateChanged(bool checked);
    
public:
    PhasesScene* mPhasesScene;
    QJsonObject mPhase;
    
    float mBorderWidth;
    float mTitleHeight;
    float mEltsMargin;
    float mEltsWidth;
    float mEltsHeight;
    bool mShowElts;
    
    Qt::CheckState mState;
};

#endif
