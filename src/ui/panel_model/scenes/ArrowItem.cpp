#include "ArrowItem.h"
#include "EventItem.h"
#include "EventConstraint.h"
#include "MainWindow.h"
#include "Project.h"
#include "Painting.h"
#include <QtWidgets>
#include <math.h>


ArrowItem::ArrowItem(AbstractScene* scene, Type type, const QJsonObject& constraint, QGraphicsItem* parent):QGraphicsItem(parent),
mType(type),
mScene(scene),
mXStart(0),
mYStart(0),
mXEnd(0),
mYEnd(0),
mBubbleWidth(130.f),
mBubbleHeight(20.f),
mEditing(false),
mShowDelete(false),
mGreyedOut(false)
{
    setZValue(-1.);
    setAcceptHoverEvents(true);
    setFlags(QGraphicsItem::ItemIsSelectable |
            QGraphicsItem::ItemIsFocusable |
            QGraphicsItem::ItemSendsScenePositionChanges |
            QGraphicsItem::ItemSendsGeometryChanges);
    
    setData(constraint);
}

QJsonObject& ArrowItem::data()
{
    return mData;
}

void ArrowItem::setData(const QJsonObject& c)
{
    mData = c;
    updatePosition();
}

void ArrowItem::setFrom(const double x, const double y)
{
    prepareGeometryChange();
    mXStart = x;
    mYStart = y;
    update();
    if (scene())
        scene()->update();
}

void ArrowItem::setTo(const double x, const double y)
{
    prepareGeometryChange();
    mXEnd = x;
    mYEnd = y;
}

void ArrowItem::setGreyedOut(bool greyedOut)
{
    mGreyedOut = greyedOut;
    update();
}

void ArrowItem::updatePosition()
{
    prepareGeometryChange();
    
    Project* project = MainWindow::getInstance()->getProject();
    QJsonObject state = project->state();
    
    const int fromId = mData.value(STATE_CONSTRAINT_BWD_ID).toInt();
    const int toId = mData.value(STATE_CONSTRAINT_FWD_ID).toInt();
    
    QJsonObject from;
    QJsonObject to;
    
    if (mType == eEvent) {
        const QJsonArray events = state.value(STATE_EVENTS).toArray();
        for (int i=0; i<events.size(); ++i) {
            const QJsonObject event = events.at(i).toObject();
            if (event.value(STATE_ID).toInt() == fromId)
                from = event;
            if (event.value(STATE_ID).toInt() == toId)
                to = event;
        }
    } else {
        const QJsonArray phases = state.value(STATE_PHASES).toArray();
        for (int i=0; i<phases.size(); ++i) {
            const QJsonObject phase = phases.at(i).toObject();
            if (phase.value(STATE_ID).toInt() == fromId)
                from = phase;
            if (phase.value(STATE_ID).toInt() == toId)
                to = phase;
        }
    }

    mXStart = from.value(STATE_ITEM_X).toDouble();
    mYStart = from.value(STATE_ITEM_Y).toDouble();
    
    mXEnd = to.value(STATE_ITEM_X).toDouble();
    mYEnd = to.value(STATE_ITEM_Y).toDouble();
    
    //qDebug() << "[" << mXStart << ", " << mYStart << "]" << " => " << "[" << mXEnd << ", " << mYEnd << "]";

}

void ArrowItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e)
{
    //qDebug()<<"ArrowItem::mouseDoubleClickEvent";
    QGraphicsItem::mouseDoubleClickEvent(e);
    mScene->constraintDoubleClicked(this, e);
}

void ArrowItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
   //  qDebug()<<"ArrowItem::mousePressEvent";
    QGraphicsItem::mousePressEvent(e);
    const QRectF r = getBubbleRect(getBubbleText());
    if (r.contains(e->pos()))
        mScene->constraintClicked(this, e);

}

void ArrowItem::hoverMoveEvent(QGraphicsSceneHoverEvent* e)
{
    //qDebug()<<"ArrowItem::hoverMoveEvent----->";
    QGraphicsItem::hoverMoveEvent(e);
    prepareGeometryChange();
    //const double hoverSide = 100;
    const QRectF br = boundingRect();

   /* br.adjust((br.width()-hoverSide)/2,
              (br.height()-hoverSide)/2,
              -(br.width()-hoverSide)/2,
              -(br.height()-hoverSide)/2);*/

    const bool shouldShowDelete = br.contains(e->pos());
    if (shouldShowDelete != mShowDelete) {
        mShowDelete = shouldShowDelete;
        update();
    }

}
void ArrowItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* e)
{
    QGraphicsItem::hoverLeaveEvent(e);
    prepareGeometryChange();
    if (mShowDelete) {
        mShowDelete = false;
        update();
    }
}

QRectF ArrowItem::boundingRect() const
{
    const QString text = getBubbleText();

    qreal x = qMin(mXStart, mXEnd);
    qreal y = qMin(mYStart, mYEnd);
    qreal w = qAbs(mXEnd - mXStart);
    qreal h = qAbs(mYEnd - mYStart);

    if (!text.isEmpty()) {
        const qreal hBubble = mBubbleHeight;
        QFont font;
        font.setPointSizeF(11.f);
        QFontMetrics metrics(font);
        qreal wBubble =metrics.width(text) + 20;
        qreal xa = (mXStart + mXEnd- wBubble)/2;
        qreal ya = (mYStart + mYEnd- hBubble)/2;

        x = qMin(x, xa);
        y = qMin(y, ya);
        w = qMax(w, wBubble);
        h = qMax(h, hBubble);
    }

    return QRectF(x, y, w, h);
}

QPainterPath ArrowItem::shape() const
{
    QPainterPath path;
    QRectF rect = boundingRect();
    const qreal shift = 15;
    
    if (mXStart < mXEnd && mYStart > mYEnd) {
        path.moveTo(mXStart + shift, mYStart);
        path.lineTo(mXStart, mYStart - shift);
        path.lineTo(mXEnd - shift, mYEnd);
        path.lineTo(mXEnd, mYEnd + shift);
    } else if (mXStart < mXEnd && mYStart < mYEnd) {
        path.moveTo(mXStart + shift, mYStart);
        path.lineTo(mXStart, mYStart + shift);
        path.lineTo(mXEnd - shift, mYEnd);
        path.lineTo(mXEnd, mYEnd - shift);
    } else if (mXStart > mXEnd && mYStart < mYEnd) {
        path.moveTo(mXStart - shift, mYStart);
        path.lineTo(mXStart, mYStart + shift);
        path.lineTo(mXEnd + shift, mYEnd);
        path.lineTo(mXEnd, mYEnd - shift);
    } else if (mXStart > mXEnd && mYStart > mYEnd) {
        path.moveTo(mXStart - shift, mYStart);
        path.lineTo(mXStart, mYStart - shift);
        path.lineTo(mXEnd + shift, mYEnd);
        path.lineTo(mXEnd, mYEnd + shift);
    } else
        path.addRect(rect);

    return path;
}

void ArrowItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);
    QRectF rect = boundingRect();
    
    int penWidth = 1;
    QColor color = mEditing ? QColor(77, 180, 62) : QColor(0, 0, 0);
    //set the Arrow under the Event
    setZValue(-1);

    if (mShowDelete) {
        color = Qt::red;
        //set the Arrow in front of the Event, like this we can click on
        setZValue(1);
    } else if (mGreyedOut)
        color.setAlphaF(0.05);
    
    painter->setPen(QPen(color, penWidth, mEditing ? Qt::DashLine : Qt::SolidLine));
    painter->drawLine(mXStart, mYStart, mXEnd, mYEnd);
    
    // Bubble
    
    const QString bubbleText = getBubbleText();
    bool showMiddleArrow = true;
    QFont font;

    if (mShowDelete) {

        showMiddleArrow = false;
        painter->setPen(Qt::white);
        painter->setBrush(Qt::red);
        font.setBold(true);
        font.setPointSizeF(18.f);
    } else {
        painter->setPen(Qt::black);
        painter->setBrush(Qt::white);
        font.setBold(false);
        font.setPointSizeF(12.f);
    }
    painter->setFont(font);

    if (!bubbleText.isEmpty()) {
        showMiddleArrow = false;
        const QRectF br = getBubbleRect(bubbleText);
        painter->drawEllipse(br);
        painter->drawText(br, Qt::AlignCenter, bubbleText);
     }

     /*  if (mType == eEvent) {
           painter->setPen(Qt::red);
           painter->setBrush(Qt::red);
           QRectF br = getBubbleRect();
           painter->drawEllipse(br);
           painter->save();
           painter->translate(rect.x() + rect.width()/2, rect.y() + rect.height()/2);
           painter->rotate(45.f);
           QPen pen;
           pen.setColor(Qt::white);
           pen.setCapStyle(Qt::RoundCap);
           pen.setWidthF(4.f);
           painter->setPen(pen);
           const double r = br.width()/3.f;
           painter->drawLine(-r, 0.f, r, 0.f);
           painter->drawLine(0.f, -r, 0.f, r);
           painter->restore();
        } else {
           const QString info = QString("?");
           const QRectF br = getBubbleRect(info);
           painter->setPen(Qt::white);
           painter->setBrush(Qt::red);
           painter->drawEllipse(br);
           QFont font = painter->font();
           font.setBold(true);
           font.setPointSizeF(18.f);
           painter->setFont(font);
           painter->drawText(br, Qt::AlignCenter, info);
        }

    } else if (!bubbleText.isEmpty()) {
        showMiddleArrow = false;
        QRectF br = getBubbleRect(bubbleText);
        painter->setPen(Qt::black);
        painter->setBrush(Qt::white);
        painter->drawEllipse(br);
        QFont font = painter->font();
        font.setPointSizeF(11.f);
        painter->setFont(font);
        painter->drawText(br, Qt::AlignCenter, bubbleText);
    }
    */


    // arrows
    
    const double angle_rad = atanf(qAbs(mXStart-mXEnd) / qAbs(mYStart-mYEnd));
    const double angle_deg = angle_rad * 180. / M_PI;
    
    QPainterPath path;
    const int arrow_w = 10;
    const int arrow_l = 15;
    path.moveTo(-arrow_w/2, arrow_l/2);
    path.lineTo(arrow_w/2, arrow_l/2);
    path.lineTo(0, -arrow_l/2);
    path.closeSubpath();
    
    const QRectF axeBox = QRectF(qMin(mXStart, mXEnd), qMin(mYStart, mYEnd), qAbs(mXEnd-mXStart), qAbs(mYEnd-mYStart));
    /*const qreal posX = rect.width()/2;
    const qreal posY = rect.height()/2;
    const qreal posX1 = rect.width()/3;
    const qreal posX2 = 2*rect.width()/3;
    const qreal posY1 = rect.height()/3;
    const qreal posY2 = 2*rect.height()/3;*/
    const qreal posX = axeBox.width()/2;
        const qreal posY = axeBox.height()/2;
        const qreal posX1 = axeBox.width()/3;
        const qreal posX2 = 2*axeBox.width()/3;
        const qreal posY1 = axeBox.height()/3;
        const qreal posY2 = 2*axeBox.height()/3;
    
    if (mXStart < mXEnd && mYStart > mYEnd) {
        if (showMiddleArrow) {
            painter->save();
            painter->translate(axeBox.x() + posX, axeBox.y() + posY);
            painter->rotate(angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        } else {
            painter->save();
            painter->translate(axeBox.x() + posX1, axeBox.y() + posY2);
            painter->rotate(angle_deg);
            painter->fillPath(path, color);
            painter->restore();
            
            painter->save();
            painter->translate(axeBox.x() + posX2, axeBox.y() + posY1);
            painter->rotate(angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        }
    } else if (mXStart < mXEnd && mYStart < mYEnd) {
        if (showMiddleArrow) {
            painter->save();
            painter->translate(axeBox.x() + posX, axeBox.y() + posY);
            painter->rotate(180 - angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        } else {
            painter->save();
            painter->translate(axeBox.x() + posX1, axeBox.y() + posY1);
            painter->rotate(180 - angle_deg);
            painter->fillPath(path, color);
            painter->restore();
            
            painter->save();
            painter->translate(axeBox.x() + posX2, axeBox.y() + posY2);
            painter->rotate(180 - angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        }
    } else if (mXStart > mXEnd && mYStart < mYEnd) {
        if (showMiddleArrow) {
            painter->save();
            painter->translate(axeBox.x() + posX, axeBox.y() + posY);
            painter->rotate(180 + angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        } else {
            painter->save();
            painter->translate(axeBox.x() + posX2, axeBox.y() + posY1);
            painter->rotate(180 + angle_deg);
            painter->fillPath(path, color);
            painter->restore();
            
            painter->save();
            painter->translate(axeBox.x() + posX1, axeBox.y() + posY2);
            painter->rotate(180 + angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        }
    } else if (mXStart > mXEnd && mYStart > mYEnd) {
        if (showMiddleArrow) {
            painter->save();
            painter->translate(axeBox.x() + axeBox.width()/2, axeBox.y() + axeBox.height()/2);
            painter->rotate(-angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        } else {
            painter->save();
            painter->translate(axeBox.x() + 2*axeBox.width()/3, axeBox.y() + 2*axeBox.height()/3);
            painter->rotate(-angle_deg);
            painter->fillPath(path, color);
            painter->restore();
            
            painter->save();
            painter->translate(axeBox.x() + axeBox.width()/3, axeBox.y() + axeBox.height()/3);
            painter->rotate(-angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        }
    }
    

}

QRectF ArrowItem::getBubbleRect(const QString& text) const
{
    qreal w = 0;
    qreal h = 0;
    if (!text.isEmpty()) {
        QFont font;
        if (mShowDelete)
            font.setPointSizeF(18.f);
        else
            font.setPointSizeF(12.f);

        QFontMetrics metrics(font);
        qreal wm = metrics.width(text) + 20;
        w = qMax(wm, w);
        h = mBubbleHeight;
    }

    
    QRectF rect = boundingRect();
    qreal bubble_x = rect.x() + (rect.width() - w) / 2.f - 0.5f;
    qreal bubble_y = rect.y() + (rect.height() - h) / 2.f - 0.5f;
    QRectF r(bubble_x, bubble_y, w, h);
    return r;
}

QString ArrowItem::getBubbleText() const
{
    QString bubbleText;
    if (mShowDelete)
        if (mType == eEvent)
            bubbleText = "X";
        else
            bubbleText = "?";
    else if (mType == ePhase) {
            PhaseConstraint::GammaType gammaType = (PhaseConstraint::GammaType)mData.value(STATE_CONSTRAINT_GAMMA_TYPE).toInt();
            if (gammaType == PhaseConstraint::eGammaFixed)
                bubbleText = "hiatus ≥ " + QString::number(mData.value(STATE_CONSTRAINT_GAMMA_FIXED).toDouble());
            else if (gammaType == PhaseConstraint::eGammaRange)
                bubbleText = "min hiatus ∈ [" + QString::number(mData.value(STATE_CONSTRAINT_GAMMA_MIN).toDouble()) +
                "; " + QString::number(mData.value(STATE_CONSTRAINT_GAMMA_MAX).toDouble()) + "]";
        }

    return bubbleText;
}

EventItem* ArrowItem::findEventItemWithJsonId(const int id)
{
     QList<AbstractItem*> listItems = mScene->getItemsList();
     foreach (AbstractItem* it, listItems) {
        EventItem* ev = static_cast<EventItem*>(it);
        const QJsonObject evJson = ev->getEvent();
        if (evJson.value(STATE_ID)== id)
            return ev;
    }
}
