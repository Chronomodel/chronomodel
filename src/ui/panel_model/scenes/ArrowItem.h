#ifndef ArrowItem_H
#define ArrowItem_H

#include <QObject>
#include <QGraphicsItem>
#include <QJsonObject>

class AbstractScene;
class AbstractItem;
class EventItem;

class ArrowItem: public QGraphicsItem
{
public:
    enum Type{
        eEvent = 0,
        ePhase = 1
    };
    
    ArrowItem(AbstractScene* scene, Type type, const QJsonObject& constraint, QGraphicsItem* parent = nullptr);
    virtual ~ArrowItem();

    void updatePosition();
    void setFrom(const double x, const double y);
    void setTo(const double x, const double y);
    
    void setGreyedOut(bool greyedOut);

    QRectF boundingRect() const;
    QPainterPath shape() const;
    
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e);   
    void mousePressEvent(QGraphicsSceneMouseEvent* e);
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* e);
    
    QRectF getBubbleRect(const QString& text = QString()) const;
    QSize getBubbleSize(const QString& text = QString()) const;
    QString getBubbleText() const;
    
    void setData(const QJsonObject& c);
    QJsonObject& data();

    EventItem* findEventItemWithJsonId(const int id);

public:
    Type mType;
    QJsonObject mData;
    AbstractScene* mScene;
    
    qreal mXStart;
    qreal mYStart;
    qreal mXEnd;
    qreal mYEnd;
    
    qreal mBubbleWidth;
    qreal mBubbleHeight;
    
    bool mEditing;
    bool mShowDelete;
    
    bool mGreyedOut;
};

#endif
