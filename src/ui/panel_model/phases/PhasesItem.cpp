#include "PhasesItem.h"
#include "Event.h"
#include "Date.h"
#include "Painting.h"
#include "Project.h"
#include "ProjectManager.h"
#include <QtWidgets>


PhasesItem::PhasesItem(PhasesScene* phasesView, const QJsonObject& phase, QGraphicsItem* parent):QGraphicsObject(parent),
mPhasesScene(phasesView),
mPhase(phase),
mBorderWidth(1.f),
mTitleHeight(20.f),
mEltsMargin(3.f),
mEltsWidth(15.f),
mEltsHeight(15.f),
mShowElts(true),
mState(Qt::Unchecked)
{
    setZValue(1.);
    setAcceptHoverEvents(true);
    setFlags(QGraphicsItem::ItemIsSelectable |
             QGraphicsItem::ItemIsMovable |
             QGraphicsItem::ItemIsFocusable |
             QGraphicsItem::ItemSendsScenePositionChanges |
             QGraphicsItem::ItemSendsGeometryChanges);
}

PhasesItem::~PhasesItem()
{
    
}

QJsonObject& PhasesItem::phase()
{
    return mPhase;
}

void PhasesItem::setPhase(const QJsonObject& phase)
{
    mPhase = phase;
    
    setSelected(mPhase[STATE_PHASE_IS_SELECTED].toBool());
    setPos(mPhase[STATE_PHASE_ITEM_X].toDouble(),
           mPhase[STATE_PHASE_ITEM_Y].toDouble());
    
    update();
}

void PhasesItem::stateChanged(bool checked)
{
    Project* project = ProjectManager::getProject();
    //project->updateEventsPhase(mPhase[STATE_PHASE_ID].toInt(), checked);
}

void PhasesItem::setState(Qt::CheckState state)
{
    mState = state;
    update();
}

void PhasesItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    setZValue(2.);
    if(toggleRect().contains(e->pos()))
    {
        mShowElts = !mShowElts;
        
        update();
        if(scene())
            scene()->update();
    }
    else if(checkRect().contains(e->pos()))
    {
        if(mState == Qt::PartiallyChecked) mState = Qt::Checked;
        else if(mState == Qt::Checked) mState = Qt::Unchecked;
        else if(mState == Qt::Unchecked) mState = Qt::Checked;
        
        emit stateChanged((mState == Qt::Checked));
        
        update();
        if(scene())
            scene()->update();
    }
    else
    {
        //mPhasesScene->phaseClicked(this, e);
    }
    QGraphicsItem::mousePressEvent(e);
}

void PhasesItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)
{
    setZValue(1.);
    QGraphicsItem::mouseReleaseEvent(e);
}

void PhasesItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e)
{
    //mPhasesScene->phaseDoubleClicked(this, e);
    QGraphicsItem::mouseDoubleClickEvent(e);
}

void PhasesItem::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
    //mPhasesScene->phaseMoved(this, e);
    QGraphicsItem::mouseMoveEvent(e);
}

void PhasesItem::hoverEnterEvent(QGraphicsSceneHoverEvent* e)
{
    //mPhasesScene->phaseEntered(this, e);
    QGraphicsItem::hoverEnterEvent(e);
}

void PhasesItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* e)
{
    //mPhasesScene->phaseLeaved(this, e);
    QGraphicsItem::hoverLeaveEvent(e);
}



QRectF PhasesItem::boundingRect() const
{
    qreal penWidth = 1;
    qreal w = 150;
    
    float h = mTitleHeight + 2*mBorderWidth + 3*mEltsMargin + mEltsHeight;
    if(mShowElts)
    {
        QFont font = qApp->font();
        QFontMetrics metrics(font);
        QString name = mPhase[STATE_PHASE_NAME].toString();
        w = metrics.width(name) + 2*mBorderWidth + 4*mEltsMargin + 2*mTitleHeight;
        
        QString events_ids_str = mPhase[STATE_PHASE_EVENTS_IDS].toString();
        QStringList events_ids = events_ids_str.split(",");
        
        int count = events_ids.size();
        if(count > 0)
            h += count * (mEltsHeight + mEltsMargin);
        else
            h += mEltsMargin + mEltsHeight;
        
        font.setPointSizeF(pointSize(11));
        metrics = QFontMetrics(font);
        
        /*for(int i=0; i<count; ++i)
        {
            name = mPhase->mEvents[i]->mName;
            int nw = metrics.width(name) + 2*mBorderWidth + 4*mEltsMargin;
            w = (nw > w) ? nw : w;
        }
        w = (w < 150) ? 150 : w;*/
    }
    QRectF r(-(w+penWidth)/2, -(h+penWidth)/2, w + penWidth, h + penWidth);
    return r.adjusted(-20, -20, 20, 20);
}


void PhasesItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    QRectF r = boundingRect();
    QRectF rect = insideRect();
    
    QPainterPath rounded;
    rounded.addRoundedRect(rect, 5, 5);
    
    QColor phaseColor = QColor(mPhase[STATE_PHASE_RED].toInt(),
                               mPhase[STATE_PHASE_GREEN].toInt(),
                               mPhase[STATE_PHASE_BLUE].toInt());
    QColor phaseColorLight = phaseColor;
    phaseColorLight.setAlpha(100);
    
    painter->setPen(phaseColor);
    painter->setBrush(isSelected() ? QColor(100, 100, 100) : phaseColor);
    painter->drawEllipse(r);
    
    painter->fillPath(rounded, Qt::white);
    painter->fillPath(rounded, phaseColorLight);
    if(isSelected())
        painter->fillPath(rounded, QColor(0, 0, 0, 150));

    
    // Border
    
    /*painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(phaseColor, mBorderWidth));
    painter->drawPath(rounded);*/
    
    
    // Name
    
    QRectF tr(rect.x() + mBorderWidth + 2*mEltsMargin + mTitleHeight,
              rect.y() + mBorderWidth + mEltsMargin,
              rect.width() - 2*mBorderWidth - 2*(mTitleHeight + 2*mEltsMargin),
              mTitleHeight);
    
    QFont font = qApp->font();
    painter->setFont(font);
    QFontMetrics metrics(font);
    QString name = mPhase[STATE_PHASE_NAME].toString();
    name = metrics.elidedText(name, Qt::ElideRight, tr.width());
    
    painter->setPen(isSelected() ? Qt::white : Qt::black);
    painter->drawText(tr, Qt::AlignCenter, name/* + " (" + QString::number(mEvent->mId) + ")"*/);
    
    
    // Type
    
    QRectF tpr(rect.x() + mBorderWidth + mEltsMargin,
              rect.y() + rect.height() - mBorderWidth - mEltsMargin - mEltsHeight,
              rect.width() - 2*mBorderWidth - 2*mEltsMargin,
              mEltsHeight);
    
    painter->setPen(isSelected() ? Qt::white : Qt::black);
    
    Phase::TauType tauType = (Phase::TauType)mPhase[STATE_PHASE_TAU_TYPE].toInt();
    
    if(tauType == Phase::eTauRange)
    {
        float tauMin = mPhase[STATE_PHASE_TAU_MIN].toDouble();
        float tauMax = mPhase[STATE_PHASE_TAU_MAX].toDouble();
        
        QString tau = QString::number(tauMin) +
            " < " + QObject::tr("Duration") + " < " +
            QString::number(tauMax);
        
        font.setPointSizeF(pointSize(11));
        painter->setFont(font);
        painter->drawText(tpr, Qt::AlignCenter, tau);
    }
    
    
    // Color
    
    /*QRectF colRect = colorRect();
    painter->setPen(phaseColor);
    painter->setBrush(phaseColor);
    painter->drawRect(colRect);*/
    
    // Checked
    
    QRectF cRect = checkRect();
    drawCheckBoxBox(*painter, cRect, mState, Qt::white, QColor(150, 150, 150));
    
    // Toggle
    
    QRectF tRect = toggleRect();
    painter->setPen(Qt::NoPen);
    painter->setBrush(isSelected() ? QColor(255, 255, 255, 30) : QColor(0, 0, 0, 30));
    painter->drawEllipse(tRect);
    
    painter->setPen(isSelected() ? Qt::white : Qt::black);
    float mg = 4;
    painter->drawLine(tRect.x() + mg, tRect.y() + tRect.height()/2, tRect.x() + tRect.width() - mg, tRect.y() + tRect.height()/2);
    if(!mShowElts)
        painter->drawLine(tRect.x() + tRect.width()/2, tRect.y() + mg, tRect.x() + tRect.width()/2, tRect.y() + tRect.height() - mg);
    
    // Events
    
    if(mShowElts)
    {
        QFont font = qApp->font();
        font.setPointSizeF(pointSize(11));
        painter->setFont(font);
        
        painter->setPen(isSelected() ? Qt::white : Qt::black);
        
        QRectF r(rect.x() + mBorderWidth + mEltsMargin,
                 rect.y() + mBorderWidth + mEltsMargin + mTitleHeight + mEltsMargin,
                 rect.width() - 2 * (mEltsMargin + mBorderWidth),
                 mEltsHeight);
        
        QString events_ids_str = mPhase[STATE_PHASE_EVENTS_IDS].toString();
        QStringList events_ids = events_ids_str.split(",");
        
        int count = events_ids.size();
        if(count == 0)
        {
            painter->fillRect(r, QColor(0, 0, 0, 30));
            painter->drawText(r, Qt::AlignCenter, QObject::tr("No Event !"));
        }
        else
        {
            float dy = mEltsMargin + mEltsHeight;
            /*for(int i=0; i<count; ++i)
            {
                if(i > 0)
                    r.adjust(0, dy, 0, dy);
                
                painter->fillRect(r, QColor(0, 0, 0, 30));
                
                QColor eventColor = mPhase->mEvents[i]->mColor;
                painter->fillRect(r.adjusted(2, 2, -r.width() + r.height() - 2, -2), eventColor);
                
                painter->drawText(r.adjusted(r.height() + 2, 0, 0, 0), Qt::AlignCenter, mPhase->mEvents[i]->mName);
            }*/
        }
    }
}

QRectF PhasesItem::toggleRect() const
{
    QRectF rect = insideRect();
    QRectF r(rect.x() + rect.width() - mBorderWidth - mEltsMargin - mTitleHeight,
             rect.y() + mBorderWidth + mEltsMargin,
             mTitleHeight,
             mTitleHeight);
    return r;
}

QRectF PhasesItem::checkRect() const
{
    QRectF rect = insideRect();
    QRectF r(rect.x() + mBorderWidth + mEltsMargin,
             rect.y() + mBorderWidth + mEltsMargin,
             mTitleHeight,
             mTitleHeight);
    return r;
}

QRectF PhasesItem::colorRect() const
{
    QRectF rect = insideRect();
    QRectF r(rect.x() + mBorderWidth + 2*mEltsMargin + mTitleHeight,
             rect.y() + mBorderWidth + mEltsMargin,
             mTitleHeight,
             mTitleHeight);
    return r;
}

QRectF PhasesItem::insideRect() const
{
    QRectF rect = boundingRect();
    return rect.adjusted(20, 20, -20, -20);
}

