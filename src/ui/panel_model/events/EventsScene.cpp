#include "EventsScene.h"
#include "Event.h"
#include "EventKnown.h"
#include "EventConstraint.h"
#include "EventItem.h"
#include "EventKnownItem.h"
#include "DateItem.h"
#include "EventsSceneArrowItem.h"
#include "EventsSceneArrowTmpItem.h"
#include "ProjectManager.h"
#include "Project.h"
#include "SetProjectState.h"
#include "MainWindow.h"
#include "HelpWidget.h"
#include <QtWidgets>


EventsScene::EventsScene(QGraphicsView* view, QObject* parent):QGraphicsScene(parent),
mView(view),
mDrawingArrow(false),
mUpdatingItems(false)
{
    mTempArrow = new EventsSceneArrowTmpItem(this);
    addItem(mTempArrow);
    mTempArrow->setVisible(false);
    mTempArrow->setZValue(0);
    
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

#pragma mark Help Bubble
void EventsScene::updateHelp()
{
    QString text;
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
        if(dynamic_cast<EventKnownItem*>(selected[0]))
            text = tr("Edit bound properties from the right panel.");
        else
            text = tr("Edit event properties from the right panel.");
    }
    else
    {
        text = tr("You have selected multiple elements. You can move them together or delete them (all constraints linked to them will also be deleted).");
    }
    
    mHelpView->setText(text);
    mHelpView->setGeometry(mHelpView->x(),
                           mHelpView->y(),
                           mHelpView->width(),
                           mHelpView->heightForWidth(mHelpView->width()));
}

HelpWidget* EventsScene::getHelpView()
{
    return mHelpView;
}

#pragma mark Project Update
void EventsScene::sendUpdateProject(const QString& reason, bool notify, bool storeUndoCommand)
{
    Project* project = ProjectManager::getProject();
    
    QJsonObject statePrev = project->state();
    QJsonObject stateNext = statePrev;
    
    QJsonArray events;
    for(int i=0; i<mItems.size(); ++i)
        events.append(mItems[i]->event());
    stateNext[STATE_EVENTS] = events;
    
    if(statePrev != stateNext)
    {
        if(storeUndoCommand)
            ProjectManager::getProject()->pushProjectState(stateNext, reason, notify);
        else
            ProjectManager::getProject()->sendUpdateState(stateNext, reason, notify);
    }
}

void EventsScene::updateProject()
{
    QJsonObject state = ProjectManager::getProject()->state();
    QJsonArray events = state[STATE_EVENTS].toArray();
    
    QList<int> events_ids;
    for(int i=0; i<events.size(); ++i)
        events_ids << events[i].toObject()[STATE_EVENT_ID].toInt();
    
    mUpdatingItems = true;
    
    // ------------------------------------------------------
    //  Delete items not in current state
    // ------------------------------------------------------
    qDebug() << "=> Delete items";
    for(int i=mItems.size()-1; i>=0; --i)
    {
        EventItem* eventItem = mItems[i];
        QJsonObject& event = eventItem->event();
        
        if(!events_ids.contains(event[STATE_EVENT_ID].toInt()))
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
            qDebug() << "Event deleted : " << event[STATE_EVENT_ID].toInt();
            removeItem(eventItem);
            mItems.removeOne(eventItem);
            delete eventItem;
        }
    }
    
    // ------------------------------------------------------
    //  Create / Update event items
    // ------------------------------------------------------
    qDebug() << "=> Create / Update items";
    for(int i=0; i<events.size(); ++i)
    {
        QJsonObject event = events[i].toObject();
        
        bool itemExists = false;
        for(int j=0; j<mItems.size(); ++j)
        {
            QJsonObject itemEvent = mItems[j]->event();
            if(itemEvent[STATE_EVENT_ID].toInt() == event[STATE_EVENT_ID].toInt())
            {
                itemExists = true;
                if(event != itemEvent)
                {
                    // UPDATE ITEM
                    qDebug() << "Event updated : id = " << event[STATE_EVENT_ID].toInt();
                    mItems[j]->setEvent(event);
                }
            }
        }
        if(!itemExists)
        {
            // CREATE ITEM
            EventItem* eventItem = 0;
            Event::Type type = (Event::Type)event[STATE_EVENT_TYPE].toInt();
            if(type == Event::eDefault)
                eventItem = new EventItem(this, event);
            else if(type == Event::eKnown)
                eventItem = new EventKnownItem(this, event);
            
            mItems.append(eventItem);
            addItem(eventItem);
            qDebug() << "Event created : id = " << event[STATE_EVENT_ID].toInt() << ", type : " << type;
        }
    }
    mUpdatingItems = false;
    update();
}


#pragma mark Selection & Current
void EventsScene::updateSelection()
{
    if(!mUpdatingItems)
    {
        for(int i=0; i<mItems.size(); ++i)
        {
            QJsonObject& event = mItems[i]->event();
            event[STATE_EVENT_IS_SELECTED] = mItems[i]->isSelected();
            event[STATE_EVENT_IS_CURRENT] = false;
        }
        QJsonObject event;
        EventItem* curItem = currentItem();
        if(curItem)
        {
            QJsonObject& evt = curItem->event();
            evt[STATE_EVENT_IS_CURRENT] = true;
            event = evt;
        }
        emit ProjectManager::getProject()->currentEventChanged(event);
        sendUpdateProject(tr("selection updated"), false, false);
        // TODO : update event in Props view
    }
}

#pragma mark Utilities
EventItem* EventsScene::currentItem()
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

EventItem* EventsScene::collidingItem(QGraphicsItem* item)
{
    for(int i=0; i<mItems.size(); ++i)
    {
        if(item != mItems[i] && item->collidesWithItem(mItems[i]))
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
        EventItem* hoveredEventItem = collidingItem(dateItem);
        EventItem* prevEventItem = (EventItem*)dateItem->parentItem();
        
        if(hoveredEventItem != lastEntered)
        {
            if(lastEntered)
                lastEntered->setMergeable(false);
            lastEntered = hoveredEventItem;
            
            if(hoveredEventItem && prevEventItem && (hoveredEventItem != prevEventItem))
            {
                const QJsonObject& event = hoveredEventItem->event();
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
        EventItem* hoveredEventItem = collidingItem(dateItem);
        EventItem* prevEventItem = (EventItem*)dateItem->parentItem();
        
        if(hoveredEventItem && prevEventItem && (hoveredEventItem != prevEventItem))
        {
            const QJsonObject& prevEvent = prevEventItem->event();
            const QJsonObject& nextEvent = hoveredEventItem->event();
            const QJsonObject& dateMoving = dateItem->date();
            
            if(nextEvent[STATE_EVENT_TYPE].toInt() == Event::eDefault)
            {
                // Move the date to another event :
                
                Project* project = ProjectManager::getProject();
                QJsonObject state = project->state();
                QJsonArray events = state[STATE_EVENTS].toArray();
                
                for(int i=0; i<events.size(); ++i)
                {
                    QJsonObject event = events[i].toObject();
                    
                    // remove date from previous event :
                    if(event[STATE_EVENT_ID].toInt() == prevEvent[STATE_EVENT_ID].toInt())
                    {
                        QJsonArray dates = event[STATE_EVENT_DATES].toArray();
                        for(int j=0; j<dates.size(); ++j)
                        {
                            QJsonObject date = dates[j].toObject();
                            if(date[STATE_DATE_ID].toInt() == dateMoving[STATE_DATE_ID].toInt())
                            {
                                dates.removeAt(j);
                                break;
                            }
                        }
                        event[STATE_EVENT_DATES] = dates;
                    }
                    // add date to next event :
                    else if(event[STATE_EVENT_ID].toInt() == nextEvent[STATE_EVENT_ID].toInt())
                    {
                        QJsonArray dates = event[STATE_EVENT_DATES].toArray();
                        dateMoving[STATE_DATE_ID] = project->getUnusedDateId(dates);
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
#pragma mark Event Items

void EventsScene::eventClicked(EventItem* item, QGraphicsSceneMouseEvent* e)
{
    /*Q_UNUSED(e);
    if(mDrawingArrow)
    {
        EventItem* current = currentItem();
        if(current && item != current)
        {
            QString message;
            Project* project = ProjectManager::getProject();
            if(!project->createEventConstraint(current->mEvent, item->mEvent, message))
            {
                QString m(message);
                QMessageBox message(QMessageBox::Critical, tr("Constraint refused"), m, QMessageBox::Ok, qApp->activeWindow(), Qt::Sheet);
                message.exec();
                
                update();
            }
            else
            {
                mTempArrow->setVisible(false);
                e->accept();
            }
        }
    }*/
}

void EventsScene::eventDoubleClicked(EventItem* item, QGraphicsSceneMouseEvent* e)
{
    /*Q_UNUSED(e);
    Q_UNUSED(item);
    if(!mDrawingArrow)
    {
        //((MainWindow*)qApp->activeWindow())->showDates(item->mEvent);
        emit eventDoubleClicked(item->mEvent);
    }*/
}

void EventsScene::eventEntered(EventItem* item, QGraphicsSceneHoverEvent* e)
{
    /*Q_UNUSED(e);
    
    if(mDrawingArrow)
    {
        Project* project = ProjectManager::getProject();
        EventItem* current = currentItem();
        QString message;
        if(project->isEventConstraintAllowed(current->mEvent, item->mEvent, message))
        {
            mTempArrow->setState(EventsSceneArrowTmpItem::eAllowed);
        }
        else
        {
            mTempArrow->setState(EventsSceneArrowTmpItem::eForbidden);
        }
    }
    mTempArrow->setTo(item->pos().x(), item->pos().y());
    mTempArrow->setLocked(true);*/
}

void EventsScene::eventLeaved(EventItem* item, QGraphicsSceneHoverEvent* e)
{
    /*Q_UNUSED(item);
    Q_UNUSED(e);
    mTempArrow->setLocked(false);
    mTempArrow->setState(EventsSceneArrowTmpItem::eNormal);*/
}

void EventsScene::eventMoved(EventItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    
    //item->mEvent->mItemX = item->pos().x();
    //item->mEvent->mItemY = item->pos().y();
    
    // Move constraints
    updateConstraintsPos();
    
    // Ajust Scene rect to minimal
    //setSceneRect(itemsBoundingRect());
    
    // Follow the moving item
    /*QList<QGraphicsView*> graphicsViews = views();
    for(int i=0; i<graphicsViews.size(); ++i)
    {
        graphicsViews[i]->ensureVisible(item);
        graphicsViews[i]->centerOn(item->pos());
    }*/
    
    // TODO : prevent collisions here ?
    if(e->modifiers() == Qt::ShiftModifier)
    {
        EventItem* colliding = collidingItem(item);
        for(int i=0; i<mItems.size(); ++i)
        {
            mItems[i]->setMergeable(colliding != 0 && (mItems[i] == item || mItems[i] == colliding));
        }
    }
}

void EventsScene::eventReleased(EventItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    
    if(e->modifiers() == Qt::ShiftModifier)
    {
        EventItem* colliding = collidingItem(item);
        if(colliding)
        {
            qDebug() << "fusion";
            Project* project = ProjectManager::getProject();
            //project->mergeEvents(item->mEvent, colliding->mEvent);
        }
    }
    for(int i=0; i<mItems.size(); ++i)
        mItems[i]->setMergeable(false);
    
    sendUpdateProject(tr("event moved"), true, true);
}

/*void EventsScene::handleEventMoved(EventItem* item)
{
    Q_UNUSED(item);
    
    if(mDrawingArrow)
    {
        mTempArrow->setFrom(item->pos().x(), item->pos().y());
    }
    for(int i=0; i<mConstraintItems.size(); ++i)
    {
        mConstraintItems[i]->updatePosition();
    }
}*/


// ----------------------------------------------------------------------------------------
//  Constraints Add / Delete / Update
// ----------------------------------------------------------------------------------------
#pragma mark Constraints Add / Delete / Update

void EventsScene::createEventConstraint(EventConstraint* constraint)
{
    /*if(constraint)
    {
        EventsSceneArrowItem* arrow = new EventsSceneArrowItem(this, constraint);
        for(int i=0; i<mItems.size(); ++i)
        {
            if(mItems[i]->mEvent == constraint->getEventFrom())
                arrow->mItemFrom = mItems[i];
            else if(mItems[i]->mEvent == constraint->getEventTo())
                arrow->mItemTo = mItems[i];
        }
        addItem(arrow);
        arrow->updatePosition();
        mConstraintItems.append(arrow);
    }*/
}

void EventsScene::deleteEventConstraint(EventConstraint* constraint)
{
    for(int i=0; i<mConstraintItems.size(); ++i)
    {
        if(mConstraintItems[i]->mConstraint == constraint)
        {
            EventsSceneArrowItem* item = mConstraintItems[i];
            removeItem(item);
            mConstraintItems.erase(mConstraintItems.begin() + i);
            delete item;
            break;
        }
    }
}

// ----------------------------------------------------------------------------------------
//  Constraints Items Events
// ----------------------------------------------------------------------------------------
#pragma mark Constraints Items Events
void EventsScene::constraintDoubleClicked(EventsSceneArrowItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    Project* project = ProjectManager::getProject();
    //project->updateEventConstraint(item->mConstraint);
}

void EventsScene::updateConstraintsPos()
{
    for(int i=0; i<mConstraintItems.size(); ++i)
    {
        mConstraintItems[i]->updatePosition();
    }
}




// ----------------------------------------------------------------------------------------
//  Scene Events
// ----------------------------------------------------------------------------------------
#pragma mark Scene Events
void EventsScene::keyPressEvent(QKeyEvent* keyEvent)
{
    QGraphicsScene::keyPressEvent(keyEvent);
    
    if(keyEvent->key() == Qt::Key_Delete)
    {
        Project* project = ProjectManager::getProject();
        project->deleteSelectedEvents();
    }
    else if(keyEvent->key() == Qt::Key_N)
    {
        Project* project = ProjectManager::getProject();
        project->createEvent();
    }
    else if(keyEvent->modifiers() == Qt::AltModifier)
    {
        EventItem* curItem = currentItem();
        if(curItem)
        {
            mDrawingArrow = true;
            mTempArrow->setVisible(true);
            mTempArrow->setFrom(curItem->pos().x(), curItem->pos().y());
        }
    }
    else
        keyEvent->ignore();
}

void EventsScene::keyReleaseEvent(QKeyEvent* keyEvent)
{
    mDrawingArrow = false;
    mTempArrow->setVisible(false);
    QGraphicsScene::keyReleaseEvent(keyEvent);
}

#pragma mark Drag & Drop
void EventsScene::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
    if(mDrawingArrow)
    {
        //qDebug() << e->scenePos().x();
        mTempArrow->setVisible(true);
        mTempArrow->setTo(e->scenePos().x(), e->scenePos().y());
    }
    QGraphicsScene::mouseMoveEvent(e);
}

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
    Project* project = ProjectManager::getProject();
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
    
    Project* project = ProjectManager::getProject();
    
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

