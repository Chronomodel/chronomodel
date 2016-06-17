#ifndef DateItem_H
#define DateItem_H

#include <QObject>
#include <QGraphicsObject>
#include <QJsonObject>
#include <QColor>
#include "EventsScene.h"
#include "ProjectSettings.h"


class DateItem : public QGraphicsObject
{
    Q_OBJECT
public:
    DateItem(EventsScene* EventsScene, const QJsonObject& date, const QColor& color, const QJsonObject& settings, QGraphicsItem* parent = 0);

    const QJsonObject& date() const;
    void setOriginalPos(const QPointF pos);
    
    void setGreyedOut(bool greyedOut);
    
    QRectF boundingRect() const;
    
protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    void mousePressEvent(QGraphicsSceneMouseEvent* e);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* e);
    void dropEvent(QGraphicsSceneDragDropEvent* e);
    
public:
    EventsScene* mEventsScene;
    QJsonObject mDate;
    QColor mColor;
    QPointF mOriginalPos;
    QPixmap mCalibThumb;
    bool mGreyedOut;
};

#endif
