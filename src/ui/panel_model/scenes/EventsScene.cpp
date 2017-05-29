#include "EventsScene.h"
#include "Event.h"
#include "EventKnown.h"
#include "EventConstraint.h"
#include "EventItem.h"
#include "EventKnownItem.h"
#include "DateItem.h"
#include "ArrowItem.h"
#include "ArrowTmpItem.h"
#include "MainWindow.h"
#include "Project.h"
#include "HelpWidget.h"
#include "QtUtilities.h"
#include <QtWidgets>
#include <QProgressDialog>


EventsScene::EventsScene(QGraphicsView* view, QObject* parent):AbstractScene(view, parent)
{
    mHelpView = new HelpWidget(view);
    mHelpView->setLink("http://www.chronomodel.fr/Chronomodel_User_Manual.pdf#page=9");
    mHelpTimer = new QTimer(this);
    
    mDatesAnimTimer = new QTimeLine(100);
    mDatesAnimTimer->setFrameRange(0, 2);
    
    mDatesAnim = new QGraphicsItemAnimation();
    mDatesAnim->setTimeLine(mDatesAnimTimer);
    
    connect(this, &QGraphicsScene::selectionChanged, this, &EventsScene::updateStateSelectionFromItem);
    connect(mHelpTimer, SIGNAL(timeout()), this, SLOT(updateHelp()));
    
    mHelpTimer->start(200);
}

EventsScene::~EventsScene()
{
    
}

EventItem* EventsScene::findEventItemWithJsonId(const int id)
{
    foreach(AbstractItem* it, mItems) {
        EventItem* ev = static_cast<EventItem*>(it);
        const QJsonObject evJson = ev->getEvent();
        if (evJson.value(STATE_ID)== id)
            return ev;
    }
    return nullptr;
}


/* Actions
 */
void EventsScene::deleteSelectedItems()
{
   qDebug()<<"EventsScene::deleteSelectedItems";
   mProject->deleteSelectedEvents();
}

bool EventsScene::constraintAllowed(AbstractItem* itemFrom, AbstractItem* itemTo)
{
    const QJsonArray constraints = mProject->mState.value(STATE_EVENTS_CONSTRAINTS).toArray();
    const QJsonObject eventFrom = dynamic_cast<EventItem*>(itemFrom)->getEvent();
    const QJsonObject eventTo = dynamic_cast<EventItem*>(itemTo)->getEvent();
    
    const int eventFromId = eventFrom.value(STATE_ID).toInt();
    const int eventToId = eventTo.value(STATE_ID).toInt();
    
    bool constraintAllowed = true;
    
    for (int i=0; i<constraints.size(); ++i) {
        QJsonObject constraint = constraints.at(i).toObject();
        // If the constraint already exist, impossible to create another identical one.
        if (constraint.value(STATE_CONSTRAINT_BWD_ID).toInt() == eventFromId && constraint.value(STATE_CONSTRAINT_FWD_ID).toInt() == eventToId) {
            constraintAllowed = false;
            qDebug() << "EventsScene::constraintAllowed: not Allowed " ;
        }
        // Impossible to have 2 constraints with oposite directions, between 2 events.
        else if (constraint.value(STATE_CONSTRAINT_BWD_ID).toInt() == eventToId && constraint.value(STATE_CONSTRAINT_FWD_ID).toInt() == eventFromId) {
            constraintAllowed = false;
            qDebug() << "EventsScene::constraintAllowed: not Allowed Inversion" ;
        }
        
    }
    if (constraintAllowed && constraintIsCircular(constraints, eventFromId, eventToId)) {
        constraintAllowed = false;
        qDebug() << "EventsScene::constraintAllowed: not Allowed Circular" ;
    }
    return constraintAllowed;
}

void EventsScene::createConstraint(AbstractItem* itemFrom, AbstractItem* itemTo)
{
    const QJsonObject eventFrom = dynamic_cast<EventItem*>(itemFrom)->getEvent();
    const QJsonObject eventTo = dynamic_cast<EventItem*>(itemTo)->getEvent();
    qDebug()<<"EventsScene::createConstraint"<<eventFrom.value(STATE_NAME)<<eventTo.value(STATE_NAME);

    mProject->createEventConstraint(eventFrom.value(STATE_ID).toInt(),
                                    eventTo.value(STATE_ID).toInt());

}

void EventsScene::mergeItems(AbstractItem* itemFrom, AbstractItem* itemTo)
{
    QJsonObject& eventFrom = dynamic_cast<EventItem*>(itemFrom)->getEvent();
    QJsonObject& eventTo = dynamic_cast<EventItem*>(itemTo)->getEvent();
    
    mProject->mergeEvents(eventFrom.value(STATE_ID).toInt(),
                                              eventTo.value(STATE_ID).toInt());
}


/* Help Bubble
 *
 */
void EventsScene::updateHelp()
{
    QString text;
    QList<QGraphicsItem*> selected = selectedItems();
    
    if (mItems.size() == 0) {
        text = tr("Define a study period on the right panel, apply it, and start creating your model by clicking on \"New Event...\".");
        mHelpView->setLink("http://www.chronomodel.fr/Chronomodel_User_Manual.pdf#page=24"); // chapter
    }
    else if(selected.count() == 0) {
        text = tr("Select an event or a bound by clicking on it.");
        if (mConstraintItems.size() != 0)
            text += tr("\nYou can also edit constraints by double clicking on the arrow");
        mHelpView->setLink("http://www.chronomodel.fr/Chronomodel_User_Manual.pdf#page=24"); // Chapter
    } else if (selected.count() == 1) {
        const bool isBound = (dynamic_cast<EventKnownItem*>(selected[0]) != 0);
        
        if (mAltIsDown) {
            text = tr("Mouve your mouse and click on another element to create a constraint.");
                mHelpView->setLink("http://www.chronomodel.fr/Chronomodel_User_Manual.pdf#page=24");
        } else if (mShiftIsDown && !isBound) {
            text = tr("Drag the event onto another one to merge them together.");
                mHelpView->setLink("http://www.chronomodel.fr/Chronomodel_User_Manual.pdf#page=24");
        } else {
            text = tr("You have selected an element. You can now:\r- Edit its properties from the right panel.\r- Create a constraint by holding the \"Alt\" key down and clicking on another element.");
                mHelpView->setLink("http://www.chronomodel.fr/Chronomodel_User_Manual.pdf#page=24");
            if (!isBound)
                text += tr("\r- Merge it with another element by holding the \"Shift\" key down and dragging the selected element onto another one.\r- Delete it with the button on the left.");
        }
    } else {
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

void EventsScene::showHelp(bool show)
{
    mHelpView->setVisible(show);
}


void EventsScene::sendUpdateProject(const QString& reason, bool notify, bool storeUndoCommand)
{
    qDebug()<<"EventsScene::sendUpdateProject";

    QJsonObject statePrev = mProject->state();
    QJsonObject stateNext = statePrev;
    
    //mData in the mItems are copies of the Model(State) while doing setEvent(), so we have to rebuild the
    // State with correct value of the selection
    QJsonArray events = QJsonArray();
    for (int i=0; i<mItems.size(); ++i)
        events.append(((EventItem*)mItems.at(i))->getEvent());


    stateNext[STATE_EVENTS] = events;

    if (statePrev != stateNext) {
        qDebug()<<"EventsScene::sendUpdateProject stateChange";
        if (storeUndoCommand)
            mProject->pushProjectState(stateNext, reason, notify, true);
        //else
            mProject->sendUpdateState(stateNext, reason, notify);
    }
}
void EventsScene::noHide()
{
    qDebug()<<"EventsScene::noHide";
    setShowAllThumbs(true);
}
void EventsScene::phasesSelected()
{
    qDebug()<<"EventsScene::phasesSelected";
   setShowAllThumbs(false);
}

void EventsScene::setShowAllThumbs(const bool show)
{
    mShowAllThumbs = show ;

    // update EventItem GreyedOut according to the phase selection
    for (QList<AbstractItem*>::iterator cIter = mItems.begin(); cIter != mItems.end(); ++cIter) {
        bool selectedPhase = false;
        QJsonArray phases = dynamic_cast<EventItem*>(*cIter)->getPhases();
        foreach (const QJsonValue phase, phases) {
                if ((selectedPhase == false) && (phase.toObject().value(STATE_IS_SELECTED).toBool() == true)) {
                        selectedPhase = true;
                        qDebug()<<"EventsScene::setShowAllThumbs Phase Selected: "<<phase.toObject().value(STATE_NAME).toString();
                }
         }
        dynamic_cast<EventItem*>(*cIter)->setWithSelectedPhase(selectedPhase);
        if (selectedPhase || show)
            dynamic_cast<EventItem*>(*cIter)->setGreyedOut(false);
        else
            dynamic_cast<EventItem*>(*cIter)->setGreyedOut(true);

    }

     // update constraintItems GreyedOut according to the EventItem GreyedOut
    for (QList<ArrowItem*>::iterator cIter = mConstraintItems.begin(); cIter != mConstraintItems.end(); ++cIter) {

             const int eventFromId = (*cIter)->mData.value(STATE_CONSTRAINT_BWD_ID).toInt();
             const int eventToId = (*cIter)->mData.value(STATE_CONSTRAINT_FWD_ID).toInt();

             EventItem* evFrom = findEventItemWithJsonId(eventFromId);
             EventItem* evTo = findEventItemWithJsonId(eventToId);

             if (!evFrom->mGreyedOut || !evTo->mGreyedOut || show)
                 (*cIter)->setGreyedOut(false);
             else
                  (*cIter)->setGreyedOut(true);

    }
}


void EventsScene::createSceneFromState()
{

#ifdef DEBUG
   qDebug()<<"EventsScene::createSceneFromState()";
   QTime startTime = QTime::currentTime();
#endif


    const QJsonObject state = mProject->state();
    const QJsonArray eventsInState = state.value(STATE_EVENTS).toArray();
    const QJsonArray constraints = state.value(STATE_EVENTS_CONSTRAINTS).toArray();
    const QJsonObject settings = state.value(STATE_SETTINGS).toObject();


   // EventItem* curItem = 0;

     //http://doc.qt.io/qt-5/qprogressdialog.html#minimumDuration-prop

    QProgressDialog* progress = new QProgressDialog("Create event items","Wait" , 1, eventsInState.size(),qApp->activeWindow());
    progress->setWindowModality(Qt::WindowModal);
    progress->setCancelButton(0);

    mSettings = ProjectSettings::fromJson(settings);

    mUpdatingItems = true;

    // ------------------------------------------------------
    //  Delete items not in current state
    // ------------------------------------------------------
    clear();
    // this item is delete with clear, but we need it.
    mTempArrow = new ArrowTmpItem();
    addItem(mTempArrow);
    mTempArrow->setVisible(false);

    clearSelection();

    // ------------------------------------------------------
    //  Create event items
    // ------------------------------------------------------
    int i(0);

    progress->setLabelText(tr("Create event items"));

    for (QJsonArray::const_iterator citer = eventsInState.constBegin(); citer != eventsInState.constEnd(); ++citer) {
        const QJsonObject event = (*citer).toObject();
        ++i;

        progress->setValue(i);

       // CREATE ITEM
        Event::Type type = (Event::Type)event.value(STATE_EVENT_TYPE).toInt();
        EventItem* newItem = nullptr;
        if (type == Event::eDefault)
            newItem = new EventItem(this, event, settings);
        else //if(type == Event::eKnown)
            newItem = new EventKnownItem(this, event, settings);

        mItems.append(newItem);
        addItem(newItem);

        }


    // ------------------------------------------------------
    //  Create constraint items
    // ------------------------------------------------------

    progress->setLabelText(tr("Create constraint items"));
    progress->setMaximum(constraints.size());
    for (int i=0; i<constraints.size(); ++i) {
        const QJsonObject constraint = constraints.at(i).toObject();
        progress->setValue(i);
        ArrowItem* constraintItem = new ArrowItem(this, ArrowItem::eEvent, constraint);
        mConstraintItems.append(constraintItem);
        addItem(constraintItem);

    }

    mUpdatingItems = false;

    delete progress;
 #ifdef DEBUG
     QTime timeDiff(0,0,0,1);
     timeDiff = timeDiff.addMSecs(startTime.elapsed()).addMSecs(-1);

    qDebug()<<"EventsScene::createScene() finish at " + timeDiff.toString("hh:mm:ss.zzz");
 #endif
}


/**
 * @brief EventsScene::updateSceneFromState , it is done after each Project::pushProjectState()
 */
void EventsScene::updateSceneFromState()
{

#ifdef DEBUG
   qDebug()<<"EventsScene::updateSceneFromState()";
   QTime startTime = QTime::currentTime();
#endif
    QJsonObject state = mProject->state();
    const QJsonArray eventsInNewState = state.value(STATE_EVENTS).toArray();
    const QJsonArray constraints = state.value(STATE_EVENTS_CONSTRAINTS).toArray();
    QJsonObject settings = state.value(STATE_SETTINGS).toObject();

    QProgressDialog* progress = nullptr;
    bool displayProgress = false;

    EventItem* curItem = currentEvent();
    QJsonObject currentEventPrev = QJsonObject();
    if (curItem)
        currentEventPrev = curItem->getEvent();

    if (mItems.size() != eventsInNewState.size()) {
        //http://doc.qt.io/qt-5/qprogressdialog.html#minimumDuration-prop
        displayProgress = true;
        progress = new QProgressDialog("Create / Update event items","Wait" , 1, eventsInNewState.size(),qApp->activeWindow());
        progress->setWindowModality(Qt::WindowModal);
        progress->setCancelButton(nullptr);
    }

    QList<int> events_ids_inNewState;
    for (int i=0; i<eventsInNewState.size(); ++i)
        events_ids_inNewState << eventsInNewState.at(i).toObject().value(STATE_ID).toInt();
    
    QList<int> constraints_ids;
    for (int i=0; i<constraints.size(); ++i)
        constraints_ids << constraints.at(i).toObject().value(STATE_ID).toInt();
    
    // ------------------------------------------------------
    //  Update settings
    // ------------------------------------------------------
    bool settingsChanged = false;
    ProjectSettings s = ProjectSettings::fromJson(settings);
    if (mSettings != s) {
        settingsChanged = true;
        mSettings = s;
    }
    
    mUpdatingItems = true;

    //progress->show();
    // ------------------------------------------------------
    //  Delete items not in current state
    // ------------------------------------------------------
    bool hasDeleted = false;
    for (int i=mItems.size()-1; i>=0; --i) {
        EventItem* eventItem = (EventItem*)mItems[i];
        QJsonObject& event = eventItem->getEvent();
        
        if (!events_ids_inNewState.contains(event.value(STATE_ID).toInt())) {
            if (event.value(STATE_EVENT_TYPE).toInt() == Event::eDefault) {
                QList<QGraphicsItem*> dateItems = eventItem->childItems();
                for (int j=0; j<dateItems.size(); ++j) {
                    removeItem(dateItems.at(j));
                    delete dateItems[j];
                }
            }
            qDebug() << "EventsScene::updateScene Event deleted : " << event.value(STATE_ID).toInt();

            mItems.removeAt(i);
            hasDeleted = true;
            
            // This does not break the code but is commented to match PhasesScene implementation
            //removeItem(eventItem);
            
            eventItem->deleteLater();
        }
    }
    
    bool hasCreated = false;
   // ------------------------------------------------------
    //  Create / Update event items
    // ------------------------------------------------------
    int i = 0;
    if (displayProgress)
        progress->setLabelText(tr("Create / Update event items"));
    
   for (auto && citer : eventsInNewState) {
        const QJsonObject event = citer.toObject();
        ++i;
        if (displayProgress)
            progress->setValue(i);

        bool itemUnkown = true;
        for (auto && cIterOld : mItems) {
            EventItem* oldItem = (EventItem*) cIterOld;
            const QJsonObject OldItemEvent = oldItem->getEvent();

            if ( OldItemEvent.value(STATE_ID).toInt() == event.value(STATE_ID).toInt()) {

                itemUnkown = false;
                if (event != OldItemEvent || settingsChanged) {
                    // UPDATE ITEM
                    qDebug() << "EventsScene::updateScene Event updated : id = " << event.value(STATE_ID).toInt()<< event.value(STATE_NAME).toString();

                    oldItem->setEvent(event, settings);
                }
             }

            oldItem = nullptr;
        }
        if (itemUnkown) {
            // CREATE ITEM
                Event::Type type = (Event::Type)event.value(STATE_EVENT_TYPE).toInt();
                EventItem* newItem = nullptr;
                if (type == Event::eDefault)
                    newItem = new EventItem(this, event, settings);
                else //if(type == Event::eKnown)
                    newItem = new EventKnownItem(this, event, settings);

                mItems.append(newItem);
                addItem(newItem);
                hasCreated = true;


                // Note : setting an event in (0, 0) tells the scene that this item is new! don't work when updateScene come from Undo Action
                // Thus the scene will move it randomly around the currently viewed center point.
                QPointF pos = newItem->pos();
                if (pos.isNull()) {
                    // Usefull to add some dates inside the last created Event!
                    clearSelection();
                    newItem->setSelected(true);

                    QList<QGraphicsView*> gviews = views();
                    
                    if (gviews.size() > 0) {
                        QGraphicsView* gview = gviews[0];
                        QPointF pt = gview->mapToScene(gview->width()/2, gview->height()/2);
                        const int posDelta (100);
                        newItem->setPos(pt.x() + rand() % posDelta - posDelta/2,
                                          pt.y() + rand() % posDelta - posDelta/2);
                    }

                newItem = nullptr;

                qDebug() << "EventsScene::updateScene Event created : id = " << event.value(STATE_ID).toInt() <<event[STATE_ITEM_X].toDouble()<<event[STATE_ITEM_Y].toDouble();//<< event.value(STATE_NAME).toString()<<", type : " << type;

            }

        }

    }

    // ------------------------------------------------------
    //  Delete constraints not in current state
    // ------------------------------------------------------
    for (int i=mConstraintItems.size()-1; i>=0; --i) {
        ArrowItem* constraintItem = mConstraintItems[i];
        QJsonObject& constraint = constraintItem->data();
        
        if (!constraints_ids.contains(constraint.value(STATE_ID).toInt())) {
#ifdef DEBUG
            qDebug() << "EventsScene::updateScene Event Constraint deleted : " << constraint.value(STATE_ID).toInt();
#endif
            removeItem(constraintItem);
            mConstraintItems.removeOne(constraintItem);
            delete constraintItem;
        }
    }
    
    // ------------------------------------------------------
    //  Create / Update constraint items
    // ------------------------------------------------------

    if (displayProgress)
        progress->setLabelText(tr("Create / Update constraint items"));

    for (int i=0; i<constraints.size(); ++i) {
        QJsonObject constraint = constraints.at(i).toObject();
        if (displayProgress)
            progress->setValue(i);



        bool itemExists = false;
        for (int j=0; j<mConstraintItems.size(); ++j) {
            QJsonObject constraintItem = mConstraintItems.at(j)->data();
            if (constraintItem.value(STATE_ID).toInt() == constraint.value(STATE_ID).toInt()) {
                itemExists = true;
                if (constraint != constraintItem) {
                    // UPDATE ITEM
#ifdef DEBUG
                    qDebug() << "EventsScene::updateScene Constraint updated : id = " << constraint.value(STATE_ID).toInt();
#endif
                    mConstraintItems[j]->setData(constraint);
                }
            }
        }
        if (!itemExists) {
            // CREATE ITEM
            ArrowItem* constraintItem = new ArrowItem(this, ArrowItem::eEvent, constraint);
            mConstraintItems.append(constraintItem);
            addItem(constraintItem);
#ifdef DEBUG
            qDebug() << "EventsScene::updateScene Constraint created : id = " << constraint.value(STATE_ID).toInt();
#endif
        }
    }
    


    mUpdatingItems = false;

    EventItem* lastCurItem = currentEvent();
    QJsonObject currentEventLast =QJsonObject() ;
    if (lastCurItem)
            currentEventLast = lastCurItem->getEvent();

    if (currentEventLast != currentEventPrev)
        emit mProject->currentEventChanged(currentEventLast);
    // Deleting an item that was selected involves changing the selection (and updating properties view)
    // Nothing has been triggered so far because of the mUpdatingItems flag, so we need to trigger it now!
    // As well, creating an item changes the selection because we want the newly created item to be selected.
    if (hasDeleted || hasCreated) {
        updateStateSelectionFromItem();
    }
    
    adjustSceneRect();
    adaptItemsForZoom(mZoom);

   if (displayProgress) {
       delete progress;
       progress = 0;
   }

 #ifdef DEBUG
     QTime timeDiff(0,0,0,1);
     timeDiff = timeDiff.addMSecs(startTime.elapsed()).addMSecs(-1);

    qDebug()<<"EventsScene::updateScene() finish at " + timeDiff.toString("hh:mm:ss.zzz");
 #endif
   // update();
}

void EventsScene::clean()
{

    // ------------------------------------------------------
    //  Delete all items
    // ------------------------------------------------------
    for (int i=mItems.size()-1; i>=0; --i) {
        EventItem* eventItem = (EventItem*)mItems[i];
        QJsonObject& event = eventItem->getEvent();
        
        if (event.value(STATE_EVENT_TYPE).toInt() == Event::eDefault) {
            QList<QGraphicsItem*> dateItems = eventItem->childItems();
            for (int j=0; j<dateItems.size(); ++j) {
                removeItem(dateItems[j]);
                delete dateItems[j];
            }
        }
#ifdef DEBUG
        //qDebug() << "Event deleted : " << event[STATE_ID].toInt();
#endif
        mItems.removeAt(i);
        
        // This does not break the code but is commented to match PhasesScene implementation
        //removeItem(eventItem);
        
        eventItem->deleteLater();
    }
    
    // ------------------------------------------------------
    //  Delete all constraints
    // ------------------------------------------------------
    for (int i=mConstraintItems.size()-1; i>=0; --i) {
        ArrowItem* constraintItem = mConstraintItems[i];
        removeItem(constraintItem);
        mConstraintItems.removeOne(constraintItem);
        delete constraintItem;
    }
    
    mProject = 0;
    // ------------------------------------------------------
    //  Reset scene rect
    // ------------------------------------------------------
    //setSceneRect(QRectF(-100, -100, 200, 200));
    update(sceneRect());
}


/**
 * @brief EventsScene::updateStateSelectionFromItem look inside the scene which items (Widget) are selected and update the QJsonObject
 */
void EventsScene::updateStateSelectionFromItem()
{
    qDebug()<<"EventsScene::updateStateSelectionFromItem";
    if (!mUpdatingItems) {
        bool modified = false;
        EventItem* curItem = currentEvent();
        QJsonObject currentEvent = QJsonObject();

        for (int i=0; i<mItems.size(); ++i) {
            EventItem* item = static_cast<EventItem*>(mItems.at(i));
            
            // without selected update
            const QJsonObject prevEvent = item->getEvent();

            const bool selected = (item->isSelected() || item->withSelectedDate() );
            const bool isCurrent = (curItem == item);
            // update mData in AbtractItem
            item->setSelectedInData(selected);
            item->setCurrentInData(isCurrent);
            const QJsonObject nextEvent = item->getEvent();

            if (nextEvent != prevEvent)
                modified = true;

            if (isCurrent)
                currentEvent = curItem->getEvent();
#ifdef DEBUG
            if (modified)
                qDebug()<<"EventsScene::updateStateSelectionFromItem "<<nextEvent.value(STATE_NAME).toString()<<selected<<isCurrent;
#endif
         }
       
       
        if (modified ) {
           sendUpdateProject(tr("events selection : no undo, no view update!"), true, false);//  bool notify = true, bool storeUndoCommand = false
            // refresh the show and hide Event in the phases Scenes
            if (selectedItems().size() == 0)
                emit noSelection();
            else
                emit eventsAreSelected();

            // refresh the Event propreties view
            //if (selectedItems().size() == 1)
           if (selectedItems().size() >= 1)
                emit mProject->currentEventChanged(currentEvent);
            else {
                    QJsonObject itemEmpty;
                    emit mProject->currentEventChanged( itemEmpty);
            }

        }
    }
}

/* keep in memory
void EventsScene::updateSelectedEventsFromPhases()
{
    // Do not send "selection updated" each time an item is selected in this function!
    // Do it all at once at the end.
    mUpdatingItems = true;

    QJsonObject state = mProject->state();
    QJsonArray phases = state.value(STATE_PHASES).toArray();
    
    for (int i=0; i<mItems.size(); ++i) {
        EventItem* item = static_cast<EventItem*>(mItems.at(i));
        bool mustBeSelected = false;
        QList<int> eventPhasesIds = stringListToIntList(item->mData.value(STATE_EVENT_PHASE_IDS).toString());
        
        for (int i=0; i<phases.size(); ++i) {
            const QJsonObject phase = phases.at(i).toObject();
            const int phaseId = phase.value(STATE_ID).toInt();
            
            if (eventPhasesIds.contains(phaseId) && phase.value(STATE_IS_CURRENT).toBool())
               mustBeSelected = true;

        }
        item->setSelected(mustBeSelected);

        QList<QGraphicsItem*> dateList = item->childItems();
        for (int i=0; i<dateList.size(); ++i)
            dateList.at(i)->setSelected(mustBeSelected);


    }
    mUpdatingItems = false;

    // update scene without call updateSelection,
    for (int i=0; i<mItems.size(); ++i) {
        EventItem* item = static_cast<EventItem*>(mItems.at(i));

        bool selected = item->isSelected() ;

        QJsonObject& event = item->getEvent();

        if (event.value(STATE_IS_SELECTED).toBool() != selected ) {
            event[STATE_IS_SELECTED] = selected;
           // modified = true;
        }


     }
    sendUpdateProject(tr("events selection : no undo, no view update!"), false, false);


    //updateSelection();
    //updateScene();
}
*/
void EventsScene::adaptItemsForZoom(const double prop)
{
    mZoom = prop;
    for (int i=0; i<mItems.size(); ++i) {
        EventItem* item = dynamic_cast<EventItem*>(mItems[i]);
        if (item)
                item->setDatesVisible(mZoom > 0.6);
    }
}

void EventsScene::centerOnEvent(int eventId)
{
    for (int i = 0; i<mItems.size(); ++i) {
        EventItem* item = dynamic_cast<EventItem*>(mItems[i]);
        if (item) {
            QJsonObject& event = item->getEvent();
            if ((event.value(STATE_ID).toInt() == eventId) && (views().size() > 0)) {
                views()[0]->centerOn(item);
                clearSelection();
                item->setSelected(true);
                break;
            }
        }
    }
}

AbstractItem* EventsScene::currentItem()
{
    QList<QGraphicsItem*> selItems = selectedItems();
    if (!selItems.isEmpty()) {
        AbstractItem* absItem = dynamic_cast<AbstractItem*>(selItems.first());

#ifdef DEBUG
/*
        EventItem* evtItem = dynamic_cast<EventItem*>(absItem);
        if (evtItem)
            qDebug() << "EventsScene::currentItem() selected items is Event " << dynamic_cast<EventItem*>(evtItem)->getEvent().value("name");
        else if (dynamic_cast<DateItem*>(absItem))
                qDebug() << "EventsScene::currentItem() selected items is date : " << dynamic_cast<DateItem*>(absItem)->mDate.value("name");
        else  qDebug() << "EventsScene::currentItem() selected items : " << absItem;
*/
#endif
        return absItem;
    } else
        return nullptr;
   
   
}

EventItem* EventsScene::currentEvent() const
{
    QList<QGraphicsItem*> selItems = selectedItems();
    EventItem* cEvt = nullptr;
    if (selItems.size() == 0)
        return cEvt;

    QGraphicsItem* it = nullptr;
    foreach (it , selItems) {
        EventItem* tmpItem = dynamic_cast< EventItem*>(it);
        if (tmpItem)
            return tmpItem;
    }
    if (cEvt) {
#ifdef DEBUG
        QJsonObject& e = cEvt->getEvent();
        qDebug() << "EventsScene::currentEvent() selected Event : " << e.value("name");
#endif
        return cEvt;
    } else {
            foreach (it , selItems) {
                DateItem* tmpItem = dynamic_cast<DateItem*>(it);
                if (tmpItem) {
                    cEvt = dynamic_cast< EventItem*>(tmpItem->parentItem());
#ifdef DEBUG
                    QJsonObject& e = cEvt->getEvent();
                    qDebug() << "EventsScene::currentEvent() selected Event from date : " << e.value("name");
#endif
                    return cEvt;
                }
            }

            return nullptr;
    }
    
}


AbstractItem* EventsScene::collidingItem(QGraphicsItem* item)
{
    for (int i=0; i<mItems.size(); ++i) {
        bool isBound = (dynamic_cast<EventKnownItem*>(mItems.at(i)) != 0);
        if (item != mItems.at(i) && !isBound && item->collidesWithItem(mItems.at(i)))
            return mItems[i];
    }
    return 0;
}


void EventsScene::dateMoved(DateItem* dateItem, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    if (dateItem) {
        qDebug()<<"EventsScene::dateMoved";
        static EventItem* lastEntered = 0;
        EventItem* hoveredEventItem = dynamic_cast<EventItem*>(collidingItem(dateItem));
        if (!hoveredEventItem && lastEntered )
            lastEntered->setMergeable(false);

        else if (hoveredEventItem && (hoveredEventItem != lastEntered) ) {
            if (lastEntered)
                lastEntered->setMergeable(false);
            lastEntered = hoveredEventItem;

             EventItem* prevEventItem =dynamic_cast<EventItem*>(dateItem->parentItem());
            if (hoveredEventItem && prevEventItem && (hoveredEventItem != prevEventItem) ) {
                const QJsonObject& event = hoveredEventItem->getEvent();
                
                if (event.value(STATE_EVENT_TYPE).toInt() == Event::eDefault)
                    hoveredEventItem->setMergeable(true);
            }
        }
    }
}

void EventsScene::dateReleased(DateItem* dateItem, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    if (dateItem) {
        qDebug()<<"EventsScene::dateReleased";
        EventItem* hoveredEventItem = dynamic_cast<EventItem*>(collidingItem(dateItem));
        EventItem* prevEventItem = dynamic_cast<EventItem*>(dateItem->parentItem());
        if (!hoveredEventItem || !prevEventItem) {
            mDatesAnim->setItem(dateItem);
            mDatesAnim->setPosAt(0, dateItem->pos());
            mDatesAnim->setPosAt(1, dateItem->mOriginalPos);
            mDatesAnimTimer->start();

        }
        else if (hoveredEventItem && prevEventItem && (hoveredEventItem != prevEventItem)) {
            QJsonObject prevEvent = prevEventItem->getEvent();
            QJsonObject nextEvent = hoveredEventItem->getEvent();
            QJsonObject dateToRemove = dateItem->date();
            QJsonObject dateToAdd = dateItem->date();

            if (nextEvent.value(STATE_EVENT_TYPE).toInt() == Event::eDefault) {
                // Move the date to another event :
                
                Project* project = MainWindow::getInstance()->getProject();
                QJsonObject state = project->state();
                QJsonArray events = state.value(STATE_EVENTS).toArray();
                bool isRemove = false;
                bool isAdd = false;
                for (int i=0; !(isRemove && isAdd) && (i<events.size()) ; ++i) {
                    QJsonObject event = events.at(i).toObject();
                    
                    // remove dateToRemove from previous event :
                    if (event.value(STATE_ID).toInt() == prevEvent.value(STATE_ID).toInt()){
                        QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();
                        for (int j=0; j<dates.size(); ++j) {
                            
                            if (dates.at(j).toObject() == dateToRemove) {
                                dates.removeAt(j);
                                isRemove=true;
                                break;
                            }
                        }
                        event[STATE_EVENT_DATES] = dates;
                    }
                    // add dateToAdd to next event :
                    else if (event.value(STATE_ID).toInt() == nextEvent.value(STATE_ID).toInt()) {
                        QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();
                        dateToAdd[STATE_ID] = project->getUnusedDateId(dates);
                        dates.append(dateToAdd);
                        event[STATE_EVENT_DATES] = dates;
                        isAdd = true;
                    }
                    events[i] = event;
                }
                state[STATE_EVENTS] = events;
                project->pushProjectState(state, tr("Date moved to event"), true);
                
                hoveredEventItem->setMergeable(false);
                return;
            }
        } else {
            mDatesAnim->setItem(dateItem);
            mDatesAnim->setPosAt(0, dateItem->pos());
            mDatesAnim->setPosAt(1, dateItem->mOriginalPos);
            mDatesAnimTimer->start();
        }
    }
}

/* ----------------------------------------------------------------------------------------
 *  Event Items Events
 *----------------------------------------------------------------------------------------*/

/**
 * @brief happen when the mouse come into a Event, it's an overwrite of AbstractScene::itemEntered
 * it is specific to the Event, we can click on a date
 */
void EventsScene::itemEntered(AbstractItem* item, QGraphicsSceneHoverEvent* e)
{
    Q_UNUSED(e);
    qDebug() << "EventsScene::itemEntered";
    // the difference with the AbstractScene is here we need the curentEvent, which can be a date selected
    EventItem* current = currentEvent();

    EventItem* eventClicked = dynamic_cast< EventItem*>(item);
    
    if (mDrawingArrow && current && eventClicked && (eventClicked != current)) {
        mTempArrow->setTo(item->pos().x(), item->pos().y());

        if (constraintAllowed(current, eventClicked)) {
            mTempArrow->setState(ArrowTmpItem::eAllowed);
            mTempArrow->setLocked(true);

        } else {
            mTempArrow->setState(ArrowTmpItem::eForbidden);
            mTempArrow->setLocked(false);
        }
    }
    
}

/**
 * @brief EventsScene::itemClicked
 * @param item ie anEvent or a Phase
 * @param e
 * @return true
 * Arrive with a click on item (ie an Event or Date ),
 * Becareful with the linux system Alt+click can't be detected,
 *  in this case the user must combine Alt+Shift+click to valided a constraint
 */
bool EventsScene::itemClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    qDebug() << "EventsScene::itemClicked";

    EventItem* eventClicked = dynamic_cast< EventItem*>(item);
    EventItem* current = currentEvent();

    // if mDrawingArrow is true, an Event is already selected and we can create a Constraint.
    if (eventClicked ) {
        if (current && (eventClicked != current)) {
            if (mDrawingArrow && constraintAllowed(current, eventClicked)) {
                    createConstraint(current, eventClicked);
                    mTempArrow->setVisible(false);
                    mDrawingArrow=false;
                    updateStateSelectionFromItem();
                    sendUpdateProject("Event constraint created", true, true);

              }
        } else {
            eventClicked->setSelected(true);
            updateStateSelectionFromItem();
            sendUpdateProject("Item selected", true, false);//  bool notify = true, bool storeUndoCommand = false
        }
        
        
    } else
        clearSelection();

    updateStateSelectionFromItem();
    
    return true;
}


void EventsScene::itemDoubleClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e)
{
    AbstractScene::itemDoubleClicked(item, e);
    emit eventDoubleClicked();
}

void EventsScene::constraintDoubleClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    Q_UNUSED(item);
}

void EventsScene::constraintClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    Q_UNUSED(item);
    
    QMessageBox message(QMessageBox::Question, tr("Delete constraint"), tr("Do you really want to delete this constraint?"), QMessageBox::Yes | QMessageBox::No, qApp->activeWindow(), Qt::Sheet);
    if (message.exec() == QMessageBox::Yes) {
        mProject->deleteEventConstraint(item->data().value(STATE_ID).toInt());
    }
    

}

/**
 * @brief EventsScene::keyPressEvent overwrtite AbstractScene::keyPressEvent
 * @param keyEvent
 */
void EventsScene::keyPressEvent(QKeyEvent* keyEvent)
{
 //   qDebug() << "EventsScene::keyPressEvent: " << keyEvent->modifiers() << keyEvent->key();

    if (keyEvent->isAutoRepeat())
        keyEvent->ignore();
    
   if (selectedItems().count() == 0) {
        qDebug() << "EventsScene::keyPressEvent No item selected";
        emit noSelection();
    } else {
       qDebug() << "EventsScene::keyPressEvent emit selectionChanged() ";
       emit selectionChanged();
    }

    if (keyEvent->key() == Qt::Key_Delete) {
        deleteSelectedItems();

    // spotting the  Alt key
    } else if (keyEvent->modifiers() == Qt::AltModifier && selectedItems().count()==1) {

        qDebug() << "EventsScene::keyPressEvent You Press: "<< "Qt::Key_Alt";
        mAltIsDown = true;

        // Control if an Event is still selected
        EventItem* curItem = currentEvent();
        
        if (curItem) {
            mDrawingArrow = true;
            mTempArrow->setVisible(true);
            mTempArrow->setFrom(curItem->pos().x(), curItem->pos().y());

        } else {
            mDrawingArrow = false;
            mTempArrow->setVisible(false);
            clearSelection();
        }
    } else if (keyEvent->key() == Qt::Key_Shift)
        mShiftIsDown = true;
  /*  else if (keyEvent->modifiers() == Qt::ControlModifier)  {
        // on MacOs it is the the key cmd = command

 //if (QApplication::keyboardModifiers().testFlag(Qt::ControlModifier) == true) {
                    qDebug() << "EventsScene::keyPressEvent You Press: "<< "Qt::ControlModifier";

                }*/

   /* else if (keyEvent->modifiers() && Qt::ControlModifier)  {
                mDrawingArrow = false;
                mTempArrow->setVisible(false);
                qDebug() << "EventsScene::keyPressEvent You Press: "<< "Qt::ControlModifier";

            }*/

   // else
     //   keyEvent->ignore();


    
}

void EventsScene::keyReleaseEvent(QKeyEvent* keyEvent)
{   
    if (keyEvent->key() == Qt::Key_Alt) {
        qDebug() << "EventsScene::keyReleaseEvent You Released: "<<"Qt::Key_Alt";
        mDrawingArrow = false;
        mAltIsDown = false;
        //mShiftIsDown = false;
        mTempArrow->setState(ArrowTmpItem::eNormal);
        mTempArrow->setVisible(false);
        QGraphicsScene::keyReleaseEvent(keyEvent);
    }
    if (keyEvent->key() == Qt::Key_Shift) {
        qDebug() << "EventsScene::keyReleaseEvent You Released: "<<"Qt::Key_Shift";
        mDrawingArrow = false;
        //mAltIsDown = false;
        mShiftIsDown = false;
        mTempArrow->setVisible(false);
        QGraphicsScene::keyReleaseEvent(keyEvent);
    }
    
}


// -----------------------------------------------------------
//  The following function are about drag & drop
// -----------------------------------------------------------
//#pragma mark Drag & Drop
void EventsScene::dragMoveEvent(QGraphicsSceneDragDropEvent* e)
{
    for (int i=0; i<mItems.size(); ++i) {
        QRectF r = mItems.at(i)->boundingRect();
        r.translate(mItems.at(i)->scenePos());
        if (r.contains(e->scenePos())) {
            QGraphicsScene::dragMoveEvent(e);
            return;
        }
    }
    e->accept();
}

void EventsScene::dropEvent(QGraphicsSceneDragDropEvent* e)
{
    // ------------------------------------------------------------------
    //  Check if data have been dropped on an existing event.
    //  If so, QGraphicsScene::dropEvent(e) will pass the event to the corresponding item
    // ------------------------------------------------------------------
    for (int i=0; i<mItems.size(); ++i) {
        QRectF r = mItems.at(i)->boundingRect();
        r.translate(mItems.at(i)->scenePos());
        if (r.contains(e->scenePos())) {
            QGraphicsScene::dropEvent(e);
            return;
        }
    }
    
    // ------------------------------------------------------------------
    //  The data have been dropped on the scene background,
    //  so create one event per data!
    // ------------------------------------------------------------------
    e->accept();
    
    Project* project = MainWindow::getInstance()->getProject();
    QJsonObject state = project->state();
    QJsonArray events = state.value(STATE_EVENTS).toArray();
    
    QList<Date> dates = decodeDataDrop(e);
    
    // Create one event per data
    const int deltaY = 100;
    for (int i=0; i<dates.size(); ++i) {
        Event event;
        event.mName = dates.at(i).mName;
        event.mId = project->getUnusedEventId(events);
        dates[i].mId = 0;
        event.mDates.append(dates.at(i));
        event.mItemX = e->scenePos().x();
        event.mItemY = e->scenePos().y() + i * deltaY;
        event.mColor = randomColor();
        events.append(event.toJson());
    }
    state[STATE_EVENTS] = events;
    project->pushProjectState(state, tr("Dates added (CSV drag)"), true);
}

QList<Date> EventsScene::decodeDataDrop(QGraphicsSceneDragDropEvent* e)
{
    const QMimeData* mimeData = e->mimeData();

    QByteArray encodedData = mimeData->data("application/chronomodel.import.data");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    
   // QList<QStringList> failed;
    QList<int> acceptedRows;
    QList<int> rejectedRows;
    QList<Date> dates;

    while (!stream.atEnd()) {
        QString itemStr;
        stream >> itemStr;
        const AppSettings settings = MainWindow::getInstance()->getAppSettings();
        const QString csvSep = settings.mCSVCellSeparator;
        QStringList dataStr = itemStr.split(csvSep);
        
        // Remove first column corresponding to csvRow
        const int csvRow = dataStr.takeFirst().toInt();
        
        const QLocale csvLocal = settings.mCSVDecSeparator == "." ? QLocale::English : QLocale::French;
        Date date = Date::fromCSV(dataStr, csvLocal);

        if (!date.isNull()) {
            dates << date;
            acceptedRows.append(csvRow);
            
        } else {
            //failed.append(dataStr);
            rejectedRows.append(csvRow);
        }
    }
    emit csvDataLineDropAccepted(acceptedRows); //connected to slot ImportDataView::removeCsvRows
    emit csvDataLineDropRejected(rejectedRows);

    return dates;
}

