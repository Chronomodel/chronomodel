#include "PhasesSceneArrowItem.h"
#include "PhasesItem.h"
#include "PhaseConstraint.h"
#include "ProjectManager.h"
#include "Painting.h"
#include <QtWidgets>
#include <math.h>


PhasesSceneArrowItem::PhasesSceneArrowItem(PhasesScene* phasesView, PhaseConstraint* constraint, QGraphicsItem* parent):QGraphicsItem(parent),
mConstraint(constraint),
mItemFrom(0),
mItemTo(0),
mPhasesScene(phasesView),
mXStart(0),
mYStart(0),
mXEnd(0),
mYEnd(0),
mBubbleWidth(130.f),
mBubbleHeight(40.f),
mDeleteWidth(15.f),
mEditing(false)
{
    setZValue(-1.);
    setFlags(QGraphicsItem::ItemIsSelectable |
            QGraphicsItem::ItemIsFocusable |
            QGraphicsItem::ItemSendsScenePositionChanges |
            QGraphicsItem::ItemSendsGeometryChanges);
}

void PhasesSceneArrowItem::setItemFrom(PhasesItem* itemFrom)
{
    mItemFrom = itemFrom;
    updatePosition();
}

void PhasesSceneArrowItem::setItemTo(PhasesItem* itemTo)
{
    mItemTo = itemTo;
    updatePosition();
}

void PhasesSceneArrowItem::updatePosition()
{
    mXStart = mItemFrom ? mItemFrom->pos().x() : 0;
    mYStart = mItemFrom ? mItemFrom->pos().y() : 0;
    
    mXEnd = mItemTo ? mItemTo->pos().x() : 0;
    mYEnd = mItemTo ? mItemTo->pos().y() : 0;
    
    if(scene())
    {
        scene()->update();
    }
}

void PhasesSceneArrowItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e)
{
    QGraphicsItem::mouseDoubleClickEvent(e);
    //QRectF r = getBubbleRect();
    //if(r.contains(e->pos()))
    {
        mEditing = true;
        update();
        
        //mPhasesScene->constraintDoubleClicked(this, e);
        
        mEditing = false;
        
        // TODO : cannot refresh if the constraint has been deleted !
        //update();
    }
}

QRectF PhasesSceneArrowItem::boundingRect() const
{
    double x = qMin(mXStart, mXEnd);
    double y = qMin(mYStart, mYEnd);
    double w = qAbs(mXEnd - mXStart);
    double h = qAbs(mYEnd - mYStart);

    return QRectF(x, y, w, h);
}

void PhasesSceneArrowItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
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
    
    if(mXStart < mXEnd && mYStart > mYEnd)
    {
        if(mConstraint->mGammaType == PhaseConstraint::eGammaUnknown)
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
        if(mConstraint->mGammaType == PhaseConstraint::eGammaUnknown)
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
        if(mConstraint->mGammaType == PhaseConstraint::eGammaUnknown)
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
        if(mConstraint->mGammaType == PhaseConstraint::eGammaUnknown)
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
    
    // Bubble
    
    switch(mConstraint->mGammaType)
    {
        case PhaseConstraint::eGammaUnknown:
        {
            break;
        }
        case PhaseConstraint::eGammaFixed:
        {
            break;
        }
        case PhaseConstraint::eGammaRange:
        {
            QFont font = painter->font();
            font.setPointSizeF(pointSize(11));
            painter->setFont(font);
            QRectF r = getBubbleRect();
            
            painter->setPen(color);
            painter->setBrush(Qt::white);
            painter->drawRoundedRect(r, 5, 5);
            
            //painter->setPen(QColor(120, 120, 120));
            painter->drawText(r.adjusted(0, 0, 0, -r.height()/2), Qt::AlignCenter, QString::number(mConstraint->mGammaMin));
            painter->drawText(r.adjusted(0, r.height()/2, 0, 0), Qt::AlignCenter, QString::number(mConstraint->mGammaMax));
            painter->drawLine(r.x() + 4, r.y() + r.height()/2, r.x() + r.width() - 4, r.y() + r.height()/2);
            break;
        }
        default:
            break;
    }
}

QRectF PhasesSceneArrowItem::getBubbleRect() const
{
    int w = 60;
    int h = 30;
    
    QRectF rect = boundingRect();
    float bubble_x = rect.x() + (rect.width() - w) / 2.f - 0.5f;
    float bubble_y = rect.y() + (rect.height() - h) / 2.f - 0.5f;
    QRectF r(bubble_x, bubble_y, w, h);
    return r;
}



