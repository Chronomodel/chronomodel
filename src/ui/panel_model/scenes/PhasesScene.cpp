#include "PhasesScene.h"
#include "PhaseItem.h"
#include "ArrowItem.h"
#include "ArrowTmpItem.h"
#include "MainWindow.h"
#include "Project.h"
#include "QtUtilities.h"
#include <QtWidgets>


PhasesScene::PhasesScene(QGraphicsView* view, QObject* parent):AbstractScene(view, parent)
{
   connect(this, &QGraphicsScene::selectionChanged, this, &PhasesScene::updateStateSelectionFromItem);
}

PhasesScene::~PhasesScene()
{
    
}

/*
 *  Actions
 */
void PhasesScene::deleteSelectedItems()
{
    mProject->deleteSelectedPhases();
    emit noSelection();

}

bool PhasesScene::constraintAllowed(AbstractItem* itemFrom, AbstractItem* itemTo)
{
    QJsonArray constraints = mProject->mState.value(STATE_PHASES_CONSTRAINTS).toArray();
    
    QJsonObject phaseFrom = ((PhaseItem*)itemFrom)->getPhase();
    QJsonObject phaseTo   = ((PhaseItem*)itemTo)->getPhase();
    
    const int phaseFromId = phaseFrom.value(STATE_ID).toInt();
    const int phaseToId   = phaseTo.value(STATE_ID).toInt();
    
    bool ConstraintAllowed = true;
    
    for (int i = 0; i < constraints.size(); ++i) {
        QJsonObject constraint = constraints.at(i).toObject();
        // Prevent the double
        if ((constraint.value(STATE_CONSTRAINT_BWD_ID).toInt() == phaseFromId) && (constraint.value(STATE_CONSTRAINT_FWD_ID).toInt() == phaseToId)) {
            
            ConstraintAllowed = false;
            //qDebug() << "PhasesScene::constraintAllowed: not Allowed " ;
        }
        //Prevent inversion
        else if (constraint.value(STATE_CONSTRAINT_BWD_ID).toInt() == phaseToId && constraint.value(STATE_CONSTRAINT_FWD_ID).toInt() == phaseFromId) {
            
            ConstraintAllowed = false;
            //qDebug() << "PhasesScene::constraintAllowed: not Allowed Inversion" ;
        }
    }
    if ( ConstraintAllowed && constraintIsCircular(constraints, phaseFromId, phaseToId) ) {
        
        ConstraintAllowed = false;
        
#ifdef DEBUG
        qDebug() << "PhasesScene::constraintAllowed: not Allowed Circular" ;
#endif
    }
    return ConstraintAllowed;
    
    
}


void PhasesScene::createConstraint(AbstractItem* itemFrom, AbstractItem* itemTo)
{
    const QJsonObject phaseFrom = dynamic_cast<PhaseItem*>(itemFrom)->getPhase();
    const QJsonObject phaseTo = dynamic_cast<PhaseItem*>(itemTo)->getPhase();
    
    mProject->createPhaseConstraint(phaseFrom.value(STATE_ID).toInt(),
                                                        phaseTo.value(STATE_ID).toInt());
}

void PhasesScene::mergeItems(AbstractItem* itemFrom, AbstractItem* itemTo)
{
    QJsonObject phaseFrom = ((PhaseItem*)itemFrom)->getPhase();
    QJsonObject phaseTo = ((PhaseItem*)itemTo)->getPhase();
    
    mProject->mergePhases(phaseFrom.value(STATE_ID).toInt(),
                                              phaseTo.value(STATE_ID).toInt());
}

// SIGNALS from eventScene
void PhasesScene::noHide()
{
    setShowAllEvents(true);
}

void PhasesScene::eventsSelected()
{
   setShowAllEvents(false);
}

void PhasesScene::setShowAllEvents(const bool show)
{
    mShowAllThumbs = show;
    // update childItems
    update();
}

/*Project Update
 *
 */
void PhasesScene::sendUpdateProject(const QString& reason, bool notify, bool storeUndoCommand)
{
    qDebug()<<"PhasesScene::sendUpdateProject "<<reason<<notify<<storeUndoCommand;
    QJsonObject statePrev = mProject->state();
    QJsonObject stateNext = statePrev;
    
    QJsonArray phases = QJsonArray();
    for (int i=0; i<mItems.size(); ++i)
        phases.append(((PhaseItem*)mItems.at(i))->getPhase());

    stateNext[STATE_PHASES] = phases;

    if (statePrev != stateNext) {
        if (storeUndoCommand)
            mProject->pushProjectState(stateNext, reason, notify);
        else
            mProject->sendUpdateState(stateNext, reason, notify);
    }
}
 void PhasesScene::createSceneFromState()
 {
     qDebug()<<"PhasesScene::createSceneFromState()";

     const QJsonObject state = mProject->state();
     const QJsonArray phases = state.value(STATE_PHASES).toArray();
     const QJsonArray constraints = state.value(STATE_PHASES_CONSTRAINTS).toArray();

     // ------------------------------------------------------
     //  Delete items not in current state
     // ------------------------------------------------------
    clear();
    // this item is delete with clear, but we need it. this is herited from AbstracScene
    mTempArrow = new ArrowTmpItem();
    addItem(mTempArrow);
    mTempArrow->setVisible(false);
    mTempArrow->setZValue(0);

    clearSelection();
     // ------------------------------------------------------
     //  Create phase items
     // ------------------------------------------------------

    for (QJsonArray::const_iterator iPhase= phases.constBegin(); iPhase != phases.constEnd(); ++iPhase) {
             // CREATE ITEM
         PhaseItem* phaseItem = new PhaseItem(this, iPhase->toObject());
         mItems.append(phaseItem);
         addItem(phaseItem);

 #ifdef DEBUG
             //qDebug() << "Phase item created : id = " << phase[STATE_ID].toInt();
 #endif

     }


   // ------------------------------------------------------
     //  Create  constraint items
     // ------------------------------------------------------
     for (int i=0; i<constraints.size(); ++i) {
         QJsonObject constraint = constraints.at(i).toObject();

             // CREATE ITEM
             ArrowItem* constraintItem = new ArrowItem(this, ArrowItem::ePhase, constraint);
             mConstraintItems.append(constraintItem);
             addItem(constraintItem);
 #ifdef DEBUG
             //qDebug() << "Constraint created : id = " << constraint[STATE_ID].toInt();
 #endif

     }

     mUpdatingItems = false;

     adjustSceneRect();
     adaptItemsForZoom(mZoom);
 }


void PhasesScene::updateSceneFromState()
{
    qDebug()<<"PhasesScene::updateSceneFromState()";

    const QJsonObject state = mProject->state();
    QJsonArray phases = state.value(STATE_PHASES).toArray();
    QJsonArray constraints = state.value(STATE_PHASES_CONSTRAINTS).toArray();
    
    QList<int> phases_ids;
    for (int i=0; i<phases.size(); ++i)
        phases_ids << phases.at(i).toObject().value(STATE_ID).toInt();
    
    QList<int> constraints_ids;
    for (int i=0; i<constraints.size(); ++i)
        constraints_ids << constraints.at(i).toObject().value(STATE_ID).toInt();
    
    mUpdatingItems = true;
    
    // ------------------------------------------------------
    //  Delete items not in current state
    // ------------------------------------------------------
    bool hasDeleted = false;
    for (int i=mItems.size()-1; i>=0; --i) {
        PhaseItem* item = (PhaseItem*)mItems[i];
        QJsonObject& phase = item->getPhase();
        
        if (!phases_ids.contains(phase.value(STATE_ID).toInt())) {
#ifdef DEBUG
            //qDebug() << "=> Phase item deleted : " << phase[STATE_ID].toInt();
#endif
            mItems.removeAt(i);
            hasDeleted = true;
            
            // ????? This breaks the program!!! Delete abose does the jobs but is it safe?
            //removeItem(item);
            
            // This is a QObject : call deleteLater instead of delete
            item->deleteLater();
        }
    }
    
    // ------------------------------------------------------
    //  Create / Update phase items
    // ------------------------------------------------------
    bool hasCreated = false;
    for (int i=0; i<phases.size(); ++i) {
        QJsonObject phase = phases.at(i).toObject();
        
        bool itemExists = false;
        for (int j=0; j<mItems.size(); ++j) {
            PhaseItem* item = (PhaseItem*)mItems[j];
            QJsonObject itemPhase = item->getPhase();
            if (itemPhase.value(STATE_ID).toInt() == phase.value(STATE_ID).toInt()) {
                itemExists = true;
                
                // When assigning events to a phase by clicking on the checkbox of the phase item,
                // the information is saved in the event, not in the phase!
                // So apparently, the phase hasn't changed, but we need to redraw its item to show the modified events inside it.
                //if(phase != itemPhase)
                {
                    // UPDATE ITEM
#ifdef DEBUG
                    //qDebug() << "Phase item updated : id = " << phase[STATE_ID].toInt();
#endif
                    item->setPhase(phase);
                }
            }
        }
        if (!itemExists) {
            // CREATE ITEM
            PhaseItem* phaseItem = new PhaseItem(this, phase);
            mItems.append(phaseItem);
            addItem(phaseItem);
            
            // Pratique
          //  clearSelection();
          //  phaseItem->setSelected(true);
            
            // Note : setting an event in (0, 0) tells the scene that this item is new!
            // Thus the scene will move it randomly around the currently viewed center point.
            QPointF pos = phaseItem->pos();
            if (pos.isNull()) {
                QList<QGraphicsView*> gviews = views();
                if (gviews.size() > 0) {
                    QGraphicsView* gview = gviews[0];
                    QPointF pt = gview->mapToScene(gview->width()/2, gview->height()/2);
                    int posDelta = 100;
                    phaseItem->setPos(pt.x() + rand() % posDelta - posDelta/2,
                                      pt.y() + rand() % posDelta - posDelta/2);
                }
            }
            
            hasCreated = true;
#ifdef DEBUG
            //qDebug() << "Phase item created : id = " << phase[STATE_ID].toInt();
#endif
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
            //qDebug() << "Phase Constraint deleted : " << constraint[STATE_ID].toInt();
#endif
            removeItem(constraintItem);
            mConstraintItems.removeOne(constraintItem);
            delete constraintItem;
        }
    }
    
    // ------------------------------------------------------
    //  Create / Update constraint items
    // ------------------------------------------------------
    for (int i=0; i<constraints.size(); ++i) {
        QJsonObject constraint = constraints.at(i).toObject();
        
        bool itemExists = false;
        for (int j=0; j<mConstraintItems.size(); ++j) {
            QJsonObject constraintItem = mConstraintItems.at(j)->data();
            if (constraintItem.value(STATE_ID).toInt() == constraint.value(STATE_ID).toInt()) {
                itemExists = true;
                if (constraint != constraintItem) {
                    // UPDATE ITEM
#ifdef DEBUG
                    //qDebug() << "Constraint updated : id = " << constraint[STATE_ID].toInt();
#endif
                    mConstraintItems[j]->setData(constraint);
                }
            }
        }
        if (!itemExists) {
            // CREATE ITEM
            ArrowItem* constraintItem = new ArrowItem(this, ArrowItem::ePhase, constraint);
            mConstraintItems.append(constraintItem);
            addItem(constraintItem);
#ifdef DEBUG
            //qDebug() << "Constraint created : id = " << constraint[STATE_ID].toInt();
#endif
        }
    }
    
    mUpdatingItems = false;
    
    // Deleting an item that was selected involves changing the selection (and updating properties view)
    // Nothing has been triggered so far because of the mUpdatingItems flag, so we need to trigger it now!
    // As well, creating an item changes the selection because we want the newly created item to be selected.
    if (hasDeleted || hasCreated)
        updateStateSelectionFromItem();
    
    adjustSceneRect();
    adaptItemsForZoom(mZoom);

}

void PhasesScene::clean()
{

    // ------------------------------------------------------
    //  Delete all items
    // ------------------------------------------------------
    for (int i=mItems.size()-1; i>=0; --i) {
        PhaseItem* item = (PhaseItem*)mItems[i];
        mItems.removeAt(i);
        
        // ????? This breaks the program!!! Delete abose does the jobs but is it safe?
        //removeItem(item);
        
        // This is a QObject : call deleteLater instead of delete
        item->deleteLater();
    }
    
    // ------------------------------------------------------
    //  Delete all constraints
    // ------------------------------------------------------
    for (int i=mConstraintItems.size()-1; i>=0; --i) {
        ArrowItem* constraintItem = mConstraintItems[i];
#ifdef DEBUG
        QJsonObject& constraint = constraintItem->data();
        qDebug() << "Phase Constraint deleted : " << constraint[STATE_ID].toInt();
#endif
        removeItem(constraintItem);
        mConstraintItems.removeOne(constraintItem);
        delete constraintItem;
    }
    mProject = nullptr;
    update(sceneRect());
}


/* Selection & Current
 *
 */
void PhasesScene::updateStateSelectionFromItem()
{
    qDebug()<<"PhasesScene::updateStateSelectionFromItem";
    if (!mUpdatingItems) {
        bool modified = false;
        bool oneSelection = false;
        PhaseItem* curItem = dynamic_cast<PhaseItem*>(currentItem());

        for (int i=0; i<mItems.size(); ++i) {           
            PhaseItem* item = static_cast<PhaseItem*>(mItems.at(i));

            // without selected update
            const QJsonObject prevPhase = item->getPhase();

            const bool selected = item->isSelected();
            const bool isCurrent = (curItem == item);

            if (selected)
                oneSelection = true;

            // update mData in AbtractItem, because item->getPhase use item.mData.value(STATE_IS_SELECTED)
            // and item.mData.value(STATE_IS_CURRENT)
            item->setSelectedInData(selected);
            item->setCurrentInData(isCurrent);
            const QJsonObject nextPhase = item->getPhase();

            if (nextPhase != prevPhase)
                modified = true;

#ifdef DEBUG
            if (modified)
                qDebug()<<"PhasesScene::updateStateSelectionFromItem "<<nextPhase.value(STATE_NAME).toString()<<selected<<isCurrent;
#endif


        }

        if (modified) {
           sendUpdateProject(tr("phases selection updated : phases marked as selected"), true, true);

            // refresh the thumbs in the Events scene
            if (!oneSelection) {// selectedItems().size() == 0) {
                qDebug()<<"PhasesScene::updateStateSelectionFromItem emit : no phase";
                emit noSelection();
            } else
                emit phasesAreSelected();
        }
    }
}

void PhasesScene::adaptItemsForZoom(const double prop)
{
    mZoom = prop;
    for (int i=0; i<mItems.size(); ++i) {
        PhaseItem* item = static_cast<PhaseItem*>(mItems.at(i));
        item->setControlsVisible(mZoom > 0.6);
    }
}

/* Utilities
 *
 */
PhaseItem* PhasesScene::currentPhase() const
{
    QList<QGraphicsItem*> items = selectedItems();
    if (items.size() > 0) {
        PhaseItem* item = dynamic_cast<PhaseItem*>(items.at(0));
        if (item)
            return item;
    }
    return nullptr;
}

AbstractItem* PhasesScene::currentItem()
{
    QList<QGraphicsItem*> items = selectedItems();
    if (items.size() > 0) {
        PhaseItem* item = dynamic_cast<PhaseItem*>(items.at(0));
        if (item)
            return item;
    }
    return nullptr;
}

AbstractItem* PhasesScene::collidingItem(QGraphicsItem* item)
{
    for (int i=0; i<mItems.size(); ++i) {
        bool isPhase = (dynamic_cast<PhaseItem*>(mItems.at(i)) != 0);
        if (item != mItems.at(i) && isPhase && item->collidesWithItem(mItems[i]))
            return mItems[i];
    }
    return nullptr;
}


/* Phase Items Events
 *
 */
bool PhasesScene::itemClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    qDebug() << "PhasesScene::itemClicked";

    PhaseItem* phaseClicked = dynamic_cast< PhaseItem*>(item);
    PhaseItem* current = dynamic_cast< PhaseItem*>(currentItem());

    // if mDrawingArrow is true, an Phase is already selected and we can create a Constraint.
    if (phaseClicked ) {
        if (current && (phaseClicked != current)) {
            if (mDrawingArrow && constraintAllowed(current, phaseClicked)) {
                    createConstraint(current, phaseClicked);
                    mTempArrow->setVisible(false);
                    mDrawingArrow=false;
                  //  updateStateSelectionFromItem();
                    sendUpdateProject("Phase constraint created", true, true);

              }
        } else {
            //phaseClicked->setSelected(true);
          //  updateStateSelectionFromItem();
          //  sendUpdateProject("Item selected", true, false);//  bool notify = true, bool storeUndoCommand = false
        }


    } else {
        clearSelection();
        updateStateSelectionFromItem();
        sendUpdateProject("No Item selected", true, false);//  bool notify = true, bool storeUndoCommand = false
    }

    //updateStateSelectionFromItem();

    return true;
}


/**
 * @brief PhasesScene::itemEntered Arrive when flying over a Phase, it's an overwrite of AbstractScene::itemEntered
 * @param item
 * @param e
 */
void PhasesScene::itemEntered(AbstractItem* item, QGraphicsSceneHoverEvent* e)
{
    Q_UNUSED(e);
    qDebug() << "PhasesScene::itemEntered";
    AbstractItem* current = currentItem();

    //mTempArrow->setTo(item->pos().x(), item->pos().y());

    if (mDrawingArrow && current && item && (item != current)) {
        mTempArrow->setTo(item->pos().x(), item->pos().y());
        if (constraintAllowed(current, item)) {
            mTempArrow->setState(ArrowTmpItem::eAllowed);
            mTempArrow->setLocked(true);
         } else {
            mTempArrow->setState(ArrowTmpItem::eForbidden);
            mTempArrow->setLocked(false);
        }
    }

}

void PhasesScene::itemDoubleClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e)
{
    AbstractScene::itemDoubleClicked(item, e);
    if (!mDrawingArrow)
        mProject->updatePhase(static_cast<PhaseItem*>(item)->getPhase());
}

void PhasesScene::constraintDoubleClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    mProject->updatePhaseConstraint(item->data().value(STATE_ID).toInt());
}

void PhasesScene::constraintClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e)
{
    mProject->updatePhaseConstraint(item->data().value(STATE_ID).toInt());
    Q_UNUSED(e);
}


//#pragma mark Check state
/*void PhasesScene::updateCheckedPhases()
{
    QJsonObject state = mProject->state();
    QJsonArray events = state.value(STATE_EVENTS).toArray();
    
    // tableau contenant les id des phases associés à leur nombre d'apparition dans les events
    QHash<int, int> phases;
    // nombre d'évènements sélectionnés
    int selectedEventsCount = 0;
    for (int i=0; i<events.size(); ++i) {
        QJsonObject event = events.at(i).toObject();
        
        if (event.value(STATE_IS_SELECTED).toBool()) {
            QString phaseIdsStr = event.value(STATE_EVENT_PHASE_IDS).toString();
            if (!phaseIdsStr.isEmpty()) {
                QStringList phaseIds = phaseIdsStr.split(",");
                for (int j=0; j<phaseIds.size(); ++j) {
                    const int phaseId = phaseIds.at(j).toInt();
                    if (phases.find(phaseId) == phases.end())
                        phases[phaseId] = 1;
                    else
                        phases[phaseId] += 1;
                }
            }
            ++selectedEventsCount;
        }
    }
    
    for (int i=0; i<mItems.size(); ++i) {
        PhaseItem* item = static_cast<PhaseItem*>(mItems.at(i));
        QJsonObject phase = item->getPhase();
        int id = phase.value(STATE_ID).toInt();
        
        if (phases.find(id) == phases.end())
            //item->setState(Qt::Unchecked);
            item->mAtLeastOneEventSelected = false;
        else
            item->mAtLeastOneEventSelected = true;
            //item->setState((phases[id] == selectedEventsCount) ? Qt::Checked : Qt::PartiallyChecked);
    
    }
}
*/
