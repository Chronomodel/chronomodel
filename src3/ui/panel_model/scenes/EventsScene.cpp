/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#include "EventsScene.h"
#include "Event.h"
#include "Bound.h"
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
#include "PluginManager.h"

#include <QtWidgets>
#include <QProgressDialog>
#include <QMessageBox>


EventsScene::EventsScene(QGraphicsView* view, QObject* parent):AbstractScene(view, parent)
{
    mHelpView = new HelpWidget(view);
    mHelpView->setLink("https://chronomodel.com/storage/medias/59_manuel_release_2_0_version_1_04_03_2019.pdf#page=9");
    mHelpTimer = new QTimer(this);

    connect(this, &QGraphicsScene::selectionChanged, this, &EventsScene::updateStateSelectionFromItem);
    connect(mHelpTimer, SIGNAL(timeout()), this, SLOT(updateHelp()));
    connect(this, &EventsScene::eventsAreModified, this, &EventsScene::sendUpdateProject);

    mHelpTimer->start(200);
}

EventsScene::~EventsScene()
{

}

EventItem* EventsScene::findEventItemWithJsonId(const int id)
{
    foreach(AbstractItem* it, mItems) {
        EventItem* ev = static_cast<EventItem*>(it);
        const QJsonObject evJson = ev->getData();
        if (evJson.value(STATE_ID)== id)
            return ev;
    }
    return nullptr;
}


/* Actions
 */
/**
 * @brief EventsScene::deleteSelectedItems virtual implementation of AbstractScene
 */
void EventsScene::deleteSelectedItems()
{
   qDebug()<<"EventsScene::deleteSelectedItems";
   mProject->deleteSelectedEvents();
   emit noSelection();
}

bool EventsScene::constraintAllowed(AbstractItem* itemFrom, AbstractItem* itemTo)
{
    const QJsonArray constraints = mProject->mState.value(STATE_EVENTS_CONSTRAINTS).toArray();
    const QJsonObject eventFrom = dynamic_cast<EventItem*>(itemFrom)->getData();
    const QJsonObject eventTo = dynamic_cast<EventItem*>(itemTo)->getData();

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
    const QJsonObject eventFrom = dynamic_cast<EventItem*>(itemFrom)->getData();
    const QJsonObject eventTo = dynamic_cast<EventItem*>(itemTo)->getData();
    qDebug()<<"EventsScene::createConstraint"<<eventFrom.value(STATE_NAME)<<eventTo.value(STATE_NAME);

    mProject->createEventConstraint(eventFrom.value(STATE_ID).toInt(),
                                    eventTo.value(STATE_ID).toInt());

}

void EventsScene::mergeItems(AbstractItem* itemFrom, AbstractItem* itemTo)
{
    QJsonObject& eventFrom = dynamic_cast<EventItem*>(itemFrom)->getData();
    QJsonObject& eventTo = dynamic_cast<EventItem*>(itemTo)->getData();

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
        mHelpView->setLink("https://chronomodel.com/storage/medias/59_manuel_release_2_0_version_1_04_03_2019.pdf#page=18"); // chapter
    }
    else if (selected.count() == 0) {
        text = tr("Select an event or a bound by clicking on it.");
        if (mConstraintItems.size() != 0)
            text += tr("\nYou can also edit constraints by double clicking on the arrow");
        mHelpView->setLink("https://chronomodel.com/storage/medias/59_manuel_release_2_0_version_1_04_03_2019.pdf#page=38"); // Chapter
    } else if (selected.count() == 1) {
        const bool isBound = (dynamic_cast<EventKnownItem*>(selected[0]) != nullptr);

        if (mAltIsDown) {
            text = tr("Mouve your mouse and click on another element to create a constraint.");
                mHelpView->setLink("https://chronomodel.com/storage/medias/59_manuel_release_2_0_version_1_04_03_2019.pdf#page=38");
        } else if (mSelectKeyIsDown && !isBound) {
            text = tr("Drag the Data onto another Event to shift it.");
                mHelpView->setLink("https://chronomodel.com/storage/medias/59_manuel_release_2_0_version_1_04_03_2019.pdf#page=30");
        } else {
            text = tr("You have selected an element. You can now:\r- Edit its properties from the right panel.\r- Create a constraint by holding the \"Alt\" key down and clicking on another element.");
                mHelpView->setLink("https://chronomodel.com/storage/medias/59_manuel_release_2_0_version_1_04_03_2019.pdf#page=19");
            if (!isBound)
                text += tr("\r- Move its Data in another element by holding the \"Shift\" key down and dragging the selected element onto another one.\r- Delete it with the button on the left.");
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

    for (int i = 0; i < mItems.size(); ++i) {
         events.append(((EventItem*)mItems.at(i))->getData());
    }


    stateNext[STATE_EVENTS] = events;

 //   if (statePrev != stateNext) {

        qDebug()<<"EventsScene::sendUpdateProject stateChange";
        if (storeUndoCommand)
            mProject->pushProjectState(stateNext, reason, notify);
        //else
        mProject->sendUpdateState(stateNext, reason, notify);
 //   }
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
        for (const QJsonValue phase : phases) {
                if ((selectedPhase == false) && (phase.toObject().value(STATE_IS_SELECTED).toBool() == true)) {
                        selectedPhase = true;
                      //  qDebug()<<"EventsScene::setShowAllThumbs Phase Selected: "<<phase.toObject().value(STATE_NAME).toString();
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

             if ( (evTo && evFrom && (!evFrom->mGreyedOut || !evTo->mGreyedOut)) || show)
                 (*cIter)->setGreyedOut(false);
             else
                  (*cIter)->setGreyedOut(true);

    }
}


void EventsScene::createSceneFromState()
{

#ifdef DEBUG
    qDebug()<<"EventsScene::createSceneFromState()";
    QElapsedTimer startTime;
    startTime.start();
#endif

    const QJsonObject state = mProject->state();
    const QJsonArray eventsInState = state.value(STATE_EVENTS).toArray();
    const QJsonArray constraints = state.value(STATE_EVENTS_CONSTRAINTS).toArray();
    const QJsonObject settings = state.value(STATE_SETTINGS).toObject();

    const QJsonObject CurveSettings = state.value(STATE_CURVE).toObject();



     //http://doc.qt.io/qt-5/qprogressdialog.html#minimumDuration-prop

    QProgressDialog* progress = new QProgressDialog("Create event items","Wait" , 1, eventsInState.size());//,qApp->activeWindow(), Qt::Window);
    progress->setWindowModality(Qt::WindowModal);
    progress->setCancelButton(nullptr);

    progress->setMinimumWidth(int (progress->fontMetrics().horizontalAdvance(progress->labelText()) * 1.5));

    mSettings = ProjectSettings::fromJson(settings);
    mCurveSettings = CurveSettings::fromJson(CurveSettings);

    mUpdatingItems = true;

    // ------------------------------------------------------
    //  Delete all items
    // ------------------------------------------------------
    if (mItems.size()>0) {
        clearSelection();
        clear();
        mItems.clear();
     }
    // this item is delete with clear, but we need it.
    mTempArrow = new ArrowTmpItem();
    addItem(mTempArrow);
    mTempArrow->setVisible(false);



    // ------------------------------------------------------
    //  Create event items
    // ------------------------------------------------------
    int i = 0;

    progress->setLabelText(tr("Create event items"));

    for (QJsonArray::const_iterator citer = eventsInState.constBegin(); citer != eventsInState.constEnd(); ++citer) {
        const QJsonObject event = (*citer).toObject();
        ++i;

        progress->setValue(i);

       // CREATE ITEM
        Event::Type type = Event::Type (event.value(STATE_EVENT_TYPE).toInt());
        EventItem* newItem;
        if (type == Event::eDefault)
            newItem = new EventItem(this, event, settings);
        else //if(type == Event::eBound)
            newItem = new EventKnownItem(this, event, settings);

        mItems.append(newItem);
        qDebug()<<"createSceneFromState Create mItems:"<<event.value(STATE_NAME).toString();
        addItem(newItem);

        }


    // ------------------------------------------------------
    //  Create constraint items
    // ------------------------------------------------------

    progress->setLabelText(tr("Create constraint items"));
    progress->setMaximum(constraints.size());
    for (int i = 0; i < constraints.size(); ++i) {
        const QJsonObject constraint = constraints.at(i).toObject();
        progress->setValue(i);
        ArrowItem* constraintItem = new ArrowItem(this, ArrowItem::eEvent, constraint);
        mConstraintItems.append(constraintItem);
        addItem(constraintItem);
    }

    mUpdatingItems = false;

    delete progress;
 #ifdef DEBUG
    qDebug()<<tr("EventsScene::createScene() finish in %1").arg(QString(DHMS(startTime.elapsed())));
 #endif
}


/**
 * @brief EventsScene::updateSceneFromState , it is done after each Project::pushProjectState() ??
 */
void EventsScene::updateSceneFromState()
{

#ifdef DEBUG
qDebug()<<"EventsScene::updateSceneFromState()";
    QElapsedTimer startTime;
    startTime.start();
#endif

    if (mProject->mState.value(STATE_EVENTS).toArray().isEmpty() && mProject->mState.value(STATE_PHASES).toArray().isEmpty() && mItems.isEmpty())
        return;

    QJsonObject state = mProject->state();
    const QJsonArray eventsInNewState = state.value(STATE_EVENTS).toArray();
    const QJsonArray constraints = state.value(STATE_EVENTS_CONSTRAINTS).toArray();

    // case of newProject


    QJsonObject settings = state.value(STATE_SETTINGS).toObject();
    QJsonObject curveSettings = state.value(STATE_CURVE).toObject();

    QProgressDialog* progress = nullptr;
    bool displayProgress = false;

    EventItem* curItem = currentEvent();
    QJsonObject currentEventPrev = QJsonObject();
    if (curItem)
        currentEventPrev = curItem->getData();

    if (mItems.size() != eventsInNewState.size()) {
        //http://doc.qt.io/qt-5/qprogressdialog.html#minimumDuration-prop
        displayProgress = true;
        progress = new QProgressDialog("Create / Update event items","Wait" , 1, eventsInNewState.size());//,qApp->activeWindow(), Qt::Window);
        progress->setWindowModality(Qt::WindowModal);
        progress->setCancelButton(nullptr);

        progress->setMinimumWidth(int (progress->fontMetrics().boundingRect(progress->labelText()).width()  * 1.5));
    }

    QList<int> events_ids_inNewState;

    for (auto&& ev: eventsInNewState)
        events_ids_inNewState << ev.toObject().value(STATE_ID).toInt();

    QList<int> constraints_ids;
    for (auto&& constraint : constraints)
        constraints_ids << constraint.toObject().value(STATE_ID).toInt();

    // ------------------------------------------------------
    //  Update settings
    // ------------------------------------------------------
    bool settingsChanged = false;

    ProjectSettings s = ProjectSettings::fromJson(settings);
    if (mSettings != s) {
        settingsChanged = true;
        mSettings = s;
    }

    CurveSettings cs = CurveSettings::fromJson(curveSettings);
    if (mCurveSettings != cs) {
        settingsChanged = true;
        mCurveSettings = cs;
    }

    mUpdatingItems = true;


    // ------------------------------------------------------
    //  Find items not in current state
    QList<int> indexItemToRemove;
   // bool hasDeleted = false;

    for (int i = 0; i < mItems.size(); ++i) {
        QJsonObject& event = mItems[i]->mData;

        if (!events_ids_inNewState.contains(event.value(STATE_ID).toInt())) {
                indexItemToRemove.append(i);        
        }
    }

    // ------------------------------------------------------
    //  Delete EventItems not in current state
    // ------------------------------------------------------
    bool hasDeleted = false;
    for (int i = indexItemToRemove.size()-1; i >= 0; --i) {
        QJsonObject& event = mItems[indexItemToRemove.at(i)]->mData;
        Event::Type type = Event::Type (event.value(STATE_EVENT_TYPE).toInt());

        if (!events_ids_inNewState.contains(event.value(STATE_ID).toInt())) {

            if (type == Event::eDefault) {
                 EventItem* eventItem = (EventItem*)mItems[indexItemToRemove.at(i)];
                QList<QGraphicsItem*> dateItems = eventItem->childItems();

                int dateItemsSize = dateItems.size() -1;
                for (int j = dateItemsSize; j >= 0; --j) {
                    removeItem(dateItems.at(j));
                    delete dateItems[j];
                }
                delete eventItem;
            } else if (type == Event::eBound) {
                EventKnownItem* eventItem = (EventKnownItem*)mItems[indexItemToRemove.at(i)];

               delete eventItem;
           }
            //qDebug() << "EventsScene::updateScene Event deleted : " << event.value(STATE_ID).toInt();

            mItems.removeAt(indexItemToRemove.at(i));
            hasDeleted = true;

            // This does not break the code but is commented to match PhasesScene implementation
            //removeItem(eventItem);

           // eventItem->deleteLater();

        }
    }

    bool hasCreated = false;

    // ------------------------------------------------------
    //  Create / Update event items
    // ------------------------------------------------------
    int i = 0;
    if (displayProgress) {
        progress->setLabelText(tr("Create / Update event items"));
    }

    for (auto&& citer : eventsInNewState) {
        const QJsonObject event = citer.toObject();
 //       qDebug()<<"updateSceneFromState event:"<<event.value(STATE_NAME).toString()<<event.value(STATE_ID).toInt();
        ++i;
        if (displayProgress)
            progress->setValue(i);

        bool itemUnkown = true;

        for (auto&& cIterOld : mItems) {
            EventItem* oldItem = (EventItem*) cIterOld;
            const QJsonObject OldItemEvent = oldItem->getData();
//qDebug()<<"updateSceneFromState à comparer avec OldItemEvent:"<<OldItemEvent.value(STATE_NAME).toString()<< OldItemEvent.value(STATE_ID).toInt();

            if ( OldItemEvent.value(STATE_ID).toInt() == event.value(STATE_ID).toInt()) {

                itemUnkown = false;
                if ((event != OldItemEvent) || settingsChanged) {
                    // UPDATE ITEM
                 //   qDebug() << "EventsScene::updateScene Event changed : id = " << event.value(STATE_ID).toInt()<< event.value(STATE_NAME).toString();

                    oldItem->setEvent(event, settings);
                }
             }

            oldItem = nullptr;
        }
        if (itemUnkown) {
            // CREATE ITEM
                Event::Type type = Event::Type (event.value(STATE_EVENT_TYPE).toInt());
                EventItem* newItem = nullptr;

                if (type == Event::eDefault)
                    newItem = new EventItem(this, event, settings);
                else //if(type == Event::eBound)
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
 #ifdef Q_OS_MAC
                        newItem->setPos(pt.x() + arc4random() % posDelta - posDelta/2,
                                          pt.y() + arc4random() % posDelta - posDelta/2);
 #else
                        newItem->setPos(pt.x() + rand() % posDelta - posDelta/2,
                                          pt.y() + rand() % posDelta - posDelta/2);

 #endif
                    }

                newItem = nullptr;

                //qDebug() << "EventsScene::updateScene Event created : id = " << event.value(STATE_ID).toInt() <<event[STATE_ITEM_X].toDouble()<<event[STATE_ITEM_Y].toDouble();//<< event.value(STATE_NAME).toString()<<", type : " << type;

            }

        }

    }

    // ------------------------------------------------------
    //  Delete constraints not in current state
    // ------------------------------------------------------
    for (int i = mConstraintItems.size()-1; i >= 0; --i) {
        ArrowItem* constraintItem = mConstraintItems[i];
        QJsonObject& constraint = constraintItem->data();

        if (!constraints_ids.contains(constraint.value(STATE_ID).toInt())) {
#ifdef DEBUG
            //qDebug() << "EventsScene::updateScene Event Constraint deleted : " << constraint.value(STATE_ID).toInt();
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

    for (int i = 0; i < constraints.size(); ++i) {
        QJsonObject constraint = constraints.at(i).toObject();
        if (displayProgress)
            progress->setValue(i);

        bool itemExists = false;
        for (int j = 0; j < mConstraintItems.size(); ++j) {
            QJsonObject constraintItem = mConstraintItems.at(j)->data();


            if (constraintItem.value(STATE_ID).toInt() == constraint.value(STATE_ID).toInt()) {

                // in case of UNDO command check and update position
                const int fromId = constraintItem.value(STATE_CONSTRAINT_BWD_ID).toInt();
                const int toId = constraintItem.value(STATE_CONSTRAINT_FWD_ID).toInt();
                EventItem* eventFrom = mConstraintItems.at(j)->findEventItemWithJsonId(fromId);
                EventItem* eventTo = mConstraintItems.at(j)->findEventItemWithJsonId(toId);
                // control if all event still exist
                if (eventFrom && eventTo)
                    mConstraintItems.at(j)->updatePosition();

                itemExists = true;
                if (constraint != constraintItem) {
                    // UPDATE ITEM
#ifdef DEBUG
                    //qDebug() << "EventsScene::updateScene Constraint updated : id = " << constraint.value(STATE_ID).toInt();
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
            //qDebug() << "EventsScene::updateScene Constraint created : id = " << constraint.value(STATE_ID).toInt();
#endif
        }
    }

    mUpdatingItems = false;

    EventItem* lastCurItem = currentEvent();
    QJsonObject currentEventLast =QJsonObject() ;
    if (lastCurItem)
            currentEventLast = lastCurItem->getData();

    if (currentEventLast != currentEventPrev)
        emit mProject->currentEventChanged(currentEventLast);

    // Deleting an item that was selected involves changing the selection (and updating properties view)
    // Nothing has been triggered so far because of the mUpdatingItems flag, so we need to trigger it now!
    // As well, creating an item changes the selection because we want the newly created item to be selected.
    if (hasDeleted || hasCreated)
        updateStateSelectionFromItem();

    adjustSceneRect();
    adaptItemsForZoom(mZoom);

   if (displayProgress)
       delete progress;


 #ifdef DEBUG
/*
     QTime timeDiff(0,0,0,1);
     timeDiff = timeDiff.addMSecs(startTime.elapsed()).addMSecs(-1);

    //qDebug()<<"EventsScene::updateScene() finish at " + timeDiff.toString("hh:mm:ss.zzz");
*/
 #endif
}

void EventsScene::clean()
{

    // ------------------------------------------------------
    //  Delete all items
    // ------------------------------------------------------
    const int itemsSize = mItems.size() - 1;
    for (int i = itemsSize; i >= 0; --i) {
        EventItem* eventItem = (EventItem*)mItems[i];

            QList<QGraphicsItem*> dateItems = eventItem->childItems();
            const int dateItemsSize = dateItems.size() -1;
            for (int j = dateItemsSize; j >= 0; --j) {
                delete dateItems.first(); // delete the object
                dateItems.removeFirst(); // remove the pointer
            }
  //      }
#ifdef DEBUG
        //qDebug() << "Event deleted : " << event[STATE_ID].toInt();
#endif
        mItems.removeAt(i);

        // This does not break the code but is commented to match PhasesScene implementation
        //removeItem(eventItem);
        eventItem->setVisible(false); // The item disappears and after it's deleted later
       // eventItem->deleteLater();
    }

    mItems.clear();
    // ------------------------------------------------------
    //  Delete all constraints
    // ------------------------------------------------------
    for (int i = mConstraintItems.size()-1; i >= 0; --i) {
        ArrowItem* constraintItem = mConstraintItems[i];
        removeItem(constraintItem);
        mConstraintItems.removeOne(constraintItem);
        delete constraintItem;
    }

    mProject = nullptr;
    // ------------------------------------------------------
    //  Reset scene rect
    // ------------------------------------------------------
    update(sceneRect());
}


/**
 * @brief EventsScene::updateStateSelectionFromItem look inside the scene which items (Widget) are selected and update the QJsonObject
 */
void EventsScene::updateStateSelectionFromItem()
{
    qDebug()<<"EventsScene::updateStateSelectionFromItem";
    int nbOfSelectedEvent = 0;
    QJsonArray updatedEvents = QJsonArray();

  //  selectedItems().clear();
    if (!mUpdatingItems) {
        bool modified = false;
        EventItem* curItem = currentEvent();
        QJsonObject currentEvent = QJsonObject();

        for (auto& pItem : mItems) {
            EventItem* item = static_cast<EventItem*>(pItem);

            // without selected update
            const QJsonObject prevEvent = item->getData();

            const bool selected = (item->isSelected() || item->withSelectedDate() );

            if (selected)
                ++nbOfSelectedEvent;

            const bool isCurrent = (curItem == item);
            // update mData in AbtractItem
            item->setSelectedInData(selected);
            item->setCurrentInData(isCurrent);
            const QJsonObject nextEvent = item->getData();

            if (nextEvent != prevEvent)
                modified = true;

            updatedEvents.append(std::move(nextEvent));

            if (isCurrent)
                currentEvent = curItem->getData();
#ifdef DEBUG
         //   if (modified)
         //       qDebug()<<"EventsScene::updateStateSelectionFromItem "<<nextEvent.value(STATE_NAME).toString()<<selected<<isCurrent;
#endif
         }

        if (modified ) {
            mProject->mState[STATE_EVENTS] = updatedEvents;
           sendUpdateProject(tr("events selection : no undo, no view update!"), false, false);//  bool notify = true, bool storeUndoCommand = false
            // refresh the show and hide Event in the phases Scenes
           if (nbOfSelectedEvent == 0)
                emit noSelection();
           else
                emit eventsAreSelected();

            // refresh the Event propreties view

           if (selectedItems().size() >= 1)
                emit mProject->currentEventChanged(currentEvent); // connect to EventPropertiesView::setEvent
            else {
                    QJsonObject itemEmpty;
                    emit mProject->currentEventChanged( itemEmpty);
            }

        }
    }
}

/* keep in mind
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
                item->setDatesVisible(mZoom > 0.3);
    }
}

void EventsScene::centerOnEvent(int eventId)
{
    for (int i = 0; i<mItems.size(); ++i) {
        EventItem* item = dynamic_cast<EventItem*>(mItems[i]);
        if (item) {
            QJsonObject& event = item->getData();
            if ((event.value(STATE_ID).toInt() == eventId) && (views().size() > 0)) {
                views()[0]->centerOn(item);
                clearSelection();
                selectedItems().append(item);
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
        QJsonObject& e = cEvt->getData();
        qDebug() << "EventsScene::currentEvent() selected Event : " << e.value("name");
#endif
        return cEvt;
    } else {
            foreach (it , selItems) {
                DateItem* tmpItem = dynamic_cast<DateItem*>(it);
                if (tmpItem) {
                    cEvt = dynamic_cast< EventItem*>(tmpItem->parentItem());
#ifdef DEBUG
                    QJsonObject& e = cEvt->getData();
                    qDebug() << "EventsScene::currentEvent() selected Event from date : " << e.value("name");
#endif
                    return cEvt;
                }
            }

            return nullptr;
    }

}


AbstractItem* EventsScene::collidingItem(const QGraphicsItem* item)
{
    //for (int i = 0; i < mItems.size(); ++i) {
    for (auto && it : mItems) {
        bool isBound = (dynamic_cast<EventKnownItem*>(it) != nullptr);
        if (item != it && !isBound && item->collidesWithItem(it))
            return it;
    }
    return nullptr;
}


void EventsScene::dateMoved(const DateItem* dateItem)
{
    Q_ASSERT(dateItem);

    qDebug()<<"EventsScene::dateMoved";
    static EventItem* lastEntered = nullptr;
    EventItem* hoveredEventItem = dynamic_cast<EventItem*>(collidingItem(dateItem));
    EventItem* prevEventItem =dynamic_cast<EventItem*>(dateItem->parentItem());

    if (hoveredEventItem && (hoveredEventItem != prevEventItem)) {
        const QJsonObject& event = hoveredEventItem->getData();
        // setMergeable() color the Event with a blue doted square
        if (event.value(STATE_EVENT_TYPE).toInt() == Event::eDefault)
            hoveredEventItem->setMergeable(true);
    }


    if (!hoveredEventItem && lastEntered )
        lastEntered->setMergeable(false);

    else if (hoveredEventItem && (hoveredEventItem != lastEntered) ) {
        if (lastEntered)
            lastEntered->setMergeable(false);
        lastEntered = hoveredEventItem;

        if (hoveredEventItem && prevEventItem && (hoveredEventItem != prevEventItem) ) {
            const QJsonObject& event = hoveredEventItem->getData();

            if (event.value(STATE_EVENT_TYPE).toInt() == Event::eDefault)
                hoveredEventItem->setMergeable(true);
        }
    }

}

EventItem* EventsScene::dateReleased(DateItem* dateItem)
{
    Q_ASSERT(dateItem);

    qDebug()<<"EventsScene::dateReleased";

    EventItem* hoveredEventItem = dynamic_cast<EventItem*>(collidingItem(dateItem));
    EventItem* prevEventItem = dynamic_cast<EventItem*>(dateItem->parentItem());


  if (hoveredEventItem && prevEventItem && (hoveredEventItem != prevEventItem)) {

        QJsonObject prevEvent = prevEventItem->getData();
        QJsonObject nextEvent = hoveredEventItem->getData();
        QJsonObject dateToRemove = dateItem->date();
        QJsonObject dateToAdd = dateItem->date();

        if (nextEvent.value(STATE_EVENT_TYPE).toInt() == Event::eDefault) {
            // Move the date to another event :
            //qDebug()<<"EventsScene::dateReleased MERGE";

            QJsonObject* state = mProject->state_ptr();
            QJsonArray events = state->value(STATE_EVENTS).toArray();
            bool isRemove (false);
            bool isAdd (false);
            /*
             *  we change the data inside the eventItems, and we change the parentItem of the dateItem
             * in the function DateItem::mouseReleaseEvent().
             * We can not do project::updateState because it rebuild all the scene, but in this case
             * the DateItem will be destroy and a new dateItem will be created. So when the code return in
             * DateItem::mouseReleaseEvent(), there is no valid DateItem.
             */

            dateItem->setParentItem(hoveredEventItem);

            for (int i(0); !(isRemove && isAdd) && (i<events.size()) ; ++i) {
                QJsonObject event = events.at(i).toObject();

                // remove dateToRemove from previous event :
                if (event.value(STATE_ID).toInt() == prevEvent.value(STATE_ID).toInt()) {
                    QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();
                    for (int j (0); j<dates.size(); ++j) {
                        qDebug()<<"dates j"<<dates.at(j)<<" "<<dateToRemove;
                        if (dates.at(j).toObject() == dateToRemove) {
                            dates.removeAt(j);
                            isRemove=true;
                            break;
                        }
                    }
                    event[STATE_EVENT_DATES] = dates;
                    // new code
                    prevEventItem->mData = event;
                    if (dates.size() != prevEventItem->childItems().size())
                        qDebug()<<"EventsScene::dateReleased() (dates.size() != prevEventItem->childItems().size())";

                    prevEventItem->redrawEvent();
                }
                // add dateToAdd to next event : and update mDate[STATE_ID] of the dateItem itself
                else if (event.value(STATE_ID).toInt() == nextEvent.value(STATE_ID).toInt()) {
                    QJsonArray dates = event.value(STATE_EVENT_DATES).toArray();
                    dateToAdd[STATE_ID] = mProject->getUnusedDateId(dates);
                    dateItem->mDate[STATE_ID] = dateToAdd[STATE_ID];
                    dates.append(dateToAdd);
                    event[STATE_EVENT_DATES] = dates;
                    // new code
                    hoveredEventItem->mData = event;

                    if (dates.size() != hoveredEventItem->childItems().size())
                        qDebug()<<"EventsScene::dateReleased() (dates.size() != hoveredEventItem->childItems().size())";
                    // redrawEvent is done in DateItem Release
                    isAdd = true;
                }

                events[i] = event;
            }
            (*state)[STATE_EVENTS] = events;

            hoveredEventItem->setMergeable(false);
            mProject->pushProjectState(*state, DATE_MOVE_TO_EVENT_REASON, true); // used to disable ResultsView
            return hoveredEventItem;

        } else
            return nullptr;

    } else
        return nullptr;


//qDebug()<<"EventsScene::dateReleased() "<<prevEventItem->mData.value(STATE_ITEM_X).toDouble()<<prevEventItem->mData.value(STATE_ITEM_Y).toDouble();
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
        if (current) {
            if (eventClicked != current) {
                // create constraint if possible
                if (mDrawingArrow && constraintAllowed(current, eventClicked)) {
                        createConstraint(current, eventClicked);
                        mTempArrow->setVisible(false);
                        mDrawingArrow=false;
                       // updateStateSelectionFromItem();
                        sendUpdateProject("Event constraint created", true, true);
                        return true;

                }

            }

        }

    }
    //updateStateSelectionFromItem(); // emit sendUpdateProject
   // sendUpdateProject("Item selected", true, false);//  bool notify = true, bool storeUndoCommand = false

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

    QMessageBox message(QMessageBox::Question, tr("Delete constraint"), tr("Do you really want to delete this constraint?"), QMessageBox::Yes | QMessageBox::No, qApp->activeWindow());
    if (message.exec() == QMessageBox::Yes)
        mProject->deleteEventConstraint(item->data().value(STATE_ID).toInt());
}

/**
 * @brief EventsScene::keyPressEvent overwrite AbstractScene::keyPressEvent
 * @param keyEvent
 */
void EventsScene::keyPressEvent(QKeyEvent* keyEvent)
{
    //qDebug() << "EventsScene::keyPressEvent: " << keyEvent->modifiers() << keyEvent->key();

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
    }
//#ifdef Q_OS_WIN
    else if (keyEvent->key() == Qt::Key_Shift)
        mSelectKeyIsDown = true;
//#endif
    /* REMARK
     * On macOS Qt::MetaModifier correspond to the keyboard key "Ctrl"
     * and      Qt::ControlModifier to the keyboard key "cmd"
     */
#ifdef Q_OS_MAC
    else if (keyEvent->modifiers() == Qt::ControlModifier)  {
        mSelectKeyIsDown = true;
        qDebug() << "EventsScene::keyPressEvent You Press: "<< "Qt::ControlModifier";

    }
    else if (keyEvent->modifiers() == Qt::MetaModifier)  {
        mSelectKeyIsDown = true;
        qDebug() << "EventsScene::keyPressEvent You Press: "<< "Qt::MetaModifier";

                }
    else if (keyEvent->key() == Qt::Key_Shift)  {
        mSelectKeyIsDown = true;
        qDebug() << "EventsScene::keyPressEvent You Press: "<< "Qt::Key_Shift";
    }

#endif
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
       // qDebug() << "EventsScene::keyReleaseEvent You Released: "<<"Qt::Key_Alt";
        mDrawingArrow = false;
        mAltIsDown = false;
        //mSelectKeyIsDown = false;
        mTempArrow->setState(ArrowTmpItem::eNormal);
        mTempArrow->setVisible(false);
        QGraphicsScene::keyReleaseEvent(keyEvent);
    }
#ifdef Q_OS_WIN
    else if (keyEvent->key() == Qt::Key_Shift) {
       // qDebug() << "EventsScene::keyReleaseEvent You Released: "<<"Qt::Key_Shift";
        mDrawingArrow = false;
        //mAltIsDown = false;
        mSelectKeyIsDown = false;
        mTempArrow->setVisible(false);
        QGraphicsScene::keyReleaseEvent(keyEvent);
    }
#endif
#ifdef Q_OS_MAC
   else if ((QApplication::keyboardModifiers() == Qt::ControlModifier)) {
       // qDebug() << "EventsScene::keyReleaseEvent You Released: "<<"Qt::ControlModifier";
        mDrawingArrow = false;
        //mAltIsDown = false;
        mSelectKeyIsDown = false;
        mTempArrow->setVisible(false);
        QGraphicsScene::keyReleaseEvent(keyEvent);
    }
#endif
}


/* -----------------------------------------------------------
 *  The following function are about drag & drop
 * ----------------------------------------------------------- */

// Drag & Drop
void EventsScene::dragMoveEvent(QGraphicsSceneDragDropEvent* e)
{
    for (auto&& it : mItems) {
        QRectF r = it->boundingRect();
        r.translate(it->scenePos());
        if (r.contains(e->scenePos())) {
            QGraphicsScene::dragMoveEvent(e);
            return;
        }
    }
    e->accept();
}

/**
 * @brief EventsScene::dropEvent Drag and drop from importDataView to the Events'scene. If the data is drop over an existing Event, it is inserting
 * Or if the data is drop over the scene the data is insert in a new Event or inside an event with the same name (with the same character, no UpperCase check)
 * @param e
 */
void EventsScene::dropEvent(QGraphicsSceneDragDropEvent* e)
{
    /* ------------------------------------------------------------------
     *  Check if data have been dropped on an existing event.
     *  If so, QGraphicsScene::dropEvent(e) will pass the event to the corresponding item
     * ------------------------------------------------------------------ */
    for (auto& it : mItems) {
        QRectF r = it->boundingRect();
        r.translate(it->scenePos());
        if (r.contains(e->scenePos())) {
            QGraphicsScene::dropEvent(e);
            return;
        }
    }

    /* ------------------------------------------------------------------
     *  The data have been dropped on the scene background,
     *  so create one event per data!
     * ------------------------------------------------------------------ */
    e->accept();


    QPair<QList<QPair<QString, Date>>, QList<QMap<QString, double>>> droppedData = decodeDataDrop(e);
    QList<QPair<QString, Date>> listEvent_Data = droppedData.first;
    QList<QMap<QString, double>> listCurveData = droppedData.second;

    Project* project = MainWindow::getInstance()->getProject();

    // Create one event per data
    int deltaX = 0;
    int deltaY = 200;
    int EventCount = 0;
    QJsonObject state = project->state();
    // if the list is long possibility to make an automatic strati link
    enum constraintType
    {
        eConstraintNone = 'N',
        eConstraintStrati = 'S',
        eConstraintInvStrati = 'C'
    };
    constraintType constraint = eConstraintNone;

    if (listEvent_Data.size()>4) {
        QMessageBox messageBox;
        messageBox.setWindowTitle(tr("Automatic Link"));
        messageBox.setText(tr("Do you want to automatically create a constraint link between the imported events ?"));
        QAbstractButton *stratiButton = messageBox.addButton(tr("In CSV table, the Youngest is on the top"), QMessageBox::YesRole);
        QAbstractButton *chronoButton = messageBox.addButton(tr("In CSV table, the Oldest is on the top"), QMessageBox::NoRole);
        messageBox.addButton(tr("&No Link"), QMessageBox::RejectRole);

        messageBox.exec();
        if (messageBox.clickedButton() == stratiButton) {
            constraint = eConstraintStrati;
            deltaX = 0;
            deltaY = 200;

        } else if (messageBox.clickedButton() == chronoButton) {
            constraint = eConstraintInvStrati;
            deltaX = 0;
            deltaY = 200;

        } else
            constraint = eConstraintNone;

    }

    QJsonObject previousEvent = QJsonObject();
    QJsonObject currentEvent;

    for (int i = 0; i < listEvent_Data.size(); ++i) {
        // We must regenerate the variables "state" and "events" after event or data inclusion
      //  QJsonObject state = project->state();
        QJsonArray events = state.value(STATE_EVENTS).toArray();

        // look for an existing event with the same Name
        QString eventName = listEvent_Data.at(i).first;
        Date date = listEvent_Data.at(i).second;

        int eventIdx  = -1;
        int j = 0;

        if (!eventName.contains("bound", Qt::CaseInsensitive)) {
            QJsonObject eventFinded;
            for (auto&& ev : events) {
                if (ev.toObject().value(STATE_NAME).toString() == eventName) {
                    eventIdx = j;
                    eventFinded = ev.toObject();
                    currentEvent = eventFinded;
                    break;
                }
                ++j;
            }


            if (eventIdx > -1) {
                QJsonObject dateJson = date.toJson();
                QJsonArray datesEvent = eventFinded.value(STATE_EVENT_DATES).toArray();
                dateJson[STATE_ID] = project->getUnusedDateId(datesEvent);
                if (dateJson.value(STATE_NAME).toString() == "")
                    dateJson[STATE_NAME] = "No Name " + QString::number(dateJson[STATE_ID].toInt());
                datesEvent.append(dateJson);
                eventFinded[STATE_EVENT_DATES] = datesEvent;

                if (i < listCurveData.count()) {
                    Event::setCurveCsvDataToJsonEvent(eventFinded, listCurveData.at(i));
                }

                events[eventIdx] =eventFinded;

                state[STATE_EVENTS] = events;

            } else {
                Event event;
                // eventName=="" must never happen because we set "No Name" in ImportDataView::browse()
                event.mName = (eventName=="" ? date.mName : eventName);
                event.mId = project->getUnusedEventId(events);


                date.mId = 0;
                if (date.mName == "")
                    date.mName = "No Name";

                event.mDates.append(date);
                event.mItemX = e->scenePos().x() + EventCount * deltaX;
                event.mItemY = e->scenePos().y() + EventCount * deltaY;
                event.mColor = randomColor();

                QJsonObject eventJson  (event.toJson());
                currentEvent = eventJson;
                if (i < listCurveData.count()) {
                    Event::setCurveCsvDataToJsonEvent(eventJson, listCurveData.at(i));
                }

                events.append(eventJson);
                state[STATE_EVENTS] = events;

                //project->pushProjectState(state, NEW_EVEN_BY_CSV_DRAG_REASON, true);
                ++EventCount;
            }

        } else {
            Bound bound;
            bound.mType = Event::eBound;
            bound.mTheta.mSamplerProposal = MHVariable::eFixe;
            bound.mFixed= date.mData[STATE_EVENT_KNOWN_FIXED].toDouble();
            // eventName=="" must never happen because we set "No Name" in ImportDataView::browse()
            bound.mName = ( !date.mName.isEmpty() ? date.mName : "No Name");
            bound.mId = project->getUnusedEventId(events);

            bound.mItemX = e->scenePos().x() + EventCount * deltaX;
            bound.mItemY = e->scenePos().y() + EventCount * deltaY;
            bound.mColor = randomColor();
            auto boundJson (bound.toJson());

            if (i < listCurveData.count()) {
                Event::setCurveCsvDataToJsonEvent(boundJson, listCurveData.at(i));
            }
            events.append(boundJson);
            currentEvent = boundJson;

            state[STATE_EVENTS] = events;
            project->pushProjectState(state, NEW_EVEN_BY_CSV_DRAG_REASON, true);
            ++EventCount;
        }

        // Automatic constraint creation

        if (constraint != eConstraintNone && !previousEvent.isEmpty()) {
            QJsonArray eventsConstraints = state.value(STATE_EVENTS_CONSTRAINTS).toArray();

            bool addConstraint = false;
            EventConstraint c;
            if (currentEvent.value(STATE_ID).toInt() != previousEvent.value(STATE_ID).toInt()) {
                if ( constraint == eConstraintStrati && mProject->isEventConstraintAllowed(currentEvent, previousEvent)) {
                       c.mFromId = currentEvent.value(STATE_ID).toInt();
                       c.mToId = previousEvent.value(STATE_ID).toInt();
                       addConstraint = true;

                } else if (mProject->isEventConstraintAllowed(previousEvent, currentEvent)) {
                        c.mFromId = currentEvent.value(STATE_ID).toInt();
                        c.mToId = previousEvent.value(STATE_ID).toInt();
                        addConstraint = true;
                }

                if (addConstraint) {
                    c.mId = mProject->getUnusedEventConstraintId(eventsConstraints);

                    QJsonObject constraint = c.toJson();
                    eventsConstraints.append(constraint);
                    state[STATE_EVENTS_CONSTRAINTS] = eventsConstraints;
                    project->pushProjectState(state, "Event constraint created", true);
                }
            }


        }
        previousEvent = currentEvent;
    } // for()
// TODO : Prévoir une répartition uniforme des Events en augmentant l'écart lorsqu'il y a plusieurs dates
    project->pushProjectState(state, NEW_EVEN_BY_CSV_DRAG_REASON, true);
}

QList<Date> EventsScene::decodeDataDrop_old(QGraphicsSceneDragDropEvent* e)
{
    const QMimeData* mimeData = e->mimeData();

    QByteArray encodedData = mimeData->data("application/chronomodel.import.data");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);

    QList<int> acceptedRows;
    QList<int> rejectedRows;
    QList<Date> dates;

    while (!stream.atEnd()) {
        QString itemStr;
        stream >> itemStr;

        const QString csvSep = AppSettings::mCSVCellSeparator;
        QStringList dataStr = itemStr.split(csvSep);

        // Remove first column corresponding to csvRow
        const int csvRow = dataStr.takeFirst().toInt();

        const QLocale csvLocal = AppSettings::mCSVDecSeparator == "." ? QLocale::English : QLocale::French;
        Date date = Date::fromCSV(dataStr, csvLocal);

        if (!date.isNull()) {
            dates << date;
            acceptedRows.append(csvRow);

        } else
            rejectedRows.append(csvRow);

    }
    emit csvDataLineDropAccepted(acceptedRows); //connected to slot ImportDataView::removeCsvRows
    emit csvDataLineDropRejected(rejectedRows);

    return dates;
}

/**
 * @brief EventsScene::decodeDataDrop insert Event when drop a CSV line from the table importCSV
 * the table may be come from ImportDataView::exportDates()
 * @param e
 * @example
 * Maximal number of columns used in ChronoModel;
1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16;17;18;19;20
Structure;Event 1;
// Event name;Dating method;dating name/code;Age;error;calibration curve;Reservoir R;delta R;"wiggle matching
""fixed""
""range""
""gaussian""";wiggle value 1;Wiggle value 2;;;;X_Inc_Depth;Err X- apha95- Err depth;Y_Declinaison;Err Y;Z_Field;Err Z_Err F;
Event name 1;14C;14C_Ly_5212;1370;50;intcal20.14c;0;0;none;;;;;;74;5;50;-10;2;;
Event name 2;14C;14C_Ly_5212;1370;50;intcal20.14c;0;0;gaussian;30;5;;;;;;;;;;
// Event name;methode;dating name/code;Age;error;"reference year
(for measurement)";;;;;;;;;prof;err prof;;;;;
Event name 3;TL/OSL;TL-CLER-202a;987;120;1990;;;;;;;;;220;3;;;;;
Event name 4;TL/OSL;TL-CLER-202b;1170;140;1990;;;;;;;;;;;;;;;
Event name 5;TL/OSL;TL-CLER-203;1280;170;1990;;;;;;;;;;;;;;;
// Event name;methode;dating name/code;measurement type;mean value;Inclination  value corresponding to declination;colonne inutile !;"std error
alpha95";Reference Curve;;;;;;;;;;;;
Event name 6;AM;kiln A;inclination;65;0;0;2,5;FranceInc;;;;;;;;;;;;
Event name 7;AM;kiln A;declination;-20;65;0;2,5;FranceDec;;;;;;;;;;;;
Event name 8;AM;kiln A;intensity;53;0;53;5;FranceInt;;;;;;;;;;;;
// Event name;methode;dating name/code;mean;error;calibration curve;param a;param b;param c;"wiggle matching
""fixed""
""range""
""gaussian""";wiggle value 1;Wiggle value 2;;;;;;;;;
Event name 9;GAUSS;date 1;1000;50;none;;;;;;;;;;;;;;;
Event name 10;GAUSS;date 1;1000;50;none;;;;;;;;;;;;;;;
Event name 11;GAUSS;date 1;1000;50;ReferenceCurveName;;;;;;;;;;;;;;;
Event name 12;GAUSS;date 2;1000;50;equation;0,01;-1;-1000;fixed;20;;;;;;;;;;
Event name 13;GAUSS;date 2;1000;50;equation;0,01;-1;-1000;range;10;15;;;;;;;;;
// Event name;methode;dating name/code;date t1;date t2;;;;;;;;;;;;;;;;
Event name 14;UNIF;date archéo ;300;500;;;;;;;;;;;;;;;;
 * @return
 */
QPair<QList<QPair<QString, Date>>, QList<QMap<QString, double>>> EventsScene::decodeDataDrop(QGraphicsSceneDragDropEvent* e)
{
    const QMimeData* mimeData = e->mimeData();

    QByteArray encodedData = mimeData->data("application/chronomodel.import.data");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);

    QList<int> acceptedRows;
    QList<int> rejectedRows;

    QList<QPair<QString, Date>> dates;
    QList<QMap<QString, double>> curveValues;

    while (!stream.atEnd()) {
        QString itemStr;
        stream >> itemStr;

        const QString csvSep = AppSettings::mCSVCellSeparator;
        QStringList dataStr = itemStr.split(csvSep);

        // Remove first column corresponding to csvRow
        const int csvRow = dataStr.takeFirst().toInt();
        const QString eventName = dataStr.takeFirst();

        const QLocale csvLocal = AppSettings::mCSVDecSeparator == "." ? QLocale::English : QLocale::French;
       // _____________

        const QString pluginName = dataStr.first();
        PluginAbstract* plugin = PluginManager::getPluginFromName(pluginName);
        Date date;


        if (pluginName.contains("bound", Qt::CaseInsensitive)) {
            QStringList dataTmp = dataStr.mid(1,dataStr.size()-1);
            date.mName = eventName;
            date.mPlugin = nullptr;
            date.mTi.mSamplerProposal = MHVariable::eMHSymetric; //set but not used

            QJsonObject json;
            json.insert(STATE_EVENT_KNOWN_FIXED, csvLocal.toDouble(dataTmp.at(0)));
            date.mData = json;
            date.mIsValid = true ;
            date.mUUID = QString::fromStdString(Generator::UUID());

            // We force the name of the Event in "Bound" to recognize then that it was a bound.
            dates << qMakePair("bound", date);
            acceptedRows.append(csvRow);

        } else if (plugin) {

           //  {
                date = Date::fromCSV(dataStr, csvLocal);
                dates << qMakePair(eventName, date);
                acceptedRows.append(csvRow);
          //  } else {
          //      return (QPair<QList<QPair<QString, Date>>, QList<QMap<QString, double>>>());
          //  }
        }
           /* QMap<QString, double> CurveValues;
            if (!date.isNull()) {
                dates << qMakePair(eventName, date);
                acceptedRows.append(csvRow);

                if (dataStr.size() >= 14) {
                    CurveValues.insert(STATE_EVENT_X_INC_DEPTH, csvLocal.toDouble(dataStr.at(13)));
                } else {
                    CurveValues.insert(STATE_EVENT_X_INC_DEPTH, 0);
                }
                if (dataStr.size() >= 15) {
                    CurveValues.insert(STATE_EVENT_SX_ALPHA95_SDEPTH, csvLocal.toDouble(dataStr.at(14)));
                } else {
                    CurveValues.insert(STATE_EVENT_SX_ALPHA95_SDEPTH, 0);
                }
                if (dataStr.size() >= 16) {
                    CurveValues.insert(STATE_EVENT_Y_DEC, csvLocal.toDouble(dataStr.at(15)));
                } else {
                    CurveValues.insert(STATE_EVENT_Y_DEC, 0);
                }
                if (dataStr.size() >= 17) {
                    CurveValues.insert(STATE_EVENT_SY, csvLocal.toDouble(dataStr.at(16)));
                } else {
                    CurveValues.insert(STATE_EVENT_SY, 0);
                }
                if (dataStr.size() >= 18) {
                    CurveValues.insert(STATE_EVENT_Z_F, csvLocal.toDouble(dataStr.at(17)));
                } else {
                    CurveValues.insert(STATE_EVENT_Z_F, 0);
                }

                if (dataStr.size() >= 19) {
                    CurveValues.insert(STATE_EVENT_SZ_SF, csvLocal.toDouble(dataStr.at(18)));
                } else {
                    CurveValues.insert(STATE_EVENT_SZ_SF, 0);
                }


                curveValues << CurveValues;
            } else {
               rejectedRows.append(csvRow);
            }*/

            QMap<QString, double> CurveValues;
            if (!date.mData.isEmpty()) {


                if (dataStr.size() >= 14) {
                    CurveValues.insert(STATE_EVENT_X_INC_DEPTH, csvLocal.toDouble(dataStr.at(13)));
                } else {
                    CurveValues.insert(STATE_EVENT_X_INC_DEPTH, 0);
                }
                if (dataStr.size() >= 15) {
                    CurveValues.insert(STATE_EVENT_SX_ALPHA95_SDEPTH, csvLocal.toDouble(dataStr.at(14)));
                } else {
                    CurveValues.insert(STATE_EVENT_SX_ALPHA95_SDEPTH, 0);
                }
                if (dataStr.size() >= 16) {
                    CurveValues.insert(STATE_EVENT_Y_DEC, csvLocal.toDouble(dataStr.at(15)));
                } else {
                    CurveValues.insert(STATE_EVENT_Y_DEC, 0);
                }
                if (dataStr.size() >= 17) {
                    CurveValues.insert(STATE_EVENT_SY, csvLocal.toDouble(dataStr.at(16)));
                } else {
                    CurveValues.insert(STATE_EVENT_SY, 0);
                }
                if (dataStr.size() >= 18) {
                    CurveValues.insert(STATE_EVENT_Z_F, csvLocal.toDouble(dataStr.at(17)));
                } else {
                    CurveValues.insert(STATE_EVENT_Z_F, 0);
                }

                if (dataStr.size() >= 19) {
                    CurveValues.insert(STATE_EVENT_SZ_SF, csvLocal.toDouble(dataStr.at(18)));
                } else {
                    CurveValues.insert(STATE_EVENT_SZ_SF, 0);
                }


                curveValues << CurveValues;

            } else {
                rejectedRows.append(csvRow);
            }


    }

    emit csvDataLineDropAccepted(acceptedRows); //connected to slot ImportDataView::removeCsvRows
    emit csvDataLineDropRejected(rejectedRows);

    return QPair<QList<QPair<QString, Date>>, QList<QMap<QString, double>>>(dates, curveValues);
}