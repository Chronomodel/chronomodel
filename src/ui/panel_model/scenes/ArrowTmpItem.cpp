#include "ArrowTmpItem.h"
#include <QtWidgets>


ArrowTmpItem::ArrowTmpItem(QGraphicsItem* parent):QGraphicsItem(parent),
mBubbleHeight(40.f),
mXFrom(0),
mYFrom(0),
mXTo(0),
mYTo(0),
mState(eNormal),
mLocked(false)
{
    // set the Arrow over all item
    setZValue(2.);
}

void ArrowTmpItem::setFrom(double x, double y)
{
    mXFrom = x;
    mYFrom = y;
}

void ArrowTmpItem::setTo(double x, double y)
{
    prepareGeometryChange();
    if (!mLocked) {
        mXTo = x;
        mYTo = y;
      //  update();
     /*   if (scene())
            scene()->update();*/
    }
   // update();
}

void ArrowTmpItem::setState(const State state)
{
    mState = state;
    prepareGeometryChange();
   // update();
}

void ArrowTmpItem::setLocked(bool locked)
{
    mLocked = locked;
}

QRectF ArrowTmpItem::boundingRect() const
{
    qreal x = qMin(mXFrom, mXTo);
    qreal y = qMin(mYFrom, mYTo);
    qreal w = qAbs(mXTo - mXFrom);
    qreal h = qAbs(mYTo - mYFrom);

    const QString text = getBubbleText();

    if (!text.isEmpty()) {
        const qreal xa = (mXFrom + mXTo- mBubbleHeight)/2;
        const qreal ya = (mYFrom + mYTo- mBubbleHeight)/2;

        x = qMin(x, xa);
        y = qMin(y, ya);
        w = qMax(w, mBubbleHeight);
        h = qMax(h, mBubbleHeight);
    }

    return QRectF(x, y, w, h);

}

QString ArrowTmpItem::getBubbleText() const
{
    QString bubbleText = ""; //mState == eNormal
    if (mState == eForbidden)
        bubbleText = "X";
    else if (mState == eAllowed)
        bubbleText = "OK";
     return bubbleText;
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
    
    painter->setRenderHint(QPainter::Antialiasing);
    const int penWidth = 2;
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
    
    const double angle_rad = atanf(qAbs(mXFrom-mXTo) / qAbs(mYFrom-mYTo));
    const double angle_deg = angle_rad * 180. / M_PI;
    
    QPainterPath path;
    const int arrow_w = 10;
    const int arrow_l = 15;
    path.moveTo(-arrow_w/2, arrow_l/2);
    path.lineTo(arrow_w/2, arrow_l/2);
    path.lineTo(0, -arrow_l/2);
    path.closeSubpath();
    
    const double posX = rect.width()/2;
    const double posY = rect.height()/2;

    const QRectF axeBox = QRectF(qMin(mXFrom, mXTo), qMin(mYFrom, mYTo), qAbs(mXTo-mXFrom), qAbs(mYTo-mYFrom));
    
    if (mXFrom < mXTo && mYFrom > mYTo) {
        painter->save();
        painter->translate(axeBox.x() + posX, axeBox.y() + posY);
        painter->rotate(angle_deg);
        painter->fillPath(path, color);
        painter->restore();
        
    } else if (mXFrom < mXTo && mYFrom < mYTo) {
        painter->save();
        painter->translate(axeBox.x() + posX, axeBox.y() + posY);
        painter->rotate(180 - angle_deg);
        painter->fillPath(path, color);
        painter->restore();
        
    } else if (mXFrom > mXTo && mYFrom < mYTo) {
        painter->save();
        painter->translate(axeBox.x() + posX, axeBox.y() + posY);
        painter->rotate(180 + angle_deg);
        painter->fillPath(path, color);
        painter->restore();

    } else if (mXFrom > mXTo && mYFrom > mYTo) {
        painter->save();
        painter->translate(axeBox.x() + axeBox.width()/2, axeBox.y() + axeBox.height()/2);
        painter->rotate(-angle_deg);
        painter->fillPath(path, color);
        painter->restore();
    }
    
    // Message
    const qreal w = mBubbleHeight;
    const qreal h = mBubbleHeight;
    QRectF r(axeBox.x() + (axeBox.width() - w)/2, axeBox.y() + (axeBox.height() - h)/2, w, h);
    QRectF rTex(axeBox.x() + (axeBox.width() - w)/2, axeBox.y() + (axeBox.height() - h)/2, w, h);
    QFont font (qApp->font());
    font.setBold(true);
    font.setPointSizeF(20.f); //depend of the font
    painter->setFont(font);
    switch (mState) {

        case eForbidden:
            qDebug() <<"ArrowTmpItem::paint mSate==eForbidden";

            painter->setBrush(Qt::white);
            painter->drawEllipse(r);
            painter->setPen(QPen(color, penWidth, Qt::SolidLine, Qt::RoundCap));
            painter->drawLine(r.x() + r.width()/4, r.y() + r.height()/4, r.x() + 3*r.width()/4, r.y() + 3*r.height()/4);
            painter->drawLine(r.x() + r.width()/4, r.y() + 3*r.height()/4, r.x() + 3*r.width()/4, r.y() + r.height()/4);
             break;

        case eAllowed:
            qDebug() <<"ArrowTmpItem::paint mSate==eAllowed";
            painter->setBrush(Qt::white);
            painter->drawEllipse(r);


            painter->drawText(rTex,  getBubbleText(), QTextOption(Qt::AlignCenter));


        default:
            break;
    }
}



