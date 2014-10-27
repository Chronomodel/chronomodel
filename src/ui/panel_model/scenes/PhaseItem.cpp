#include "PhaseItem.h"
#include "Event.h"
#include "Date.h"
#include "Painting.h"
#include "Project.h"
#include "ProjectManager.h"
#include "QtUtilities.h"
#include <QtWidgets>


PhaseItem::PhaseItem(AbstractScene* scene, const QJsonObject& phase, QGraphicsItem* parent):AbstractItem(scene, parent),
mState(Qt::Unchecked)
{
    mBorderWidth = 10;
    setPhase(phase);
}

PhaseItem::~PhaseItem()
{
    
}

#pragma mark Phase
QJsonObject& PhaseItem::phase()
{
    return mPhase;
}

void PhaseItem::setPhase(const QJsonObject& phase)
{
    mPhase = phase;
    
    setSelected(mPhase[STATE_PHASE_IS_SELECTED].toBool());
    setPos(mPhase[STATE_PHASE_ITEM_X].toDouble(),
           mPhase[STATE_PHASE_ITEM_Y].toDouble());
    
    update();
}

#pragma mark Check state
void PhaseItem::stateChanged(bool checked)
{
    Project* project = ProjectManager::getProject();
    //project->updateEventsPhase(mPhase[STATE_PHASE_ID].toInt(), checked);
}

void PhaseItem::setState(Qt::CheckState state)
{
    mState = state;
    update();
}

#pragma mark Mouse events
void PhaseItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    AbstractItem::mousePressEvent(e);
    if(checkRect().contains(e->pos()))
    {
        if(mState == Qt::PartiallyChecked) mState = Qt::Checked;
        else if(mState == Qt::Checked) mState = Qt::Unchecked;
        else if(mState == Qt::Unchecked) mState = Qt::Checked;
        
        ProjectManager::getProject()->updatePhaseEvents(mPhase[STATE_PHASE_ID].toInt(), mState);
        
        update();
        if(scene())
            scene()->update();
    }
}

void PhaseItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)
{
    mPhase[STATE_PHASE_ITEM_X] = pos().x();
    mPhase[STATE_PHASE_ITEM_Y] = pos().y();
    
    AbstractItem::mouseReleaseEvent(e);
}

void PhaseItem::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
    mPhase[STATE_PHASE_ITEM_X] = pos().x();
    mPhase[STATE_PHASE_ITEM_Y] = pos().y();
    
    AbstractItem::mouseMoveEvent(e);
}


QRectF PhaseItem::boundingRect() const
{
    qreal w = 150;
    qreal h = mTitleHeight + 2*mBorderWidth + 2*mEltsMargin;
    
    QJsonArray events = getEvents();
    if(events.size() > 0)
        h = mTitleHeight + 2*mBorderWidth + mEltsMargin + events.size() * (mEltsHeight + mEltsMargin);
    
    QFont font = qApp->font();
    QString name = mPhase[STATE_PHASE_NAME].toString();
    QFontMetrics metrics(font);
    int nw = metrics.width(name) + 2*mBorderWidth + 3*mEltsMargin + mTitleHeight;
    w = (nw > w) ? nw : w;
    
    font.setPointSizeF(pointSize(11));
    metrics = QFontMetrics(font);
    
    for(int i=0; i<events.size(); ++i)
    {
        QJsonObject event = events[i].toObject();
        name = event[STATE_EVENT_NAME].toString();
        nw = metrics.width(name) + 2*mBorderWidth + 4*mEltsMargin;
        w = (nw > w) ? nw : w;
    }
    return QRectF(-w/2, -h/2, w, h);
}


void PhaseItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    QRectF rect = boundingRect();
    int rounded = 15;
    
    QColor phaseColor = QColor(mPhase[STATE_PHASE_RED].toInt(),
                               mPhase[STATE_PHASE_GREEN].toInt(),
                               mPhase[STATE_PHASE_BLUE].toInt());
    QColor fontColor = getContrastedColor(phaseColor);
    
    painter->setPen(Qt::NoPen);
    painter->setBrush(phaseColor);
    painter->drawRoundedRect(rect, rounded, rounded);
    
    // Checked
    QRectF cRect = checkRect();
    drawCheckBoxBox(*painter, cRect, mState, Qt::white, QColor(150, 150, 150));
    
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
    painter->setPen(fontColor);
    painter->drawText(tr, Qt::AlignCenter, name);
    
    // Type
    QRectF tpr(rect.x() + mBorderWidth + mEltsMargin,
               rect.y() + rect.height() - mBorderWidth - mEltsMargin - mEltsHeight,
               rect.width() - 2*mBorderWidth - 2*mEltsMargin,
               mEltsHeight);
    
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
    
    // Events
    QRectF r(rect.x() + mBorderWidth + mEltsMargin,
             rect.y() + mBorderWidth + mEltsMargin + mTitleHeight + mEltsMargin,
             rect.width() - 2 * (mEltsMargin + mBorderWidth),
             mEltsHeight);
    
    QJsonArray events = getEvents();
    float dy = mEltsMargin + mEltsHeight;
    for(int i=0; i<events.size(); ++i)
    {
        QJsonObject event = events[i].toObject();
        QColor eventColor(event[STATE_EVENT_RED].toInt(),
                          event[STATE_EVENT_GREEN].toInt(),
                          event[STATE_EVENT_BLUE].toInt());
        if(i > 0)
            r.adjust(0, dy, 0, dy);
        
        painter->fillRect(r, eventColor);
        painter->setPen(getContrastedColor(eventColor));
        painter->drawText(r, Qt::AlignCenter, event[STATE_EVENT_NAME].toString());
    }
    
    // Border
    painter->setBrush(Qt::NoBrush);
    if(isSelected())
    {
        painter->setPen(QPen(mainColorDark, 3.f));
        painter->drawRoundedRect(rect.adjusted(1, 1, -1, -1), rounded, rounded);
    }
}

QJsonArray PhaseItem::getEvents() const
{
    QString phaseId = QString::number(mPhase[STATE_PHASE_ID].toInt());
    QJsonObject state = ProjectManager::getProject()->state();
    QJsonArray allEvents = state[STATE_EVENTS].toArray();
    QJsonArray events;
    for(int i=0; i<allEvents.size(); ++i)
    {
        QJsonObject event = allEvents[i].toObject();
        QString phasesIdsStr = event[STATE_EVENT_PHASE_IDS].toString();
        QStringList phasesIds = phasesIdsStr.split(",");
        if(phasesIds.contains(phaseId))
            events.append(event);
    }
    return events;
}

QRectF PhaseItem::checkRect() const
{
    QRectF rect = boundingRect();
    QRectF r(rect.x() + mBorderWidth + mEltsMargin,
             rect.y() + mBorderWidth + mEltsMargin,
             mTitleHeight,
             mTitleHeight);
    return r;
}

