/* ---------------------------------------------------------------------
Copyright or © or Copr. CNRS	2014 - 2024

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

#include "PhasesScene.h"
#include "PhaseItem.h"
#include "ArrowItem.h"
#include "ArrowTmpItem.h"
#include "QtUtilities.h"
#include "StateKeys.h"
#include "Generator.h"
#include "Project.h"

#include <QtWidgets>


PhasesScene::PhasesScene(QGraphicsView* view, QObject* parent):
    AbstractScene(view, parent)
{
   connect(this, &QGraphicsScene::selectionChanged, this, &PhasesScene::updateStateSelectionFromItem);
}

PhasesScene::~PhasesScene()
{

}

/*
 *  Actions
 */
/**
 * @brief PhasesScene::deleteSelectedItems virtual implementation of AbstractScene
 */
void PhasesScene::deleteSelectedItems()
{
    getProject_ptr()->deleteSelectedPhases();
    emit noSelection();
}

bool PhasesScene::constraintAllowed(AbstractItem* itemFrom, AbstractItem* itemTo)
{
    const QJsonArray& constraints = getProject_ptr()->mState.value(STATE_PHASES_CONSTRAINTS).toArray();

    const QJsonObject& phaseFrom = ((PhaseItem*)itemFrom)->getData();
    const QJsonObject& phaseTo   = ((PhaseItem*)itemTo)->getData();

    const int phaseFromId = phaseFrom.value(STATE_ID).toInt();
    const int phaseToId   = phaseTo.value(STATE_ID).toInt();

    bool ConstraintAllowed = true;

    for (const auto c : constraints) {
       const QJsonObject& constraint = c.toObject();
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
    const QJsonObject phaseFrom = dynamic_cast<PhaseItem*>(itemFrom)->getData();
    const QJsonObject phaseTo = dynamic_cast<PhaseItem*>(itemTo)->getData();

    getProject_ptr()->createPhaseConstraint(phaseFrom.value(STATE_ID).toInt(),
                                                        phaseTo.value(STATE_ID).toInt());
}

void PhasesScene::mergeItems(AbstractItem* itemFrom, AbstractItem* itemTo)
{
    QJsonObject phaseFrom = ((PhaseItem*)itemFrom)->getData();
    QJsonObject phaseTo = ((PhaseItem*)itemTo)->getData();

    getProject_ptr()->mergePhases(phaseFrom.value(STATE_ID).toInt(),
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
    /* The code below is usefull on Windows with MSVC2017 */
    for( auto && it : mItems)
        it->update();

    update();

}

/*
 *  Project Update
 */
void PhasesScene::sendUpdateProject(const QString& reason, bool notify, bool storeUndoCommand)
{
    qDebug()<<"[PhasesScene::sendUpdateProject] "<<reason<<notify<<storeUndoCommand;
    //QJsonObject statePrev = mProject->state();
    QJsonObject stateNext = getProject_ptr()->state();

    QJsonArray phases = QJsonArray();
    for (const auto &item : mItems)
        phases.append(item->getData());

    stateNext[STATE_PHASES] = phases;

    if (storeUndoCommand)
        getProject_ptr()->pushProjectState(stateNext, reason, notify);
    else
        getProject_ptr()->sendUpdateState(stateNext, reason, notify);

}

void PhasesScene::createSceneFromState()
 {
     qDebug()<<"[PhasesScene::createSceneFromState]";

     const QJsonObject state = getProject_ptr()->state();
     const QJsonArray phases = state.value(STATE_PHASES).toArray();
     const QJsonArray constraints = state.value(STATE_PHASES_CONSTRAINTS).toArray();

     // ------------------------------------------------------
     //  Delete items not in current state
     // ------------------------------------------------------
     mUpdatingItems = true;

     // ------------------------------------------------------
     //  Delete all items
     // ------------------------------------------------------
     if (mItems.size()>0) {
         clearSelection();
         clear();
         mItems.clear();
      }

    // this item is delete with clear, but we need it. this is herited from AbstracScene
    mTempArrow = new ArrowTmpItem();
    addItem(mTempArrow);
    mTempArrow->setVisible(false);
    mTempArrow->setZValue(0);

     // ------------------------------------------------------
     //  Create phase items
     // ------------------------------------------------------

    for (QJsonArray::const_iterator iPhase= phases.constBegin(); iPhase != phases.constEnd(); ++iPhase) {
             // CREATE ITEM
        auto phaseItem = new PhaseItem(this, iPhase->toObject());
        mItems.append(phaseItem);
        addItem(phaseItem);
    }


    // ------------------------------------------------------
    //  Create  constraint items
    // ------------------------------------------------------
    for (const auto c : constraints) {
        const QJsonObject constraint = c.toObject();

        // CREATE ITEM
        ArrowItem* constraintItem = new ArrowItem(this, ArrowItem::ePhase, constraint);
        mConstraintItems.append(constraintItem);
        addItem(constraintItem);
 #ifdef DEBUG
        //qDebug() << "Constraint created : id = " << constraint[STATE_ID].toInt();
 #endif

    }

    mUpdatingItems = false;


 }


void PhasesScene::updateSceneFromState()
{
#ifdef DEBUG
    qDebug()<<"[PhasesScene::updateSceneFromState] Start";
    QElapsedTimer startTime;
    startTime.start();
#endif

    if (getProject_ptr()->mState.value(STATE_EVENTS).toArray().isEmpty() && getProject_ptr()->mState.value(STATE_PHASES).toArray().isEmpty() && mItems.isEmpty())
        return;

    if (getProject_ptr()->mState.value(STATE_PHASES).toArray().isEmpty() && mItems.isEmpty()) {
        // ------------------------------------------------------
        //  Delete items not in current state
        // ------------------------------------------------------
        blockSignals(true);
        for (qsizetype i = mItems.size()-1; i >= 0; --i) {
            auto item = (PhaseItem*)mItems[i];
            mItems.removeAt(i);
            // This is a QObject : call deleteLater instead of delete
            item->deleteLater();
        }

        // ------------------------------------------------------
        //  Delete constraints not in current state
        // ------------------------------------------------------
        for (qsizetype i = mConstraintItems.size()-1; i >= 0; --i) {
            ArrowItem* constraintItem = mConstraintItems[i];
            removeItem(constraintItem);
            mConstraintItems.removeOne(constraintItem);
            delete constraintItem;
        }
        blockSignals(false);
        mItems.squeeze();
        mConstraintItems.squeeze();
        return;
    }
    const QJsonObject &state = getProject_ptr()->state();
    const QJsonArray &phases = state.value(STATE_PHASES).toArray();
    const QJsonArray &constraints = state.value(STATE_PHASES_CONSTRAINTS).toArray();

    QList<int> phases_ids;
    for (const auto p : phases)
        phases_ids << p.toObject().value(STATE_ID).toInt();

    QList<int> constraints_ids;
    for (const auto c : constraints)
        constraints_ids << c.toObject().value(STATE_ID).toInt();

    mUpdatingItems = true;

    // ------------------------------------------------------
    //  Delete items not in current state
    // ------------------------------------------------------
    bool hasDeleted = false;
    blockSignals(true);
    for (qsizetype i = mItems.size()-1; i >=0 ; --i) {
        auto item = static_cast<PhaseItem*>(mItems[i]);
        QJsonObject& phase = item->getData();

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
    blockSignals(false);
    // ------------------------------------------------------
    //  Create / Update phase items
    // ------------------------------------------------------
    bool hasCreated = false;
    for (const auto p : phases) {
        const QJsonObject phase = p.toObject();

        bool itemExists = false;
        for (int j = 0; j<mItems.size(); ++j) {
            auto item = static_cast<PhaseItem*>(mItems[j]);
            const QJsonObject itemPhase = item->getData();
            if (itemPhase.value(STATE_ID).toInt() == phase.value(STATE_ID).toInt()) {
                itemExists = true;

                // When assigning events to a phase by clicking on the checkbox of the phase item,
                // the information is saved in the event, not in the phase!
                // So apparently, the phase hasn't changed, but we need to redraw its item to show the modified events inside it.
                //if(phase != itemPhase)
               // {
                    // UPDATE ITEM
#ifdef DEBUG
                    //qDebug() << "Phase item updated : id = " << phase[STATE_ID].toInt();
#endif
                    item->setPhase(phase);
              //  }
            }
            item = nullptr;
        }
        if (!itemExists) {
            // CREATE ITEM
            AbstractItem* phaseItem = new PhaseItem(this, phase);

            // Note : setting an phase in (0, 0) tells the scene that this item is new!
            // Thus the scene will move it randomly around currently viewed center point).
            QPointF pos = phaseItem->pos();
            if (pos.isNull()) {

                // With the code above the new phase item is created randomly near the center of the view
                QList<QGraphicsView*> gviews = views();
                if (gviews.size() > 0) {
                    QGraphicsView* gview = gviews[0];
                    QPointF pt = gview->mapToScene(gview->width()/2, gview->height()/2);
                    const int randCoef = 3;
                    const qreal shift_x = randCoef * Generator::randomUniform(-PhaseItem::mEltsHeight, PhaseItem::mEltsHeight);
                    const qreal shift_y = randCoef * Generator::randomUniform(-PhaseItem::mEltsHeight, PhaseItem::mEltsHeight);

                    phaseItem->setPos(pt.x() + shift_x, pt.y() + shift_y);
                    phaseItem->mData[STATE_ITEM_X] = pt.x() + shift_x;
                    phaseItem->mData[STATE_ITEM_Y] = pt.y() + shift_y;
                    gview = nullptr;
                }

                // the new phase item is created randomly near the central point (0, 0)
               // QPointF pt  (rand() % posDelta - posDelta/2, rand() % posDelta - posDelta/2);
               // phaseItem->setPos( pt.x(), pt.y());
            }

            mItems.append(phaseItem);
            addItem(phaseItem);
            hasCreated = true;

            // usefull, changing the selected item force to update the state ??
            clearSelection();
#ifdef DEBUG
            qDebug() << "[PhasesScene::updateScene] Phase item created : id = " << phase[STATE_ID].toInt();
#endif

        }
    }

    // ------------------------------------------------------
    //  Delete constraints not in current state
    // ------------------------------------------------------
    blockSignals(true);
    for (qsizetype i = mConstraintItems.size()-1; i>=0; --i) {
        ArrowItem* constraintItem = mConstraintItems[i];
        QJsonObject &constraint = constraintItem->data();

        if (!constraints_ids.contains(constraint.value(STATE_ID).toInt())) {
#ifdef DEBUG
            //qDebug() << "[PhasesScene::updateScene] Phase Constraint deleted : " << constraint[STATE_ID].toInt();
#endif
            removeItem(constraintItem);
            mConstraintItems.removeOne(constraintItem);
            delete constraintItem;
            constraintItem = nullptr;
        }
    }
    blockSignals(false);
    mConstraintItems.squeeze();
    // ------------------------------------------------------
    //  Create / Update constraint items
    // ------------------------------------------------------
    for (const auto c : constraints) {
        const QJsonObject& constraint = c.toObject();

        bool itemExists = false;
        for (const auto &ci : mConstraintItems) {
            ci->updatePosition();
            const QJsonObject& constraintItem = ci->data();
            if (constraintItem.value(STATE_ID).toInt() == constraint.value(STATE_ID).toInt()) {
                itemExists = true;
                if (constraint != constraintItem) {
                    // UPDATE ITEM
#ifdef DEBUG
                    //qDebug() << "Constraint updated : id = " << constraint[STATE_ID].toInt();
#endif
                    ci->setData(constraint);
                }

            }
        }
        if (!itemExists) {
            // CREATE ITEM
            ArrowItem* constraintItem = new ArrowItem(this, ArrowItem::ePhase, constraint);
            mConstraintItems.append(constraintItem);
            addItem(constraintItem);
#ifdef DEBUG
            //qDebug() << "[PhasesScene::updateScene] Constraint created : id = " << constraint[STATE_ID].toInt();
#endif
        }
    }

    mUpdatingItems = false;

    // Deleting an item that was selected involves changing the selection (and updating properties view)
    // Nothing has been triggered so far because of the mUpdatingItems flag, so we need to trigger it now!
    // As well, creating an item changes the selection because we want the newly created item to be selected.
    if (hasDeleted || hasCreated)
       updateStateSelectionFromItem();

    //adjustSceneRect();
    adaptItemsForZoom(mZoom);

    adjustSceneRect();
    //setSceneRect(specialItemsBoundingRect().adjusted(-30, -30, 30, 30));

#ifdef DEBUG
    qDebug()<<"[PhasesScene::updateSceneFromState] finish in "<< DHMS(startTime.elapsed());
#endif
}

void PhasesScene::clean()
{

    // ------------------------------------------------------
    //  Delete all items
    // ------------------------------------------------------
    for (qsizetype i = mItems.size()-1; i>=0; --i) {
        auto item = static_cast<PhaseItem*>(mItems[i]);
        mItems.removeAt(i);

        // ????? This breaks the program!!! Delete abose does the jobs but is it safe?
        //removeItem(item);

        // This is a QObject : call deleteLater instead of delete
        item->setVisible(false); // The item disappears and after it's deleted
        item->deleteLater();
    }

    // ------------------------------------------------------
    //  Delete all constraints
    // ------------------------------------------------------
    for (qsizetype i = mConstraintItems.size()-1; i>=0; --i) {
        ArrowItem* constraintItem = mConstraintItems[i];
#ifdef DEBUG
        qDebug() << "Phase Constraint deleted : " << constraintItem->data()[STATE_ID].toInt();
#endif
        removeItem(constraintItem);
        mConstraintItems.removeOne(constraintItem);
        delete constraintItem;
        constraintItem = nullptr;
    }

    clear();
    setSceneRect(QRectF());
    update(sceneRect());
}


/* Selection & Current
 *
 */
void PhasesScene::updateStateSelectionFromItem()
{
    qDebug()<<"[PhasesScene::updateStateSelectionFromItem]";
    if (!mUpdatingItems) {
        bool modified = false;
        bool oneSelection = false;
        auto curItem = dynamic_cast<PhaseItem*>(currentItem());
        blockSignals(true);
        for (qsizetype i=0; i<mItems.size(); ++i) {
            PhaseItem* item = dynamic_cast<PhaseItem*>(mItems.at(i));

            // without selected update
            const QJsonObject prevPhase = item->getData(); // Must be a copy

            const bool selected = item->isSelected();
            const bool isCurrent = (curItem == item);

            if (selected)
                oneSelection = true;

            // update mData in AbtractItem, because item->getPhase use item.mData.value(STATE_IS_SELECTED)
            // and item.mData.value(STATE_IS_CURRENT)
            item->setSelectedInData(selected);
            item->setCurrentInData(isCurrent);
            const QJsonObject& nextPhase = item->getData();

            if (nextPhase != prevPhase)
                modified = true;

#ifdef DEBUG
            if (modified)
                qDebug()<<"[PhasesScene::updateStateSelectionFromItem] "<< nextPhase.value(STATE_NAME).toString()<<selected<<isCurrent;
#endif


        }
        blockSignals(false);

        if (modified ) {
           sendUpdateProject(tr("Phases selection"), true, false);//true);

            // refresh the thumbs in the Events scene
            if (!oneSelection) {// selectedItems().size() == 0) {
                qDebug()<<"[PhasesScene::updateStateSelectionFromItem] emit : no phase";
                emit noSelection();
            } else
                emit phasesAreSelected();
        }
    }
}

void PhasesScene::adaptItemsForZoom(const double prop)
{
    mZoom = prop;
    for (auto &&abstract_item : mItems) {
        PhaseItem* item = static_cast<PhaseItem*>(abstract_item);
        item->setControlsVisible(mZoom > 0.6);
    }
}

/* Utilities
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

AbstractItem *PhasesScene::currentItem()
{
    QList<QGraphicsItem*> items = selectedItems();
    if (items.size() > 0) {
        AbstractItem* item = static_cast<AbstractItem*>(currentPhase());
        if (item)
            return item;
    }
    return nullptr;
}

AbstractItem* PhasesScene::collidingItem(const QGraphicsItem* item)
{
    for (auto &&abstract_item : mItems) {
        bool isPhase = (dynamic_cast<PhaseItem*>(abstract_item) != nullptr);
        if (isPhase && item != abstract_item &&  item->collidesWithItem(abstract_item))
            return abstract_item;
    }
    return nullptr;
}


/* Phase Items Events
 */
bool PhasesScene::itemClicked(AbstractItem* item, QGraphicsSceneMouseEvent*)
{
    qDebug() << "[PhasesScene::itemClicked]";

    PhaseItem* phaseClicked = dynamic_cast< PhaseItem*>(item);
    PhaseItem* current = currentPhase();//dynamic_cast< PhaseItem*>(currentItem().get());

    // if mDrawingArrow is true, an Phase is already selected and we can create a Constraint.
    if (phaseClicked ) {
        if (current && (phaseClicked != current)) {
            if (mDrawingArrow && constraintAllowed(current, phaseClicked)) {
                createConstraint(current, phaseClicked);
                mTempArrow->setVisible(false);
                mDrawingArrow = false;
                sendUpdateProject("Phase constraint created", true, true);

              }
        }

    } else {
        clearSelection();
        updateStateSelectionFromItem();
        sendUpdateProject("No Item selected", true, false);//  bool notify = true, bool storeUndoCommand = false
    }

    phaseClicked = nullptr;
    current = nullptr;

    return true;
}


/**
 * @brief PhasesScene::itemEntered Arrive when flying over a Phase, it's an overwrite of AbstractScene::itemEntered
 * @param item
 * @param e
 */
void PhasesScene::itemEntered(AbstractItem* item, QGraphicsSceneHoverEvent*)
{
    qDebug() << "[PhasesScene::itemEntered]";
    AbstractItem* current = currentItem();

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
    current = nullptr;
}

void PhasesScene::itemDoubleClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e)
{
    AbstractScene::itemDoubleClicked(item, e);
    if (!mDrawingArrow)
        getProject_ptr()->updatePhase(static_cast<PhaseItem*>(item)->getData());
}

void PhasesScene::constraintDoubleClicked(ArrowItem* item, QGraphicsSceneMouseEvent*)
{
    getProject_ptr()->updatePhaseConstraint(item->data().value(STATE_ID).toInt());
}

void PhasesScene::constraintClicked(ArrowItem* item, QGraphicsSceneMouseEvent*)
{
    getProject_ptr()->updatePhaseConstraint(item->data().value(STATE_ID).toInt());
}


