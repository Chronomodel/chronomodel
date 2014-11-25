#include "EventsScene.h"
#include "Event.h"
#include "EventKnown.h"
#include "EventConstraint.h"
#include "EventItem.h"
#include "EventKnownItem.h"
#include "DateItem.h"
#include "ArrowItem.h"
#include "ArrowTmpItem.h"
#include "SetProjectState.h"
#include "MainWindow.h"
#include "Project.h"
#include "HelpWidget.h"
#include "QtUtilities.h"
#include <QtWidgets>


EventsScene::EventsScene(QGraphicsView* view, QObject* parent):AbstractScene(view, parent)
{
    mHelpView = new HelpWidget(view);
    mHelpTimer = new QTimer(this);
    
    mDatesAnimTimer = new QTimeLine(100);
    mDatesAnimTimer->setFrameRange(0, 2);
    
    mDatesAnim = new QGraphicsItemAnimation();
    mDatesAnim->setTimeLine(mDatesAnimTimer);
    
    connect(this, SIGNAL(selectionChanged()), this, SLOT(updateSelection()));
    connect(mHelpTimer, SIGNAL(timeout()), this, SLOT(updateHelp()));
    
    mHelpTimer->start(200);
}

EventsScene::~EventsScene()
{
    
}

#pragma mark Actions
void EventsScene::deleteSelectedItems()
{
    MainWindow::getInstance()->getProject()->deleteSelectedEvents();
}

void EventsScene::createConstraint(AbstractItem* itemFrom, AbstractItem* itemTo)
{
    QJsonObject eventFrom = ((EventItem*)itemFrom)->getEvent();
    QJsonObject eventTo = ((EventItem*)itemTo)->getEvent();
    
    MainWindow::getInstance()->getProject()->createEventConstraint(eventFrom[STATE_ID].toInt(),
                                                        eventTo[STATE_ID].toInt());
}

void EventsScene::mergeItems(AbstractItem* itemFrom, AbstractItem* itemTo)
{
    QJsonObject eventFrom = ((EventItem*)itemFrom)->getEvent();
    QJsonObject eventTo = ((EventItem*)itemTo)->getEvent();
    
    MainWindow::getInstance()->getProject()->mergeEvents(eventFrom[STATE_ID].toInt(),
                                              eventTo[STATE_ID].toInt());
}

void EventsScene::updateGreyedOutEvents(const QMap<int, bool>& eyedPhases)
{
    qDebug() << "-> Update greyed out events";
    
    // If no phases is eyed, then no event must be greyed out!
    bool noEyedPhases = true;
    QMapIterator<int, bool> iter(eyedPhases);
    while(iter.hasNext())
    {
        iter.next();
        if(iter.value())
        {
            noEyedPhases = false;
            break;
        }
    }
    
    for(int i=0; i<mItems.size(); ++i)
    {
        EventItem* item = (EventItem*)mItems[i];
        if(noEyedPhases)
        {
            item->setGreyedOut(false);
        }
        else
        {
            QString eventPhasesIdsStr = item->mData[STATE_EVENT_PHASE_IDS].toString();
            bool mustBeGreyedOut = true;
            if(!eventPhasesIdsStr.isEmpty())
            {
                QStringList eventPhasesIds = eventPhasesIdsStr.split(",");
                for(int j=0; j<eventPhasesIds.size(); ++j)
                {
                    if(eyedPhases[eventPhasesIds[j].toInt()])
                    {
                        mustBeGreyedOut = false;
                    }
                }
            }
            item->setGreyedOut(mustBeGreyedOut);
        }
    }
}

#pragma mark Help Bubble
void EventsScene::updateHelp()
{
    /*QString text;
    QList<QGraphicsItem*> selected = selectedItems();
    
    if(mItems.size() == 0)
    {
        text = tr("Start creating your model by clicking on \"New Event...\".");
    }
    else if(selected.count() == 0)
    {
        text = tr("Select an event or a bound by clicking on it.");
        if(mConstraintItems.size() != 0)
            text += tr("\nYou can also edit constraints by double clicking on the arrow");
    }
    else if(selected.count() == 1)
    {
        bool isBound = (dynamic_cast<EventKnownItem*>(selected[0]) != 0);
        
        if(mAltIsDown)
        {
            text = tr("Mouve your mouse and click on another element to create a constraint.");
        }
        else if(mShiftIsDown && !isBound)
        {
            text = tr("Drag the event onto another one to merge them together.");
        }
        else
        {
            text = tr("You have selected an element. You can now:\n- Edit its properties from the right panel.\n- Create a constraint by holding the \"Option\" key down and clicking on another element.");
            if(!isBound)
                text += tr("\n- Merge it with another element by holding the \"Shift\" key down and dragging the selected element onto another one.");
        }
    }
    else
    {
        text = tr("You have selected multiple elements. You can move them together or delete them (all constraints linked to them will also be deleted).");
    }
    
    mHelpView->setText(text);
    mHelpView->setGeometry(mHelpView->x(),
                           mHelpView->y(),
                           mHelpView->width(),
                           mHelpView->heightForWidth(mHelpView->width()));*/
}

HelpWidget* EventsScene::getHelpView()
{
    return mHelpView;
}

void EventsScene::showHelp(bool show)
{
    mHelpView->setVisible(show);
}

#pragma mark Project Update
void EventsScene::sendUpdateProject(const QString& reason, bool notify, bool storeUndoCommand)
{
    Project* project = MainWindow::getInstance()->getProject();
    
    QJsonObject statePrev = project->state();
    QJsonObject stateNext = statePrev;
    
    QJsonArray events;
    for(int i=0; i<mItems.size(); ++i)
    {
        EventItem* item = (EventItem*)mItems[i];
        events.append(item->getEvent());
    }
    stateNext[STATE_EVENTS] = events;
    
    if(statePrev != stateNext)
    {
        if(storeUndoCommand)
            MainWindow::getInstance()->getProject()->pushProjectState(stateNext, reason, notify);
        else
            MainWindow::getInstance()->getProject()->sendUpdateState(stateNext, reason, notify);
    }
}

void EventsScene::updateProject()
{
    QJsonObject state = MainWindow::getInstance()->getProject()->state();
    QJsonArray events = state[STATE_EVENTS].toArray();
    QJsonArray constraints = state[STATE_EVENTS_CONSTRAINTS].toArray();
    QJsonObject settings = state[STATE_SETTINGS].toObject();
    
    QList<int> events_ids;
    for(int i=0; i<events.size(); ++i)
        events_ids << events[i].toObject()[STATE_ID].toInt();
    
    QList<int> constraints_ids;
    for(int i=0; i<constraints.size(); ++i)
        constraints_ids << constraints[i].toObject()[STATE_ID].toInt();
    
    mUpdatingItems = true;
    
    // ------------------------------------------------------
    //  Delete items not in current state
    // ------------------------------------------------------
    for(int i=mItems.size()-1; i>=0; --i)
    {
        EventItem* eventItem = (EventItem*)mItems[i];
        QJsonObject& event = eventItem->getEvent();
        
        if(!events_ids.contains(event[STATE_ID].toInt()))
        {
            if(event[STATE_EVENT_TYPE].toInt() == Event::eDefault)
            {
                QList<QGraphicsItem*> dateItems = eventItem->childItems();
                for(int j=0; j<dateItems.size(); ++j)
                {
                    removeItem(dateItems[j]);
                    delete dateItems[j];
                }
            }
            qDebug() << "Event deleted : " << event[STATE_ID].toInt();
            removeItem(eventItem);
            mItems.removeOne(eventItem);
            delete eventItem;
        }
    }
    
    // ------------------------------------------------------
    //  Create / Update event items
    // ------------------------------------------------------
    for(int i=0; i<events.size(); ++i)
    {
        QJsonObject event = events[i].toObject();
        
        bool itemExists = false;
        for(int j=0; j<mItems.size(); ++j)
        {
            EventItem* item = (EventItem*)mItems[j];
            QJsonObject itemEvent = item->getEvent();
            if(itemEvent[STATE_ID].toInt() == event[STATE_ID].toInt())
            {
                itemExists = true;
                if(event != itemEvent)
                {
                    // UPDATE ITEM
                    qDebug() << "Event updated : id = " << event[STATE_ID].toInt();
                    item->setEvent(event, settings);
                }
            }
        }
        if(!itemExists)
        {
            // CREATE ITEM
            EventItem* eventItem = 0;
            Event::Type type = (Event::Type)event[STATE_EVENT_TYPE].toInt();
            if(type == Event::eDefault)
                eventItem = new EventItem(this, event, settings);
            else if(type == Event::eKnown)
                eventItem = new EventKnownItem(this, event, settings);
            
            mItems.append(eventItem);
            addItem(eventItem);
            qDebug() << "Event created : id = " << event[STATE_ID].toInt() << ", type : " << type;
        }
    }
    
    // ------------------------------------------------------
    //  Delete constraints not in current state
    // ------------------------------------------------------
    for(int i=mConstraintItems.size()-1; i>=0; --i)
    {
        ArrowItem* constraintItem = mConstraintItems[i];
        QJsonObject& constraint = constraintItem->data();
        
        if(!constraints_ids.contains(constraint[STATE_ID].toInt()))
        {
            qDebug() << "Constraint deleted : " << constraint[STATE_ID].toInt();
            removeItem(constraintItem);
            mConstraintItems.removeOne(constraintItem);
            delete constraintItem;
        }
    }
    
    // ------------------------------------------------------
    //  Create / Update constraint items
    // ------------------------------------------------------
    for(int i=0; i<constraints.size(); ++i)
    {
        QJsonObject constraint = constraints[i].toObject();
        
        bool itemExists = false;
        for(int j=0; j<mConstraintItems.size(); ++j)
        {
            QJsonObject constraintItem = mConstraintItems[j]->data();
            if(constraintItem[STATE_ID].toInt() == constraint[STATE_ID].toInt())
            {
                itemExists = true;
                if(constraint != constraintItem)
                {
                    // UPDATE ITEM
                    qDebug() << "Constraint updated : id = " << constraint[STATE_ID].toInt();
                    mConstraintItems[j]->setData(constraint);
                }
            }
        }
        if(!itemExists)
        {
            // CREATE ITEM
            ArrowItem* constraintItem = new ArrowItem(this, ArrowItem::eEvent, constraint);
            mConstraintItems.append(constraintItem);
            addItem(constraintItem);
            qDebug() << "Constraint created : id = " << constraint[STATE_ID].toInt();
        }
    }
    
    mUpdatingItems = false;
    updateSelection();
    update();
}


#pragma mark Selection & Current
void EventsScene::updateSelection()
{
    if(!mUpdatingItems)
    {
        for(int i=0; i<mItems.size(); ++i)
        {
            EventItem* item = (EventItem*)mItems[i];
            QJsonObject& event = item->getEvent();
            event[STATE_IS_SELECTED] = mItems[i]->isSelected();
            event[STATE_IS_CURRENT] = false;
            
            if(mItems[i]->isSelected())
                qDebug() << "Marke selected event : " << item->mData[STATE_NAME].toString();
        }
        QJsonObject event;
        EventItem* curItem = (EventItem*)currentItem();
        if(curItem)
        {
            QJsonObject& evt = curItem->getEvent();
            evt[STATE_IS_CURRENT] = true;
            event = evt;
        }
        emit MainWindow::getInstance()->getProject()->currentEventChanged(event);
        sendUpdateProject(tr("events selection updated : events marked as selected"), false, false);
        MainWindow::getInstance()->getProject()->sendEventsSelectionChanged();
    }
}

void EventsScene::updateSelectedEventsFromPhases()
{
    // Do not send "selection updated" each time an item is selected in this function!
    // Do it all at once at the end.
    mUpdatingItems = true;
    
    QJsonObject state = MainWindow::getInstance()->getProject()->state();
    QJsonArray phases = state[STATE_PHASES].toArray();
    
    for(int i=0; i<mItems.size(); ++i)
    {
        EventItem* item = (EventItem*)mItems[i];
        bool mustBeSelected = false;
        QList<int> eventPhasesIds = stringListToIntList(item->mData[STATE_EVENT_PHASE_IDS].toString());
        
        for(int i=0; i<phases.size(); ++i)
        {
            QJsonObject phase = phases[i].toObject();
            int phaseId = phase[STATE_ID].toInt();
            
            if(eventPhasesIds.contains(phaseId))
            {
                if(phase[STATE_IS_SELECTED].toBool())
                    mustBeSelected = true;
            }
        }
        item->setSelected(mustBeSelected);
    }
    mUpdatingItems = false;
    updateSelection();
}

#pragma mark Utilities
AbstractItem* EventsScene::currentItem()
{
    QList<QGraphicsItem*> items = selectedItems();
    if(items.size() > 0)
    {
        EventItem* evtItem = dynamic_cast<EventItem*>(items[0]);
        if(evtItem)
            return evtItem;
    }
    return 0;
}

AbstractItem* EventsScene::collidingItem(QGraphicsItem* item)
{
    for(int i=0; i<mItems.size(); ++i)
    {
        bool isBound = (dynamic_cast<EventKnownItem*>(mItems[i]) != 0);
        if(item != mItems[i] && !isBound && item->collidesWithItem(mItems[i]))
            return mItems[i];
    }
    return 0;
}

#pragma mark Dates Items
void EventsScene::dateMoved(DateItem* dateItem, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    if(dateItem)
    {
        static EventItem* lastEntered = 0;
        EventItem* hoveredEventItem = (EventItem*)collidingItem(dateItem);
        EventItem* prevEventItem = (EventItem*)dateItem->parentItem();
        
        if(hoveredEventItem != lastEntered)
        {
            if(lastEntered)
                lastEntered->setMergeable(false);
            lastEntered = hoveredEventItem;
            
            if(hoveredEventItem && prevEventItem && (hoveredEventItem != prevEventItem))
            {
                const QJsonObject& event = hoveredEventItem->getEvent();
                if(event[STATE_EVENT_TYPE].toInt() == Event::eDefault)
                    hoveredEventItem->setMergeable(true);
            }
        }
    }
}

void EventsScene::dateReleased(DateItem* dateItem, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    if(dateItem)
    {
        EventItem* hoveredEventItem = (EventItem*)collidingItem(dateItem);
        EventItem* prevEventItem = (EventItem*)dateItem->parentItem();
        
        if(hoveredEventItem && prevEventItem && (hoveredEventItem != prevEventItem))
        {
            const QJsonObject& prevEvent = prevEventItem->getEvent();
            const QJsonObject& nextEvent = hoveredEventItem->getEvent();
            const QJsonObject& dateMoving = dateItem->date();
            
            if(nextEvent[STATE_EVENT_TYPE].toInt() == Event::eDefault)
            {
                // Move the date to another event :
                
                Project* project = MainWindow::getInstance()->getProject();
                QJsonObject state = project->state();
                QJsonArray events = state[STATE_EVENTS].toArray();
                
                for(int i=0; i<events.size(); ++i)
                {
                    QJsonObject event = events[i].toObject();
                    
                    // remove date from previous event :
                    if(event[STATE_ID].toInt() == prevEvent[STATE_ID].toInt())
                    {
                        QJsonArray dates = event[STATE_EVENT_DATES].toArray();
                        for(int j=0; j<dates.size(); ++j)
                        {
                            QJsonObject date = dates[j].toObject();
                            if(date[STATE_ID].toInt() == dateMoving[STATE_ID].toInt())
                            {
                                dates.removeAt(j);
                                break;
                            }
                        }
                        event[STATE_EVENT_DATES] = dates;
                    }
                    // add date to next event :
                    else if(event[STATE_ID].toInt() == nextEvent[STATE_ID].toInt())
                    {
                        QJsonArray dates = event[STATE_EVENT_DATES].toArray();
                        dateMoving[STATE_ID] = project->getUnusedDateId(dates);
                        dates.append(dateMoving);
                        event[STATE_EVENT_DATES] = dates;
                    }
                    events[i] = event;
                }
                state[STATE_EVENTS] = events;
                project->pushProjectState(state, tr("Date moved to event"), true);
                
                hoveredEventItem->setMergeable(false);
                return;
            }
        }
        
        mDatesAnim->setItem(dateItem);
        mDatesAnim->setPosAt(0, dateItem->pos());
        mDatesAnim->setPosAt(1, dateItem->mOriginalPos);
        mDatesAnimTimer->start();
    }
}

// ----------------------------------------------------------------------------------------
//  Event Items Events
// ----------------------------------------------------------------------------------------
#pragma mark Item mouse events
void EventsScene::itemDoubleClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e)
{
    AbstractScene::itemDoubleClicked(item, e);
    emit eventDoubleClicked();
}

void EventsScene::constraintDoubleClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    Q_UNUSED(item);
    
    QMessageBox message(QMessageBox::Question, tr("Delete constraint"), tr("Do you really want to delete this constraint?"), QMessageBox::Yes | QMessageBox::No, qApp->activeWindow(), Qt::Sheet);
    if(message.exec() == QMessageBox::Yes)
    {
        MainWindow::getInstance()->getProject()->deleteEventConstraint(item->data()[STATE_ID].toInt());
        qDebug() << "TODO : delete constraint";
    }
    
    //Project* project = MainWindow::getInstance()->getProject();
    //project->updateEventConstraint(item->constraint()[STATE_ID].toInt());
}


// -----------------------------------------------------------
//  The following function are about drag & drop
// -----------------------------------------------------------
#pragma mark Drag & Drop
void EventsScene::dragMoveEvent(QGraphicsSceneDragDropEvent* e)
{
    for(int i=0; i<mItems.size(); ++i)
    {
        QRectF r = mItems[i]->boundingRect();
        r.translate(mItems[i]->scenePos());
        if(r.contains(e->scenePos()))
        {
            QGraphicsScene::dragMoveEvent(e);
            return;
        }
    }
    e->accept();
}

void EventsScene::dropEvent(QGraphicsSceneDragDropEvent* e)
{
    for(int i=0; i<mItems.size(); ++i)
    {
        QRectF r = mItems[i]->boundingRect();
        r.translate(mItems[i]->scenePos());
        if(r.contains(e->scenePos()))
        {
            QGraphicsScene::dropEvent(e);
            return;
        }
    }
    
    e->accept();
    
    Project* project = MainWindow::getInstance()->getProject();
    QJsonObject state = project->state();
    QJsonArray events = state[STATE_EVENTS].toArray();
    
    QList<Date> dates = decodeDataDrop(e);
    
    // Create one event per data
    for(int i=0; i<dates.size(); ++i)
    {
        Event event;
        event.mName = tr("Untitled");
        event.mId = project->getUnusedEventId(events);
        dates[i].mId = 0;
        event.mDates.append(dates[i]);
        events.append(event.toJson());
    }
    state[STATE_EVENTS] = events;
    project->pushProjectState(state, tr("Dates added (CSV drag)"), true);
}

QList<Date> EventsScene::decodeDataDrop(QGraphicsSceneDragDropEvent* e)
{
    const QMimeData* mimeData = e->mimeData();
    
    Project* project = MainWindow::getInstance()->getProject();
    
    QByteArray encodedData = mimeData->data("application/chronomodel.import.data");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    
    QList<QStringList> failed;
    QList<int> acceptedRows;
    QList<Date> dates;
    
    while(!stream.atEnd())
    {
        QString itemStr;
        stream >> itemStr;
        QStringList dataStr = itemStr.split(";");
        
        // Remove first column corresponding to csvRow
        int csvRow = dataStr.takeFirst().toInt();
        
        QString pluginName = dataStr.takeFirst();
        
        Date date = project->createDateFromData(pluginName, dataStr);
        if(!date.isNull())
        {
            dates << date;
            acceptedRows.append(csvRow);
        }
        else
        {
            failed.append(dataStr);
        }
    }
    emit csvDataLineDropAccepted(acceptedRows);
    return dates;
}

