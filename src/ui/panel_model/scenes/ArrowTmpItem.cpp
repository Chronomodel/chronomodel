#include "ArrowTmpItem.h"
#include <QtWidgets>


ArrowTmpItem::ArrowTmpItem(QGraphicsItem* parent):QGraphicsItem(parent),
mBubbleHeight(30.),
mXFrom(0.),
mYFrom(0.),
mXTo(0.),
mYTo(0.),
mState(eNormal),
mLocked(false)
{
    // set the Arrow over all item
    setZValue(2.);
}

void ArrowTmpItem::setFrom(const double& x, const double& y)
{
    mXFrom = x;
    mYFrom = y;
}

void ArrowTmpItem::setTo(const double& x, const double& y)
{
    prepareGeometryChange();
    if (!mLocked) {
        mXTo = x;
        mYTo = y;
    }
}

void ArrowTmpItem::setState(const State state)
{
    prepareGeometryChange();
    mState = state;
}

void ArrowTmpItem::setLocked(bool locked)
{
    mLocked = locked;
}

QRectF ArrowTmpItem::boundingRect() const
{
    const int penWidth = 2;
    const qreal arrow_w (15.);
    qreal x = qMin(mXFrom, mXTo);
    qreal y = qMin(mYFrom, mYTo);
    qreal w = std::abs(mXTo - mXFrom) + 2*penWidth + arrow_w; // when the arrow is horizontal
    qreal h = std::abs(mYTo - mYFrom) + 2*penWidth + arrow_w; // when the arrow is vertical

    const QString text = getBubbleText();

    if (!text.isEmpty()) {
        const qreal xa = (mXFrom + mXTo - mBubbleHeight)/2.;
        const qreal ya = (mYFrom + mYTo - mBubbleHeight)/2.;

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
    const int penWidth (2);
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
    
    const double angle_rad = atan(std::abs(mXFrom-mXTo) / std::abs(mYFrom-mYTo));
    const double angle_deg = angle_rad * 180. / M_PI;
    
    QPainterPath path;
    const qreal arrow_w (10.);
    const qreal arrow_l (15.);
    path.moveTo(-arrow_w/2., arrow_l/2.);
    path.lineTo(arrow_w/2., arrow_l/2.);
    path.lineTo(0, -arrow_l/2.);
    path.closeSubpath();
    
    //const double posX = rect.width()/2.;
    //const double posY = rect.height()/2.;

   // const QRectF axeBox = QRectF(qMin(mXFrom, mXTo), qMin(mYFrom, mYTo), std::abs(mXTo-mXFrom), std::abs(mYTo-mYFrom));
     const QPoint centrum ((mXFrom + mXTo)/2., (mYFrom + mYTo)/2.);

    if (mXFrom < mXTo && mYFrom > mYTo) {
        painter->save();
        painter->translate(centrum.x(), centrum.y());
        painter->rotate(angle_deg);
        painter->fillPath(path, color);
        painter->restore();
        
    } else if (mXFrom < mXTo && mYFrom < mYTo) {
        painter->save();        
        painter->translate(centrum.x(), centrum.y());
        painter->rotate(180 - angle_deg);
        painter->fillPath(path, color);
        painter->restore();
        
    } else if (mXFrom > mXTo && mYFrom < mYTo) {
        painter->save();        
        painter->translate(centrum.x(), centrum.y());
        painter->rotate(180 + angle_deg);
        painter->fillPath(path, color);
        painter->restore();

    } else if (mXFrom > mXTo && mYFrom > mYTo) {
        painter->save();
        painter->translate(centrum.x(), centrum.y());
        painter->rotate(-angle_deg);
        painter->fillPath(path, color);
        painter->restore();
    }
    
    // Message
    const qreal ra (mBubbleHeight/2. * 0.7 * .5); // radius of the cercle * sin(45°) * coef

    QRectF r(centrum.x() - mBubbleHeight/2., centrum.y() - mBubbleHeight/2., mBubbleHeight, mBubbleHeight);
    QFont font (qApp->font());
    font.setBold(true);
    font.setPointSizeF(mBubbleHeight*0.4); //depend of the font
    QFontMetrics fm (font);
    painter->setFont(font);
    painter->setRenderHint(QPainter::Antialiasing);

    QRectF rTex(centrum.x() - fm.width(getBubbleText())/2., centrum.y() - fm.ascent()/2., fm.width(getBubbleText()), fm.height());

    switch (mState) {

        case eForbidden:
            qDebug() <<"ArrowTmpItem::paint mSate==eForbidden";
            painter->setBrush(Qt::white);
            painter->drawEllipse(r);
            painter->setPen(QPen(color, penWidth, Qt::SolidLine, Qt::RoundCap));
            painter->drawLine(centrum.x() - ra, centrum.y() - ra, centrum.x() + ra, centrum.y() + ra);
            painter->drawLine(centrum.x() - ra, centrum.y() + ra, centrum.x() + ra, centrum.y() - ra);

            break;

        case eAllowed:
            qDebug() <<"ArrowTmpItem::paint mSate==eAllowed";
            painter->setBrush(Qt::white);
            painter->drawEllipse(r);

            painter->drawText(rTex,  getBubbleText());//, QTextOption(Qt::AlignCenter));

        default:
            break;
    }
}



