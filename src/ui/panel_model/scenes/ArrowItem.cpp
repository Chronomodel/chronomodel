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
mBubbleHeight(40.f),
mEditing(false),
mShowDelete(false)
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

void ArrowItem::setFrom(double x, double y)
{
    prepareGeometryChange();
    mXStart = x;
    mYStart = y;
    update();
    if(scene())
        scene()->update();
}

void ArrowItem::setTo(double x, double y)
{
    prepareGeometryChange();
    mXEnd = x;
    mYEnd = y;
    update();
    if(scene())
        scene()->update();
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
    
    int fromId = mData[STATE_CONSTRAINT_BWD_ID].toInt();
    int toId = mData[STATE_CONSTRAINT_FWD_ID].toInt();
    
    QJsonObject from;
    QJsonObject to;
    
    if(mType == eEvent)
    {
        QJsonArray events = state[STATE_EVENTS].toArray();
        for(int i=0; i<events.size(); ++i)
        {
            QJsonObject event = events[i].toObject();
            if(event[STATE_ID].toInt() == fromId)
                from = event;
            if(event[STATE_ID].toInt() == toId)
                to = event;
        }
    }
    else
    {
        QJsonArray phases = state[STATE_PHASES].toArray();
        for(int i=0; i<phases.size(); ++i)
        {
            QJsonObject phase = phases[i].toObject();
            if(phase[STATE_ID].toInt() == fromId)
                from = phase;
            if(phase[STATE_ID].toInt() == toId)
                to = phase;
        }
    }

    mXStart = from[STATE_ITEM_X].toDouble();
    mYStart = from[STATE_ITEM_Y].toDouble();
    
    mXEnd = to[STATE_ITEM_X].toDouble();
    mYEnd = to[STATE_ITEM_Y].toDouble();
    
    //qDebug() << "[" << mXStart << ", " << mYStart << "]" << " => " << "[" << mXEnd << ", " << mYEnd << "]";
    
    update();
    if(scene())
        scene()->update();
}

void ArrowItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e)
{
    QGraphicsItem::mouseDoubleClickEvent(e);
    //QRectF r = getBubbleRect();
    //if(r.contains(e->pos()))
    {
        mScene->constraintDoubleClicked(this, e);
    }
}

void ArrowItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    QGraphicsItem::mousePressEvent(e);
    QRectF r = getBubbleRect();
    if(r.contains(e->pos()))
    {
        mScene->constraintClicked(this, e);
    }
}

void ArrowItem::hoverMoveEvent(QGraphicsSceneHoverEvent* e)
{
    QGraphicsItem::hoverMoveEvent(e);
    
    if(mType == eEvent)
    {
        double hoverSide = 100;
        QRectF br = boundingRect();
        br.adjust((br.width()-hoverSide)/2,
                  (br.height()-hoverSide)/2,
                  -(br.width()-hoverSide)/2,
                  -(br.height()-hoverSide)/2);
        
        bool shouldShowDelete = br.contains(e->pos());
        if(shouldShowDelete != mShowDelete)
        {
            mShowDelete = shouldShowDelete;
            update();
        }
    }
}
void ArrowItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* e)
{
    QGraphicsItem::hoverLeaveEvent(e);
    
    if(mShowDelete)
    {
        mShowDelete = false;
        update();
    }
}

QRectF ArrowItem::boundingRect() const
{
    double x = qMin(mXStart, mXEnd);
    double y = qMin(mYStart, mYEnd);
    double w = qAbs(mXEnd - mXStart);
    double h = qAbs(mYEnd - mYStart);

    return QRectF(x, y, w, h);
}

QPainterPath ArrowItem::shape() const
{
    QPainterPath path;
    QRectF rect = boundingRect();
    double shift = 15;
    
    if(mXStart < mXEnd && mYStart > mYEnd)
    {
        path.moveTo(mXStart + shift, mYStart);
        path.lineTo(mXStart, mYStart - shift);
        path.lineTo(mXEnd - shift, mYEnd);
        path.lineTo(mXEnd, mYEnd + shift);
    }
    else if(mXStart < mXEnd && mYStart < mYEnd)
    {
        path.moveTo(mXStart + shift, mYStart);
        path.lineTo(mXStart, mYStart + shift);
        path.lineTo(mXEnd - shift, mYEnd);
        path.lineTo(mXEnd, mYEnd - shift);
    }
    else if(mXStart > mXEnd && mYStart < mYEnd)
    {
        path.moveTo(mXStart - shift, mYStart);
        path.lineTo(mXStart, mYStart + shift);
        path.lineTo(mXEnd + shift, mYEnd);
        path.lineTo(mXEnd, mYEnd - shift);
    }
    else if(mXStart > mXEnd && mYStart > mYEnd)
    {
        path.moveTo(mXStart - shift, mYStart);
        path.lineTo(mXStart, mYStart - shift);
        path.lineTo(mXEnd + shift, mYEnd);
        path.lineTo(mXEnd, mYEnd + shift);
    }
    else
    {
        path.addRect(rect);
    }
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
    if(mShowDelete)
        color = Qt::red;
    
    if(mGreyedOut)
        color.setAlphaF(0.05);
    
    painter->setPen(QPen(color, penWidth, mEditing ? Qt::DashLine : Qt::SolidLine));
    painter->drawLine(mXStart, mYStart, mXEnd, mYEnd);
    
    // Bubble
    
    QString bubbleText = getBubbleText();
    bool showMiddleArrow = true;
    
    if(mShowDelete)
    {
        showMiddleArrow = false;
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
        double r = br.width()/3.f;
        painter->drawLine(-r, 0.f, r, 0.f);
        painter->drawLine(0.f, -r, 0.f, r);
        painter->restore();
    }
    else if(!bubbleText.isEmpty())
    {
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
    double posX1 = rect.width()/3;
    double posX2 = 2*rect.width()/3;
    double posY1 = rect.height()/3;
    double posY2 = 2*rect.height()/3;
    
    if(mXStart < mXEnd && mYStart > mYEnd)
    {
        if(showMiddleArrow)
        {
            painter->save();
            painter->translate(rect.x() + posX, rect.y() + posY);
            painter->rotate(angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        }
        else
        {
            painter->save();
            painter->translate(rect.x() + posX1, rect.y() + posY2);
            painter->rotate(angle_deg);
            painter->fillPath(path, color);
            painter->restore();
            
            painter->save();
            painter->translate(rect.x() + posX2, rect.y() + posY1);
            painter->rotate(angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        }
    }
    else if(mXStart < mXEnd && mYStart < mYEnd)
    {
        if(showMiddleArrow)
        {
            painter->save();
            painter->translate(rect.x() + posX, rect.y() + posY);
            painter->rotate(180 - angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        }
        else
        {
            painter->save();
            painter->translate(rect.x() + posX1, rect.y() + posY1);
            painter->rotate(180 - angle_deg);
            painter->fillPath(path, color);
            painter->restore();
            
            painter->save();
            painter->translate(rect.x() + posX2, rect.y() + posY2);
            painter->rotate(180 - angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        }
    }
    else if(mXStart > mXEnd && mYStart < mYEnd)
    {
        if(showMiddleArrow)
        {
            painter->save();
            painter->translate(rect.x() + posX, rect.y() + posY);
            painter->rotate(180 + angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        }
        else
        {
            painter->save();
            painter->translate(rect.x() + posX2, rect.y() + posY1);
            painter->rotate(180 + angle_deg);
            painter->fillPath(path, color);
            painter->restore();
            
            painter->save();
            painter->translate(rect.x() + posX1, rect.y() + posY2);
            painter->rotate(180 + angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        }
    }
    else if(mXStart > mXEnd && mYStart > mYEnd)
    {
        if(showMiddleArrow)
        {
            painter->save();
            painter->translate(rect.x() + rect.width()/2, rect.y() + rect.height()/2);
            painter->rotate(-angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        }
        else
        {
            painter->save();
            painter->translate(rect.x() + 2*rect.width()/3, rect.y() + 2*rect.height()/3);
            painter->rotate(-angle_deg);
            painter->fillPath(path, color);
            painter->restore();
            
            painter->save();
            painter->translate(rect.x() + rect.width()/3, rect.y() + rect.height()/3);
            painter->rotate(-angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        }
    }
    
    //QPainterPath sh = shape();
    //painter->fillPath(sh, Qt::blue);
}

QRectF ArrowItem::getBubbleRect(const QString& text) const
{
    QFont font;
    font.setPointSizeF(11.f);
    QFontMetrics metrics(font);
    int w = metrics.width(text) + 20;
    w = qMax(w, 25);
    int h = 25;
    
    QRectF rect = boundingRect();
    double bubble_x = rect.x() + (rect.width() - w) / 2.f - 0.5f;
    double bubble_y = rect.y() + (rect.height() - h) / 2.f - 0.5f;
    QRectF r(bubble_x, bubble_y, w, h);
    return r;
}

QString ArrowItem::getBubbleText() const
{
    QString bubbleText;
    if(mType == ePhase)
    {
        PhaseConstraint::GammaType gammaType = (PhaseConstraint::GammaType)mData[STATE_CONSTRAINT_GAMMA_TYPE].toInt();
        if(gammaType == PhaseConstraint::eGammaFixed)
            bubbleText = "hiatus ≥ " + QString::number(mData[STATE_CONSTRAINT_GAMMA_FIXED].toDouble());
        else if(gammaType == PhaseConstraint::eGammaRange)
            bubbleText = "min hiatus ∈ [" + QString::number(mData[STATE_CONSTRAINT_GAMMA_MIN].toDouble()) +
            "; " + QString::number(mData[STATE_CONSTRAINT_GAMMA_MAX].toDouble()) + "]";
    }
    return bubbleText;
}

