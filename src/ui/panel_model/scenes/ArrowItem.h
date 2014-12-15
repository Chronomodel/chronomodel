#ifndef ArrowItem_H
#define ArrowItem_H

#include <QObject>
#include <QGraphicsItem>
#include <QJsonObject>

class AbstractScene;
class AbstractItem;


class ArrowItem: public QGraphicsItem
{
public:
    enum Type{
        eEvent = 0,
        ePhase = 1
    };
    
    ArrowItem(AbstractScene* scene, Type type, const QJsonObject& constraint, QGraphicsItem* parent = 0);
    
    void updatePosition();
    void setFrom(float x, float y);
    void setTo(float x, float y);

    QRectF boundingRect() const;
    QPainterPath shape() const;
    
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e);
    void mousePressEvent(QGraphicsSceneMouseEvent* e);
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* e);
    
    QRectF getBubbleRect(const QString& text = QString()) const;
    QString getBubbleText() const;
    
    void setData(const QJsonObject& c);
    QJsonObject& data();
    
protected:
    Type mType;
    QJsonObject mData;
    AbstractScene* mScene;
    
    float mXStart;
    float mYStart;
    float mXEnd;
    float mYEnd;
    
    float mBubbleWidth;
    float mBubbleHeight;
    
    bool mEditing;
    bool mShowDelete;
};

#endif
