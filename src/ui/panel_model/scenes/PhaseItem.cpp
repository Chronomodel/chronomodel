#include "PhaseItem.h"
#include "PhasesScene.h"
#include "Event.h"
#include "Date.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "MainWindow.h"
#include "Project.h"
#include <QtWidgets>


PhaseItem::PhaseItem(AbstractScene* scene, const QJsonObject& phase, QGraphicsItem* parent):AbstractItem(scene, parent),
mState(Qt::Unchecked),
mEyeActivated(false),
mControlsVisible(true)
{
    mBorderWidth = 10;
    mEltsHeight = 15;
    setPhase(phase);
}

PhaseItem::~PhaseItem()
{
    
}

#pragma mark Phase
QJsonObject& PhaseItem::getPhase()
{
    return mData;
}

void PhaseItem::setPhase(const QJsonObject& phase)
{
    mData = phase;
    
    setSelected(mData[STATE_IS_SELECTED].toBool());
    setPos(mData[STATE_ITEM_X].toDouble(),
           mData[STATE_ITEM_Y].toDouble());
    
    update();
}

void PhaseItem::setState(Qt::CheckState state)
{
    mState = state;
    update();
}

void PhaseItem::setControlsVisible(double visible)
{
    mControlsVisible = visible;
    update();
}

#pragma mark Mouse events
void PhaseItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    if(mControlsVisible && checkRect().contains(e->pos()))
    {
        //qDebug() << "-> Check clicked";
        
        // Do not select phase when clicking on the box
        e->accept();
        
        if(mState == Qt::PartiallyChecked) mState = Qt::Checked;
        else if(mState == Qt::Checked) mState = Qt::Unchecked;
        else if(mState == Qt::Unchecked) mState = Qt::Checked;
        
        MainWindow::getInstance()->getProject()->updatePhaseEvents(mData[STATE_ID].toInt(), mState);
        
        update();
        if(scene())
            scene()->update();
    }
    else if(mControlsVisible && eyeRect().contains(e->pos()))
    {
        //qDebug() << "-> Eye clicked";
        
        // Do not select phase when clicking on the box
        e->accept();
        
        mEyeActivated = !mEyeActivated;
        ((PhasesScene*)mScene)->updateEyedPhases();
        
        update();
        if(scene())
            scene()->update();
    }
    else
    {
        AbstractItem::mousePressEvent(e);
    }
}

void PhaseItem::updateItemPosition(const QPointF& pos)
{
    mData[STATE_ITEM_X] = pos.x();
    mData[STATE_ITEM_Y] = pos.y();
}

QRectF PhaseItem::boundingRect() const
{
    qreal w = 150;
    qreal h = mTitleHeight + 2*mBorderWidth + 2*mEltsMargin;
    
    QJsonArray events = getEvents();
    if(events.size() > 0)
        h += events.size() * (mEltsHeight + mEltsMargin) - mEltsMargin;
    
    QString tauStr = getTauString();
    if(!tauStr.isEmpty())
        h += mEltsMargin + mEltsHeight;
    
    QFont font = qApp->font();
    QString name = mData[STATE_NAME].toString();
    QFontMetrics metrics(font);
    int nw = metrics.width(name) + 2*mBorderWidth + 4*mEltsMargin + 2*mTitleHeight;
    w = (nw > w) ? nw : w;
    
    font.setPointSizeF(pointSize(11.f));
    metrics = QFontMetrics(font);
    
    nw = metrics.width(tauStr) + 2*mBorderWidth + 4*mEltsMargin;
    w = (nw > w) ? nw : w;
    
    for(int i=0; i<events.size(); ++i)
    {
        QJsonObject event = events[i].toObject();
        name = event[STATE_NAME].toString();
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
    
    QColor phaseColor = QColor(mData[STATE_COLOR_RED].toInt(),
                               mData[STATE_COLOR_GREEN].toInt(),
                               mData[STATE_COLOR_BLUE].toInt());
    QColor fontColor = getContrastedColor(phaseColor);
    
    painter->setPen(Qt::NoPen);
    painter->setBrush(phaseColor);
    painter->drawRoundedRect(rect, rounded, rounded);
    
    if(mControlsVisible)
    {
        // Checked
        QRectF cRect = checkRect();
        drawCheckBoxBox(*painter, cRect, mState, Qt::white, QColor(150, 150, 150));
        
        // Eye
        QRectF eRect = eyeRect();
        painter->setBrush(Qt::white);
        painter->setPen(Qt::black);
        painter->drawRoundedRect(eRect, 10, 10);
        if(mEyeActivated)
        {
            QPixmap eye(":eye.png");
            painter->drawPixmap(eRect, eye, eye.rect());
        }
    }
    
    // Name
    QRectF tr(rect.x() + mBorderWidth + 2*mEltsMargin + mTitleHeight,
              rect.y() + mBorderWidth + mEltsMargin,
              rect.width() - 2*mBorderWidth - 2*(mTitleHeight + 2*mEltsMargin),
              mTitleHeight);
    
    QFont font = qApp->font();
    painter->setFont(font);
    QFontMetrics metrics(font);
    QString name = mData[STATE_NAME].toString();
    name = metrics.elidedText(name, Qt::ElideRight, tr.width());
    painter->setPen(fontColor);
    painter->drawText(tr, Qt::AlignCenter, name);
    
    // Change font
    font.setPointSizeF(pointSize(11.f));
    painter->setFont(font);
    
    // Type (duration tau)
    QString tauStr = getTauString();
    if(!tauStr.isEmpty())
    {
        QRectF tpr(rect.x() + mBorderWidth + mEltsMargin,
                   rect.y() + rect.height() - mBorderWidth - mEltsMargin - mEltsHeight,
                   rect.width() - 2*mBorderWidth - 2*mEltsMargin,
                   mEltsHeight);
        
        painter->setPen(Qt::black);
        painter->setBrush(Qt::white);
        painter->drawRect(tpr);
        painter->drawText(tpr, Qt::AlignCenter, tauStr);
    }
    
    // Events
    QRectF r(rect.x() + mBorderWidth + mEltsMargin,
             rect.y() + mBorderWidth + mEltsMargin + mTitleHeight + mEltsMargin,
             rect.width() - 2 * (mEltsMargin + mBorderWidth),
             mEltsHeight);
    
    QJsonArray events = getEvents();
    double dy = mEltsMargin + mEltsHeight;
    for(int i=0; i<events.size(); ++i)
    {
        QJsonObject event = events[i].toObject();
        QColor eventColor(event[STATE_COLOR_RED].toInt(),
                          event[STATE_COLOR_GREEN].toInt(),
                          event[STATE_COLOR_BLUE].toInt());
        if(i > 0)
            r.adjust(0, dy, 0, dy);
        
        painter->fillRect(r, eventColor);
        painter->setPen(getContrastedColor(eventColor));
        painter->drawText(r, Qt::AlignCenter, event[STATE_NAME].toString());
    }
    
    // Border
    painter->setBrush(Qt::NoBrush);
    if(isSelected())
    {
        painter->setPen(QPen(Qt::white, 5.f));
        painter->drawRoundedRect(rect.adjusted(1, 1, -1, -1), rounded, rounded);
        
        painter->setPen(QPen(Qt::red, 3.f));
        painter->drawRoundedRect(rect.adjusted(1, 1, -1, -1), rounded, rounded);
    }
}

QJsonArray PhaseItem::getEvents() const
{
    QString phaseId = QString::number(mData[STATE_ID].toInt());
    QJsonObject state = MainWindow::getInstance()->getProject()->state();
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

QString PhaseItem::getTauString() const
{
    QString tauStr;
    Phase::TauType type = (Phase::TauType)mData[STATE_PHASE_TAU_TYPE].toInt();
    if(type == Phase::eTauFixed)
    {
        tauStr += tr("duration") + " ≤ " + QString::number(mData[STATE_PHASE_TAU_FIXED].toDouble());
    }
    else if(type == Phase::eTauRange)
    {
        tauStr += tr("max duration") + " ∈ [" + QString::number(mData[STATE_PHASE_TAU_MIN].toDouble()) + "; " + QString::number(mData[STATE_PHASE_TAU_MAX].toDouble()) + "]";
    }
    return tauStr;
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

QRectF PhaseItem::eyeRect() const
{
    QRectF rect = boundingRect();
    QRectF r(rect.x() + rect.width() - mBorderWidth - mEltsMargin - mTitleHeight,
             rect.y() + mBorderWidth + mEltsMargin,
             mTitleHeight,
             mTitleHeight);
    return r;
}
