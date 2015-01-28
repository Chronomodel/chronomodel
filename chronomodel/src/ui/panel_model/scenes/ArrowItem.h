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
    void setFrom(double x, double y);
    void setTo(double x, double y);
    
    void setGreyedOut(bool greyedOut);

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
    
public:
    Type mType;
    QJsonObject mData;
    AbstractScene* mScene;
    
    double mXStart;
    double mYStart;
    double mXEnd;
    double mYEnd;
    
    double mBubbleWidth;
    double mBubbleHeight;
    
    bool mEditing;
    bool mShowDelete;
    
    bool mGreyedOut;
};

#endif
