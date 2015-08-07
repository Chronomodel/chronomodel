#include "EventItem.h"
#include "Event.h"
#include "Phase.h"
#include "Date.h"
#include "Painting.h"
#include "DateItem.h"
#include "QtUtilities.h"
#include "Painting.h"
#include "MainWindow.h"
#include "Project.h"
#include <QtWidgets>


EventItem::EventItem(EventsScene* scene, const QJsonObject& event, const QJsonObject& settings, QGraphicsItem* parent):AbstractItem(scene, parent)
{
    setEvent(event, settings);
}

EventItem::~EventItem()
{
    
}

#pragma mark Event Managment
QJsonObject& EventItem::getEvent()
{
    return mData;
}

void EventItem::setEvent(const QJsonObject& event, const QJsonObject& settings)
{
    prepareGeometryChange();
    
    // ----------------------------------------------
    //  Update item position and selection
    // ----------------------------------------------
    setSelected(event[STATE_IS_SELECTED].toBool());
    setPos(event[STATE_ITEM_X].toDouble(),
           event[STATE_ITEM_Y].toDouble());
    
    // ----------------------------------------------
    //  Check if item should be greyed out
    // ----------------------------------------------
    //updateGreyedOut();
    
    // ----------------------------------------------
    //  Calculate item size
    // ----------------------------------------------
    qreal w = 150.;
    double h = mTitleHeight + mPhasesHeight + 2*mBorderWidth + 2*mEltsMargin;
    
    QString name = event[STATE_NAME].toString();
    QFont font = qApp->font();
    QFontMetrics metrics(font);
    w = metrics.width(name) + 2*mBorderWidth + 4*mEltsMargin + 2*mTitleHeight;
    
    QJsonArray dates = event[STATE_EVENT_DATES].toArray();
    
    int count = dates.size();
    if(count > 0)
        h += count * (mEltsHeight + mEltsMargin);
    else
        h += mEltsMargin + mEltsHeight;
    
    font.setPointSizeF(pointSize(11));
    metrics = QFontMetrics(font);
    for(int i=0; i<count; ++i)
    {
        QJsonObject date = dates[i].toObject();
        name = date[STATE_NAME].toString();
        int nw = metrics.width(name) + 2*mBorderWidth + 4*mEltsMargin;
        w = (nw > w) ? nw : w;
    }
    w = (w < 150) ? 150 : w;
    
    mSize = QSize(w, h);
    
    if(event[STATE_EVENT_DATES].toArray() != mData[STATE_EVENT_DATES].toArray() || mSettings != settings)
    {
        // ----------------------------------------------
        //  Delete Date Items
        // ----------------------------------------------
        QList<QGraphicsItem*> dateItems = childItems();
        for(int i=0; i<dateItems.size(); ++i)
        {
            mScene->removeItem(dateItems[i]);
            delete dateItems[i];
        }
        
        // ----------------------------------------------
        //  Re-create Date Items
        // ----------------------------------------------
        for(int i=0; i<dates.size(); ++i)
        {
            QJsonObject date = dates[i].toObject();
            QColor color(event[STATE_COLOR_RED].toInt(),
                         event[STATE_COLOR_GREEN].toInt(),
                         event[STATE_COLOR_BLUE].toInt());
            
            try{
                DateItem* dateItem = new DateItem((EventsScene*)mScene, date, color, settings);
                dateItem->setParentItem(this);
                dateItem->setGreyedOut(mGreyedOut);
                
                QPointF pos(0,
                            boundingRect().y() +
                            mTitleHeight +
                            mBorderWidth +
                            2*mEltsMargin +
                            i * (mEltsHeight + mEltsMargin));
                dateItem->setPos(pos);
                dateItem->setOriginalPos(pos);
            }
            catch(QString error){
                QMessageBox message(QMessageBox::Critical,
                                    qApp->applicationName() + " " + qApp->applicationVersion(),
                                    tr("Error : ") + error,
                                    QMessageBox::Ok,
                                    qApp->activeWindow(),
                                    Qt::Sheet);
                message.exec();
            }
        }
    }
    
    mData = event;
    mSettings = settings;

    // ----------------------------------------------
    //  Repaint based on mEvent
    // ----------------------------------------------
    update();
}

void EventItem::setGreyedOut(bool greyedOut)
{
    AbstractItem::setGreyedOut(greyedOut);
    
    QList<QGraphicsItem*> children = childItems();
    for(int i=0; i<children.size(); ++i)
    {
        ((DateItem*)children[i])->setGreyedOut(greyedOut);
    }
    setOpacity(mGreyedOut ? 0.1 : 1);
}

void EventItem::updateGreyedOut()
{
    mGreyedOut = true;
    QJsonObject state = MainWindow::getInstance()->getProject()->state();
    QJsonArray phases = state[STATE_PHASES].toArray();
    QStringList selectedPhasesIds;
    for(int i=0; i<phases.size(); ++i)
    {
        QJsonObject phase = phases[i].toObject();
        bool isSelected = phase[STATE_IS_SELECTED].toBool();
        if(isSelected)
            selectedPhasesIds.append(QString::number(phase[STATE_ID].toInt()));
    }
    if(selectedPhasesIds.size() == 0)
    {
        mGreyedOut = false;
    }
    else
    {
        QString eventPhasesIdsStr = mData[STATE_EVENT_PHASE_IDS].toString();
        QStringList eventPhasesIds = eventPhasesIdsStr.split(",");
        for(int i=0; i<selectedPhasesIds.size(); ++i)
        {
            if(eventPhasesIds.contains(selectedPhasesIds[i]))
            {
                mGreyedOut = false;
                break;
            }
        }
    }
    setOpacity(mGreyedOut ? 0.3 : 1);
}

void EventItem::setDatesVisible(bool visible)
{
    QList<QGraphicsItem*> dateItems = childItems();
    for(int i=0; i<dateItems.size(); ++i)
    {
        dateItems[i]->setVisible(visible);
    }
}

#pragma mark Events
void EventItem::updateItemPosition(const QPointF& pos)
{
    mData[STATE_ITEM_X] = pos.x();
    mData[STATE_ITEM_Y] = pos.y();
}

void EventItem::dropEvent(QGraphicsSceneDragDropEvent* e)
{
    handleDrop(e);
}

void EventItem::handleDrop(QGraphicsSceneDragDropEvent* e)
{
    e->acceptProposedAction();
    Project* project = MainWindow::getInstance()->getProject();
    QJsonObject event = mData;
    
    QJsonArray dates = event[STATE_EVENT_DATES].toArray();
    QList<Date> datesDragged = ((EventsScene*)mScene)->decodeDataDrop(e);
    for(int i=0; i<datesDragged.size(); ++i)
    {
        QJsonObject date = datesDragged[i].toJson();
        date[STATE_ID] = project->getUnusedDateId(dates);
        dates.append(date);
    }
    event[STATE_EVENT_DATES] = dates;
    
    project->updateEvent(event, QObject::tr("Dates added to event (CSV drag)"));
}


void EventItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    
    QRectF rect = boundingRect();
    
    QColor eventColor = QColor(mData[STATE_COLOR_RED].toInt(),
                               mData[STATE_COLOR_GREEN].toInt(),
                               mData[STATE_COLOR_BLUE].toInt());
    painter->setPen(Qt::NoPen);
    painter->setBrush(eventColor);
    painter->drawRect(rect);
    
    
    // Phases
    QJsonArray phases = getPhases();
    QRectF phasesRect(rect.x(), rect.y() + rect.height() - mPhasesHeight, rect.width(), mPhasesHeight);
    phasesRect.adjust(1, 1, -1, -1);
    
    int numPhases = (int)phases.size();
    double w = phasesRect.width()/numPhases;
    
    for(int i=0; i<numPhases; ++i)
    {
        QJsonObject phase = phases[i].toObject();
        QColor c(phase[STATE_COLOR_RED].toInt(),
                 phase[STATE_COLOR_GREEN].toInt(),
                 phase[STATE_COLOR_BLUE].toInt());
        painter->setPen(c);
        painter->setBrush(c);
        painter->drawRect(phasesRect.x() + i*w, phasesRect.y(), w, phasesRect.height());
    }
    
    if(numPhases == 0)
    {
        QFont font = qApp->font();
        font.setPointSizeF(pointSize(11));
        painter->setFont(font);
        painter->fillRect(phasesRect, QColor(0, 0, 0, 180));
        painter->setPen(QColor(200, 200, 200));
        painter->drawText(phasesRect, Qt::AlignCenter, tr("No Phase"));
    }
    
    painter->setPen(QColor(0, 0, 0));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(phasesRect);
    
    
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
    
    QColor frontColor = getContrastedColor(eventColor);
    painter->setPen(frontColor);
    painter->drawText(tr, Qt::AlignCenter, name);
    //painter->drawText(tr, Qt::AlignCenter, QString::number(scenePos().x()) + ", " + QString::number(scenePos().y()));
    
    
    /*if(mGreyedOut)
    {
        painter->setPen(Painting::greyedOut);
        painter->setBrush(Painting::greyedOut);
        painter->drawRect(boundingRect());
    }*/
    
    // Border
    painter->setBrush(Qt::NoBrush);
    if(mMergeable)
    {
        painter->setPen(QPen(Qt::white, 5.f));
        painter->drawRect(rect.adjusted(1, 1, -1, -1));
        
        painter->setPen(QPen(Painting::mainColorLight, 3.f, Qt::DashLine));
        painter->drawRect(rect.adjusted(1, 1, -1, -1));
    }
    else if(isSelected())
    {
        painter->setPen(QPen(Qt::white, 5.f));
        painter->drawRect(rect.adjusted(1, 1, -1, -1));
        
        painter->setPen(QPen(Qt::red, 3.f));
        painter->drawRect(rect.adjusted(1, 1, -1, -1));
    }
    
    painter->restore();
}

QJsonArray EventItem::getPhases() const
{
    QJsonObject state = MainWindow::getInstance()->getProject()->state();
    QJsonArray allPhases = state[STATE_PHASES].toArray();
    QJsonArray phases;
    QString eventPhaseIdsStr = mData[STATE_EVENT_PHASE_IDS].toString();
    QStringList eventPhaseIds = eventPhaseIdsStr.split(",");
    
    for(int i=0; i<allPhases.size(); ++i)
    {
        QJsonObject phase = allPhases[i].toObject();
        QString phaseId = QString::number(phase[STATE_ID].toInt());
        if(eventPhaseIds.contains(phaseId))
            phases.append(phase);
    }
    return phases;
}

#pragma mark Geometry
QRectF EventItem::boundingRect() const
{
    return QRectF(-mSize.width()/2, -mSize.height()/2, mSize.width(), mSize.height());
}
