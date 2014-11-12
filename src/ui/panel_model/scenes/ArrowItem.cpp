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
mEditing(false)
{
    setZValue(-1.);
    setAcceptHoverEvents(true);
    setFlags(QGraphicsItem::ItemIsSelectable |
            QGraphicsItem::ItemIsFocusable |
            QGraphicsItem::ItemSendsScenePositionChanges |
            QGraphicsItem::ItemSendsGeometryChanges);
    
    setConstraint(constraint);
}

QJsonObject& ArrowItem::constraint()
{
    return mConstraint;
}

void ArrowItem::setConstraint(const QJsonObject& c)
{
    mConstraint = c;
    updatePosition();
}

void ArrowItem::updatePosition()
{
    Project* project = MainWindow::getInstance()->getProject();
    QJsonObject state = project->state();
    
    if(mType == eEvent)
    {
        int fromId = mConstraint[STATE_EVENT_CONSTRAINT_BWD_ID].toInt();
        int toId = mConstraint[STATE_EVENT_CONSTRAINT_FWD_ID].toInt();
        
        QJsonArray events = state[STATE_EVENTS].toArray();
        
        QJsonObject from;
        QJsonObject to;
        
        for(int i=0; i<events.size(); ++i)
        {
            QJsonObject event = events[i].toObject();
            if(event[STATE_EVENT_ID].toInt() == fromId)
                from = event;
            if(event[STATE_EVENT_ID].toInt() == toId)
                to = event;
        }
        
        mXStart = from[STATE_EVENT_ITEM_X].toDouble();
        mYStart = from[STATE_EVENT_ITEM_Y].toDouble();
        
        mXEnd = to[STATE_EVENT_ITEM_X].toDouble();
        mYEnd = to[STATE_EVENT_ITEM_Y].toDouble();
    }
    else
    {
        int fromId = mConstraint[STATE_PHASE_CONSTRAINT_BWD_ID].toInt();
        int toId = mConstraint[STATE_PHASE_CONSTRAINT_FWD_ID].toInt();
        
        QJsonArray phases = state[STATE_PHASES].toArray();
        
        QJsonObject from;
        QJsonObject to;
        
        for(int i=0; i<phases.size(); ++i)
        {
            QJsonObject phase = phases[i].toObject();
            if(phase[STATE_PHASE_ID].toInt() == fromId)
                from = phase;
            if(phase[STATE_PHASE_ID].toInt() == toId)
                to = phase;
        }
        
        mXStart = from[STATE_PHASE_ITEM_X].toDouble();
        mYStart = from[STATE_PHASE_ITEM_Y].toDouble();
        
        mXEnd = to[STATE_PHASE_ITEM_X].toDouble();
        mYEnd = to[STATE_PHASE_ITEM_Y].toDouble();
    }

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
        mEditing = true;
        update();
        
        mScene->constraintDoubleClicked(this, e);
        
        mEditing = false;
        
        // TODO : cannot refresh if the constraint has been deleted !
        //update();
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

void ArrowItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    painter->setRenderHint(QPainter::Antialiasing);
    QRectF rect = boundingRect();
    
    //painter->fillRect(rect, QColor(0, 255, 0, 30));
    
    int penWidth = 1;
    QColor color = mEditing ? QColor(77, 180, 62) : QColor(0, 0, 0);
    //if(isUnderMouse())
     //   color = QColor(30, 50, 150);
    
    painter->setPen(QPen(color, penWidth, mEditing ? Qt::DashLine : Qt::SolidLine));
    painter->drawLine(mXStart, mYStart, mXEnd, mYEnd);
    
    // arrows
    
    float angle_rad = atanf(rect.width() / rect.height());
    float angle_deg = angle_rad * 180. / M_PI;
    
    QPainterPath path;
    int arrow_w = 10;
    int arrow_l = 15;
    path.moveTo(-arrow_w/2, arrow_l/2);
    path.lineTo(arrow_w/2, arrow_l/2);
    path.lineTo(0, -arrow_l/2);
    path.closeSubpath();
    
    float posX = rect.width()/2;
    float posY = rect.height()/2;
    float posX1 = rect.width()/3;
    float posX2 = 2*rect.width()/3;
    float posY1 = rect.height()/3;
    float posY2 = 2*rect.height()/3;
    
    //EventConstraint::PhiType phiType = (EventConstraint::PhiType)mConstraint[STATE_EVENT_CONSTRAINT_PHI_TYPE].toInt();
    
    if(mXStart < mXEnd && mYStart > mYEnd)
    {
        //if(phiType == EventConstraint::ePhiUnknown)
        {
            painter->save();
            painter->translate(rect.x() + posX, rect.y() + posY);
            painter->rotate(angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        }
        /*else
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
        }*/
    }
    else if(mXStart < mXEnd && mYStart < mYEnd)
    {
        //if(phiType == EventConstraint::ePhiUnknown)
        {
            painter->save();
            painter->translate(rect.x() + posX, rect.y() + posY);
            painter->rotate(180 - angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        }
        /*else
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
        }*/
    }
    else if(mXStart > mXEnd && mYStart < mYEnd)
    {
        //if(phiType == EventConstraint::ePhiUnknown)
        {
            painter->save();
            painter->translate(rect.x() + posX, rect.y() + posY);
            painter->rotate(180 + angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        }
        /*else
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
        }*/
    }
    else if(mXStart > mXEnd && mYStart > mYEnd)
    {
        //if(phiType == EventConstraint::ePhiUnknown)
        {
            painter->save();
            painter->translate(rect.x() + rect.width()/2, rect.y() + rect.height()/2);
            painter->rotate(-angle_deg);
            painter->fillPath(path, color);
            painter->restore();
        }
        /*else
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
        }*/
    }
    
    // Bubble
    
    /*switch(phiType)
    {
        case EventConstraint::ePhiUnknown:
        {
            break;
        }
        case EventConstraint::ePhiFixed:
        {
            break;
        }
        case EventConstraint::ePhiRange:
        {
            QFont font = painter->font();
            font.setPointSizeF(pointSize(11));
            painter->setFont(font);
            QRectF r = getBubbleRect();
            
            painter->setPen(color);
            painter->setBrush(Qt::white);
            painter->drawRoundedRect(r, 5, 5);
            
            float phiMin = mConstraint[STATE_EVENT_CONSTRAINT_PHI_MIN].toDouble();
            float phiMax = mConstraint[STATE_EVENT_CONSTRAINT_PHI_MAX].toDouble();
            
            //painter->setPen(QColor(120, 120, 120));
            painter->drawText(r.adjusted(0, 0, 0, -r.height()/2), Qt::AlignCenter, QString::number(phiMin));
            painter->drawText(r.adjusted(0, r.height()/2, 0, 0), Qt::AlignCenter, QString::number(phiMax));
            painter->drawLine(r.x() + 4, r.y() + r.height()/2, r.x() + r.width() - 4, r.y() + r.height()/2);
            break;
        }
        default:
            break;
    }*/
}

QRectF ArrowItem::getBubbleRect() const
{
    int w = 60;
    int h = 30;
    
    /*switch(mConstraint->mPhiType)
    {
        case EventConstraint::ePhiUnknown:
        {
            w = 30;
            h = 30;
            break;
        }
        case EventConstraint::ePhiFixed:
        case EventConstraint::ePhiRange:
        {
            w = 70;
            h = 30;
            break;
        }
        default:
            break;
    }*/
    
    QRectF rect = boundingRect();
    float bubble_x = rect.x() + (rect.width() - w) / 2.f - 0.5f;
    float bubble_y = rect.y() + (rect.height() - h) / 2.f - 0.5f;
    QRectF r(bubble_x, bubble_y, w, h);
    return r;
}



