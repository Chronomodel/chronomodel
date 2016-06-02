#include "PhaseItem.h"
#include "PhasesScene.h"
#include "Event.h"
#include "Date.h"
#include "Painting.h"
#include "QtUtilities.h"
#include "MainWindow.h"
#include "Project.h"
#include "ArrowTmpItem.h"
#include <QtWidgets>


PhaseItem::PhaseItem(AbstractScene* scene, const QJsonObject& phase, QGraphicsItem* parent):AbstractItem(scene, parent),
mState(Qt::Unchecked),
mEyeActivated(false),
mControlsVisible(false),
mControlsEnabled(false)
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
    
    setSelected(mData.value(STATE_IS_SELECTED).toBool());
    setPos(mData.value(STATE_ITEM_X).toDouble(),
           mData.value(STATE_ITEM_Y).toDouble());
    
    // ----------------------------------------------------
    //  Calculate item size
    // ----------------------------------------------------
    qreal w = 150;
    qreal h = mTitleHeight + 2*mBorderWidth + 2*mEltsMargin;
    
    const QJsonArray events = getEvents();
    if (events.size() > 0)
        h += events.size() * (mEltsHeight + mEltsMargin) - mEltsMargin;
    
    const QString tauStr = getTauString();
    if (!tauStr.isEmpty())
        h += mEltsMargin + mEltsHeight;
    
    QFont font = qApp->font();
    const QString name = mData.value(STATE_NAME).toString();
    QFontMetrics metrics(font);

   // Width of the phase box Name Width
    int nw = metrics.width(name) + 2*mBorderWidth + 2*mEltsMargin ;
    w = (nw > w) ? nw : w;
    
    font.setPointSizeF(pointSize(11.f));
    metrics = QFontMetrics(font);
    
    nw = metrics.width(tauStr) + 2*mBorderWidth + 4*mEltsMargin;
    w = (nw > w) ? nw : w;
    
    for (int i=0; i<events.size(); ++i) {
        const QJsonObject event = events.at(i).toObject();
        const QString eventName = event.value(STATE_NAME).toString();
        nw = metrics.width(eventName) + 2*mBorderWidth + 4*mEltsMargin;
        w = (nw > w) ? nw : w;
    }

    
    mSize = QSize(w, h);
    
    update();
}

void PhaseItem::setState(Qt::CheckState state)
{
    mState = state;
    //update();
}

void PhaseItem::setControlsEnabled(const bool enabled)
{
    mControlsEnabled = enabled;
   // update();
}

void PhaseItem::setControlsVisible(const bool visible)
{
    mControlsVisible = visible;
    //update();
}

#pragma mark Mouse events

void PhaseItem::hoverEnterEvent(QGraphicsSceneHoverEvent* e)
{
    setControlsEnabled(true);
    AbstractItem::hoverEnterEvent(e);
}

void PhaseItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* e)
{
    setControlsEnabled(false);
    AbstractItem::hoverLeaveEvent(e);
}

void PhaseItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    if (mControlsVisible && checkRect().contains(e->pos())) {
        //qDebug() << "-> Check clicked";
        
        // Do not select phase when clicking on the box
        e->accept();
        
        if (mState == Qt::PartiallyChecked)
            mState = Qt::Checked;
        else if (mState == Qt::Checked)
            mState = Qt::Unchecked;
        else if (mState == Qt::Unchecked)
            mState = Qt::Checked;
        
        MainWindow::getInstance()->getProject()->updatePhaseEvents(mData.value(STATE_ID).toInt(), mState);
        

    } else if (mControlsVisible && eyeRect().contains(e->pos())) {
        //qDebug() << "-> Eye clicked";
        
        // Do not select phase when clicking on the box
        e->accept();
        
        mEyeActivated = !mEyeActivated;
        ((PhasesScene*)mScene)->updateEyedPhases();
        
        update();
        if (scene())
            scene()->update();
    } else
       // AbstractItem::mousePressEvent(e);
    {
        PhasesScene* itemScene = dynamic_cast<PhasesScene*>(mScene);
        PhaseItem* currentPhase = itemScene->currentPhase();


        if (itemScene->selectedItems().size()<2) {
            if ((this != currentPhase) && (!itemScene->mDrawingArrow)) {
                itemScene->clearSelection();
            }

            if (!itemScene->itemClicked(this, e)) {
                setZValue(2.);
                QGraphicsItem::mousePressEvent(e);
            } else
                itemScene->mTempArrow->setFrom(pos().x(), pos().y());
        }

        QGraphicsObject::mousePressEvent(e);

    }

}

void PhaseItem::updateItemPosition(const QPointF& pos)
{
    mData[STATE_ITEM_X] = pos.x();
    mData[STATE_ITEM_Y] = pos.y();
}

QRectF PhaseItem::boundingRect() const
{
    return QRectF(-mSize.width()/2, -mSize.height()/2, mSize.width(), mSize.height());
}


void PhaseItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    QRectF rect = boundingRect();
    int rounded = 15;
    
    const QColor phaseColor = QColor(mData.value(STATE_COLOR_RED).toInt(),
                               mData.value(STATE_COLOR_GREEN).toInt(),
                               mData.value(STATE_COLOR_BLUE).toInt());
    const QColor fontColor = getContrastedColor(phaseColor);
    QFont font = qApp->font();

    painter->setPen(Qt::NoPen);
    painter->setBrush(phaseColor);
    painter->drawRoundedRect(rect, rounded, rounded);
    
    if (mControlsVisible && mControlsEnabled) {
        // Checked box
        const QRectF cRect = checkRect();
        drawCheckBoxBox(*painter, cRect, mState, Qt::white, QColor(150, 150, 150));
        
        // Eye
        const QRectF eRect = eyeRect();
        painter->drawRect(eRect);
        if (mEyeActivated) {
            QLinearGradient grad(0, 0, 0, eRect.height());
            grad.setColorAt(0, Painting::mainColorLight);
            grad.setColorAt(1,Painting::mainColorDark);

            painter->fillRect(eRect,grad);
        }
        else
            painter->fillRect(eRect, Qt::black);

        const QPixmap eye(":eye_w.png");
        painter->drawPixmap(eRect, eye, eye.rect());


    } else {
        // Name
        const QRectF tr(rect.x() + mBorderWidth ,
                  rect.y() + mBorderWidth + mEltsMargin,
                  rect.width() - 2*mBorderWidth ,
                  mTitleHeight);

        painter->setFont(font);
        QFontMetrics metrics(font);
        QString name = mData.value(STATE_NAME).toString();
        name = metrics.elidedText(name, Qt::ElideRight, tr.width());
        painter->setPen(fontColor);
        painter->drawText(tr, Qt::AlignCenter, name);
    }

    // Change font
    font.setPointSizeF(pointSize(11.f));
    painter->setFont(font);
    
    // Type (duration tau)
    const QString tauStr = getTauString();
    if (!tauStr.isEmpty()) {
        const QRectF tpr(rect.x() + mBorderWidth + mEltsMargin,
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
    
    const QJsonArray events = getEvents();
    double dy = mEltsMargin + mEltsHeight;
    PhasesScene* itemScene = dynamic_cast<PhasesScene*>(mScene);

    const bool showAlldata = itemScene->showAllEvents();
    for (int i=0; i<events.size(); ++i) {

        const QJsonObject event = events.at(i).toObject();
        const QColor eventColor(event.value(STATE_COLOR_RED).toInt(),
                          event.value(STATE_COLOR_GREEN).toInt(),
                          event.value(STATE_COLOR_BLUE).toInt());
        const bool isSelected = ( event.value(STATE_IS_SELECTED).toBool() || event.value(STATE_IS_CURRENT).toBool() );

        if (i > 0)
            r.adjust(0, dy, 0, dy);

        painter->setPen(getContrastedColor(eventColor));

        // magnify and highlight selected events
        if (isSelected) {
            painter->setPen(QPen(fontColor, 3.f));
            r.adjust(1, 1, -1, -1);
            painter->drawRoundedRect(r,1,1);
            painter->fillRect(r, eventColor);
            painter->setPen(getContrastedColor(eventColor));
            painter->drawText(r, Qt::AlignCenter, event.value(STATE_NAME).toString());
            r.adjust(-1, -1, +1, +1);
        }
        else if (isSelected || showAlldata) {
                painter->fillRect(r, eventColor);
                painter->drawText(r, Qt::AlignCenter, event.value(STATE_NAME).toString());

        } else {
            painter->setOpacity(0.2);
            painter->fillRect(r, eventColor);
            painter->drawText(r, Qt::AlignCenter, event.value(STATE_NAME).toString());
            painter->setOpacity(1);
        }

    }
    
    // Border
    painter->setBrush(Qt::NoBrush);
    if (isSelected()) {
        painter->setPen(QPen(Qt::white, 5.f));
        painter->drawRoundedRect(rect.adjusted(1, 1, -1, -1), rounded, rounded);
        
        painter->setPen(QPen(Qt::red, 3.f));
        painter->drawRoundedRect(rect.adjusted(1, 1, -1, -1), rounded, rounded);
    }
}

QJsonArray PhaseItem::getEvents() const
{
    QString phaseId = QString::number(mData.value(STATE_ID).toInt());
    QJsonObject state = MainWindow::getInstance()->getProject()->state();
    QJsonArray allEvents = state.value(STATE_EVENTS).toArray();
    QJsonArray events;
    for (int i=0; i<allEvents.size(); ++i) {
        QJsonObject event = allEvents.at(i).toObject();
        QString phasesIdsStr = event.value(STATE_EVENT_PHASE_IDS).toString();
        QStringList phasesIds = phasesIdsStr.split(",");
        if (phasesIds.contains(phaseId))
            events.append(event);
    }
    return events;
}

QString PhaseItem::getTauString() const
{
    QString tauStr;
    Phase::TauType type = (Phase::TauType)mData.value(STATE_PHASE_TAU_TYPE).toInt();
    if (type == Phase::eTauFixed)
        tauStr += tr("duration") + " ≤ " + QString::number(mData.value(STATE_PHASE_TAU_FIXED).toDouble());

    /*else if(type == Phase::eTauRange)
    {
        tauStr += tr("max duration") + " ∈ [" + QString::number(mData[STATE_PHASE_TAU_MIN].toDouble()) + "; " + QString::number(mData[STATE_PHASE_TAU_MAX].toDouble()) + "]";
    }*/
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
    const QRectF rect = boundingRect();
    const QRectF r(rect.x() + rect.width() - mBorderWidth - mEltsMargin - mTitleHeight,
             rect.y() + mBorderWidth + mEltsMargin,
             mTitleHeight,
             mTitleHeight);
    /*const QRectF r(rect.x() + rect.width() - mBorderWidth - mEltsMargin - int(1.2*mTitleHeight),
             rect.y() + mBorderWidth + mEltsMargin ,
             int(1.2*mTitleHeight),
             int(1.2*mTitleHeight));*/
    return r;
}

QRectF PhaseItem::extractRect() const
{
    QRectF rect = boundingRect();
    QRectF r(rect.x() + mBorderWidth + mEltsMargin,
             rect.y() + mBorderWidth + mEltsMargin,
             mTitleHeight,
             mTitleHeight);
    return r;
}
