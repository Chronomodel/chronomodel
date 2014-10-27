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

    QRectF boundingRect() const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e);
    
    QRectF getBubbleRect() const;
    
    void setConstraint(const QJsonObject& c);
    QJsonObject& constraint();
    
protected:
    Type mType;
    QJsonObject mConstraint;
    AbstractScene* mScene;
    
    double mXStart;
    double mYStart;
    double mXEnd;
    double mYEnd;
    
    float mBubbleWidth;
    float mBubbleHeight;
    
    bool mEditing;
};

#endif
