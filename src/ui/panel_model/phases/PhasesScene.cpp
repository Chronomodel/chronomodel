#include "PhasesScene.h"
#include "Phase.h"
#include "Event.h"
#include "PhaseConstraint.h"
#include "PhasesItem.h"
#include "PhasesSceneArrowItem.h"
#include "PhasesSceneArrowTmpItem.h"
#include "ProjectManager.h"
#include "Project.h"
#include <QtWidgets>


PhasesScene::PhasesScene(QGraphicsView* view, QObject* parent):QGraphicsScene(parent),
mView(view),
mDrawingArrow(false),
mUpdatingItems(false)
{
    mTempArrow = new PhasesSceneArrowTmpItem(this);
    addItem(mTempArrow);
    mTempArrow->setVisible(false);
    mTempArrow->setZValue(0);
    
    mHelpView = new QLabel();
    
    connect(this, SIGNAL(selectionChanged()), this, SLOT(updateSelection()));
}

PhasesScene::~PhasesScene()
{
    
}

#pragma mark Project Update
void PhasesScene::sendUpdateProject(const QString& reason, bool notify, bool storeUndoCommand)
{
    Project* project = ProjectManager::getProject();
    
    QJsonObject statePrev = project->state();
    QJsonObject stateNext = statePrev;
    
    QJsonArray phases;
    for(int i=0; i<mItems.size(); ++i)
        phases.append(mItems[i]->phase());
    stateNext[STATE_PHASES] = phases;
    
    if(statePrev != stateNext)
    {
        if(storeUndoCommand)
            ProjectManager::getProject()->pushProjectState(stateNext, reason, notify);
        else
            ProjectManager::getProject()->sendUpdateState(stateNext, reason, notify);
    }
}

void PhasesScene::updateProject()
{
    QJsonObject state = ProjectManager::getProject()->state();
    QJsonArray phases = state[STATE_PHASES].toArray();
    
    QList<int> phases_ids;
    for(int i=0; i<phases.size(); ++i)
        phases_ids << phases[i].toObject()[STATE_EVENT_ID].toInt();
    
    mUpdatingItems = true;
    
    // ------------------------------------------------------
    //  Delete items not in current state
    // ------------------------------------------------------
    for(int i=mItems.size()-1; i>=0; --i)
    {
        PhasesItem* item = mItems[i];
        QJsonObject& phase = item->phase();
        
        if(!phases_ids.contains(phase[STATE_PHASE_ID].toInt()))
        {
            qDebug() << "=> Phase item deleted : " << phase[STATE_PHASE_ID].toInt();
            removeItem(item);
            mItems.removeOne(item);
            delete item;
        }
    }
    
    // ------------------------------------------------------
    //  Create / Update event items
    // ------------------------------------------------------
    for(int i=0; i<phases.size(); ++i)
    {
        QJsonObject phase = phases[i].toObject();
        
        bool itemExists = false;
        for(int j=0; j<mItems.size(); ++j)
        {
            QJsonObject itemPhase = mItems[j]->phase();
            if(itemPhase[STATE_PHASE_ID].toInt() == phase[STATE_PHASE_ID].toInt())
            {
                itemExists = true;
                if(phase != itemPhase)
                {
                    // UPDATE ITEM
                    qDebug() << "Phase item updated : id = " << phase[STATE_PHASE_ID].toInt();
                    mItems[j]->setPhase(phase);
                }
            }
        }
        if(!itemExists)
        {
            // CREATE ITEM
            PhasesItem* phaseItem = new PhasesItem(this, phase);
            mItems.append(phaseItem);
            addItem(phaseItem);
            qDebug() << "Phase item created : id = " << phase[STATE_PHASE_ID].toInt();
        }
    }
    mUpdatingItems = false;
    update();
}

#pragma mark Selection & Current
void PhasesScene::updateSelection()
{
    if(!mUpdatingItems)
    {
        /*QList<int> selected_ids;
        for(int i=0; i<mItems.size(); ++i)
        {
            QJsonObject& phase = mItems[i]->phase();
            phase[STATE_PHASE_IS_SELECTED] = mItems[i]->isSelected();
            if(mItems[i]->isSelected())
                selected_ids.append(mItems[i]->phase()[STATE_PHASE_ID].toInt());
        }
        emit ProjectManager::getProject()->selectedPhasesChanged(selected_ids);*/
        
        Project* project = ProjectManager::getProject();
        QJsonObject statePrev = project->state();
        QJsonObject stateNext = statePrev;
        
        QJsonArray phases;
        for(int i=0; i<mItems.size(); ++i)
            phases.append(mItems[i]->phase());
        stateNext[STATE_PHASES] = phases;
        
        if(statePrev != stateNext)
        {
            ProjectManager::getProject()->sendUpdateState(stateNext, tr("Phases selection changed"), false);
        }
    }
}

// ----------------------------------------------------------------------------------------
//  Phases Add / Delete / Update
// ----------------------------------------------------------------------------------------
#pragma mark Phases Add / Delete / Update

/*void PhasesScene::createPhase(Phase* phase)
{
    PhasesItem* item = new PhasesItem(this, phase);
    item->setPos(phase->mItemX, phase->mItemY);
    mItems.append(item);
    addItem(item);
}

void PhasesScene::deletePhase(Phase* phase)
{
    for(int i=0; i<mItems.size(); ++i)
    {
        if(mItems[i]->mPhase == phase)
        {
            PhasesItem* item = mItems[i];
            removeItem(item);
            mItems.erase(mItems.begin() + i);
            delete item;
            break;
        }
    }
}

void PhasesScene::updatePhase(Phase* phase)
{
    for(int i=0; i<mItems.size(); ++i)
    {
        if(mItems[i]->mPhase == phase)
        {
            mItems[i]->update();
        }
    }
}

void PhasesScene::updateCheckedPhases()
{
    Project* project = ProjectManager::getProject();
    if(project)
    {
        QHash<Phase*, int> phases;
        int eventsCount = 0;
        for(int i=0; i<project->mEvents.size(); ++i)
        {
            Event* e = project->mEvents[i];
            if(e->mIsSelected)
            {
                for(int j=0; j<e->mPhases.size(); ++j)
                {
                    Phase* p = e->mPhases[j];
                    if(phases.find(p) == phases.end())
                        phases[p] = 1;
                    else
                        phases[p] += 1;
                }
                ++eventsCount;
            }
        }
        QHash<Phase*, bool> phasesStates;
        QHashIterator<Phase*, int> iter(phases);
        while(iter.hasNext())
        {
            iter.next();
            phasesStates[iter.key()] = (iter.value() == eventsCount);
            //qDebug() << "phase : " << iter.key() << ", count : " << iter.value();
        }
        
        for(int i=0; i<mItems.size(); ++i)
        {
            if(phasesStates.find(mItems[i]->mPhase) == phasesStates.end())
            {
                mItems[i]->setState(Qt::Unchecked);
            }
            else
            {
                mItems[i]->setState(phasesStates[mItems[i]->mPhase] ? Qt::Checked : Qt::PartiallyChecked);
            }
        }
    }
}*/

#pragma mark Selection & Current

/*PhasesItem* PhasesScene::currentItem()
{
    QList<QGraphicsItem*> items = selectedItems();
    if(items.size() > 0)
    {
        PhasesItem* evtItem = dynamic_cast<PhasesItem*>(items[0]);
        if(evtItem)
            return evtItem;
    }
    return 0;
}

void PhasesScene::setCurrentPhase(Phase* phase)
{
    for(int i=0; i<mItems.size(); ++i)
    {
        if(mItems[i]->mPhase == phase)
        {
            //qDebug() << "Scene setCurrentEvent. id : " << event->mId;
            mItems[i]->setFocus();
            mTempArrow->setFrom(mItems[i]->pos().x(), mItems[i]->pos().y());
            
            //if(mView)
            //mView->centerOn(mItems[i]);
        }
        mItems[i]->update();
    }
}*/

// ----------------------------------------------------------------------------------------
//  Phase Items Events
// ----------------------------------------------------------------------------------------
#pragma mark Phase Items Events

/*void PhasesScene::phaseClicked(PhasesItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    Project* project = ProjectManager::getProject();
    PhasesItem* curItem = currentItem();
    
    if(mDrawingArrow && curItem && item != curItem)
    {
        project->createPhaseConstraint(curItem->mPhase, item->mPhase);
    }
    else
    {
        project->setCurrentPhase(item->mPhase);
    }
}

void PhasesScene::phaseDoubleClicked(PhasesItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    if(!mDrawingArrow)
    {
        Project* project = ProjectManager::getProject();
        project->updatePhase(item->mPhase);
    }
}

void PhasesScene::phaseEntered(PhasesItem* item, QGraphicsSceneHoverEvent* e)
{
    Q_UNUSED(e);
    
    if(mDrawingArrow)
    {
        Project* project = ProjectManager::getProject();
        PhasesItem* current = currentItem();
        if(project->isPhaseConstraintAllowed(current->mPhase, item->mPhase))
        {
            mTempArrow->setState(PhasesSceneArrowTmpItem::eAllowed);
        }
        else
        {
            mTempArrow->setState(PhasesSceneArrowTmpItem::eForbidden);
        }
    }
    
    mTempArrow->setTo(item->pos().x(), item->pos().y());
    mTempArrow->setLocked(true);
}

void PhasesScene::phaseLeaved(PhasesItem* item, QGraphicsSceneHoverEvent* e)
{
    Q_UNUSED(item);
    Q_UNUSED(e);
    mTempArrow->setLocked(false);
    mTempArrow->setState(PhasesSceneArrowTmpItem::eNormal);
}

void PhasesScene::phaseMoved(PhasesItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    
    item->mPhase->mItemX = item->pos().x();
    item->mPhase->mItemY = item->pos().y();
    
    // Only the current item can be moved :
    mTempArrow->setFrom(item->pos().x(), item->pos().y());
    
    updateConstraintsPos();
    
    // TODO : prevent collisions here ?
}

/*void PhasesScene::handlePhaseMoved(PhasesItem* item)
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

/*void PhasesScene::createPhaseConstraint(PhaseConstraint* constraint)
{
    if(constraint)
    {
        PhasesSceneArrowItem* arrow = new PhasesSceneArrowItem(this, constraint);
        for(int i=0; i<mItems.size(); ++i)
        {
            if(mItems[i]->mPhase == constraint->mPhaseFrom)
                arrow->mItemFrom = mItems[i];
            else if(mItems[i]->mPhase == constraint->mPhaseTo)
                arrow->mItemTo = mItems[i];
        }
        addItem(arrow);
        arrow->updatePosition();
        mConstraintItems.append(arrow);
    }
}

void PhasesScene::deletePhaseConstraint(PhaseConstraint* constraint)
{
    for(int i=0; i<mConstraintItems.size(); ++i)
    {
        if(mConstraintItems[i]->mConstraint == constraint)
        {
            PhasesSceneArrowItem* item = mConstraintItems[i];
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

void PhasesScene::constraintDoubleClicked(PhasesSceneArrowItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    Project* project = ProjectManager::getProject();
    project->updatePhaseConstraint(item->mConstraint);
}

void PhasesScene::updateConstraintsPos()
{
    for(int i=0; i<mConstraintItems.size(); ++i)
    {
        mConstraintItems[i]->updatePosition();
    }
}
*/

// ----------------------------------------------------------------------------------------
//  Scene Events
// ----------------------------------------------------------------------------------------
#pragma mark Scene Events
/*void PhasesScene::keyPressEvent(QKeyEvent* keyEvent)
{
    if(keyEvent->key() == Qt::Key_Delete)
    {
        Project* project = ProjectManager::getProject();
        project->deleteCurrentPhase();
    }
    else if(keyEvent->key() == Qt::Key_N)
    {
        Project* project = ProjectManager::getProject();
        project->createPhase();
    }
    else if(keyEvent->modifiers() == Qt::AltModifier)
    {
        mDrawingArrow = true;
        mTempArrow->setVisible(true);
    }
    QGraphicsScene::keyPressEvent(keyEvent);
}

void PhasesScene::keyReleaseEvent(QKeyEvent* keyEvent)
{
    mDrawingArrow = false;
    mTempArrow->setVisible(false);
    QGraphicsScene::keyReleaseEvent(keyEvent);
}

void PhasesScene::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
    if(mDrawingArrow)
    {
        //qDebug() << e->scenePos().x();
        mTempArrow->setTo(e->scenePos().x(), e->scenePos().y());
    }
    QGraphicsScene::mouseMoveEvent(e);
}
*/


