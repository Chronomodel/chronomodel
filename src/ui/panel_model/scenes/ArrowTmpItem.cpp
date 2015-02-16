#include "ArrowTmpItem.h"
#include <QtWidgets>


ArrowTmpItem::ArrowTmpItem(QGraphicsItem* parent):QGraphicsItem(parent),
mXFrom(0),
mYFrom(0),
mXTo(0),
mYTo(0),
mState(eNormal),
mLocked(false)
{
    setZValue(-1.);
}

void ArrowTmpItem::setFrom(double x, double y)
{
    mXFrom = x;
    mYFrom = y;
    
    update();
    if(scene())
        scene()->update();
}

void ArrowTmpItem::setTo(double x, double y)
{
    if(!mLocked)
    {
        mXTo = x;
        mYTo = y;
        update();
        if(scene())
            scene()->update();
    }
}

void ArrowTmpItem::setState(const State state)
{
    mState = state;
    update();
}

void ArrowTmpItem::setLocked(bool locked)
{
    mLocked = locked;
}

QRectF ArrowTmpItem::boundingRect() const
{
    double x = qMin(mXFrom, mXTo);
    double y = qMin(mYFrom, mYTo);
    double w = qAbs(mXTo - mXFrom);
    double h = qAbs(mYTo - mYFrom);

    return QRectF(x, y, w, h);
}
/**
 * @brief ArrowTmpItem::paint Draw the dach line linking Event or Phase
 * @param painter
 * @param option
 * @param widget
 */
void ArrowTmpItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    QRectF rect = boundingRect();
    
    //painter->fillRect(rect, QColor(255, 0, 0, 30));
    
    painter->setRenderHint(QPainter::Antialiasing);
    int penWidth = 2;
    QColor color = Qt::black;
    
    switch(mState)
    {
        case eNormal:
            color = Qt::black;
            break;
        case eAllowed:
            color = QColor(77, 180, 62);
            break;
        case eForbidden:
            color = Qt::red;
            break;
        default:
            break;
    }
    painter->setPen(QPen(color, penWidth, Qt::DashLine));
    painter->drawLine(mXFrom, mYFrom, mXTo, mYTo);
    
    // arrows
    
    double angle_rad = atanf(rect.width() / rect.height());
    double angle_deg = angle_rad * 180. / M_PI;
    
    QPainterPath path;
    int arrow_w = 10;
    int arrow_l = 15;
    path.moveTo(-arrow_w/2, arrow_l/2);
    path.lineTo(arrow_w/2, arrow_l/2);
    path.lineTo(0, -arrow_l/2);
    path.closeSubpath();
    
    double posX = rect.width()/2;
    double posY = rect.height()/2;
    
    if(mXFrom < mXTo && mYFrom > mYTo)
    {
        painter->save();
        painter->translate(rect.x() + posX, rect.y() + posY);
        painter->rotate(angle_deg);
        painter->fillPath(path, color);
        painter->restore();
    }
    else if(mXFrom < mXTo && mYFrom < mYTo)
    {
        painter->save();
        painter->translate(rect.x() + posX, rect.y() + posY);
        painter->rotate(180 - angle_deg);
        painter->fillPath(path, color);
        painter->restore();
    }
    else if(mXFrom > mXTo && mYFrom < mYTo)
    {
        painter->save();
        painter->translate(rect.x() + posX, rect.y() + posY);
        painter->rotate(180 + angle_deg);
        painter->fillPath(path, color);
        painter->restore();
    }
    else if(mXFrom > mXTo && mYFrom > mYTo)
    {
        painter->save();
        painter->translate(rect.x() + rect.width()/2, rect.y() + rect.height()/2);
        painter->rotate(-angle_deg);
        painter->fillPath(path, color);
        painter->restore();
    }
    
    // Message
    //qDebug() <<"mSate="<<QString::number(eAllowed);
    switch(mState)
    {
        case eAllowed:
        case eForbidden:
        {
            double w = 40;
            double h = 40;
            QRectF r(rect.x() + (rect.width() - w)/2, rect.y() + (rect.height() - h)/2, w, h);
            painter->setBrush(Qt::white);
            painter->drawEllipse(r);
            
            if(mState == eAllowed)
            {
                //qDebug() <<"if mSate==eAllowed"<<QString::number(eAllowed);
                painter->drawText(r, Qt::AlignCenter, "OK");
            }
            else
            {
                painter->setPen(QPen(color, penWidth, Qt::SolidLine, Qt::RoundCap));
                painter->drawLine(r.x() + r.width()/4, r.y() + r.height()/4, r.x() + 3*r.width()/4, r.y() + 3*r.height()/4);
                painter->drawLine(r.x() + r.width()/4, r.y() + 3*r.height()/4, r.x() + 3*r.width()/4, r.y() + r.height()/4);
            }
        }
        default:
            break;
    }
}



