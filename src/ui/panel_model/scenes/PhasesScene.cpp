#include "PhasesScene.h"
#include "PhaseItem.h"
#include "ArrowItem.h"
#include "ProjectManager.h"
#include "Project.h"
#include <QtWidgets>


PhasesScene::PhasesScene(QGraphicsView* view, QObject* parent):AbstractScene(view, parent)
{
    connect(this, SIGNAL(selectionChanged()), this, SLOT(updateSelection()));
}

PhasesScene::~PhasesScene()
{
    
}

#pragma mark Actions
void PhasesScene::deleteSelectedItems()
{
    ProjectManager::getProject()->deleteSelectedEvents();
}

void PhasesScene::createConstraint(AbstractItem* itemFrom, AbstractItem* itemTo)
{
    QJsonObject phaseFrom = ((PhaseItem*)itemFrom)->phase();
    QJsonObject phaseTo = ((PhaseItem*)itemTo)->phase();
    
    ProjectManager::getProject()->createPhaseConstraint(phaseFrom[STATE_PHASE_ID].toInt(),
                                                        phaseTo[STATE_PHASE_ID].toInt());
}

void PhasesScene::mergeItems(AbstractItem* itemFrom, AbstractItem* itemTo)
{
    QJsonObject phaseFrom = ((PhaseItem*)itemFrom)->phase();
    QJsonObject phaseTo = ((PhaseItem*)itemTo)->phase();
    
    ProjectManager::getProject()->mergePhases(phaseFrom[STATE_PHASE_ID].toInt(),
                                              phaseTo[STATE_PHASE_ID].toInt());
}

#pragma mark Project Update
void PhasesScene::sendUpdateProject(const QString& reason, bool notify, bool storeUndoCommand)
{
    Project* project = ProjectManager::getProject();
    
    QJsonObject statePrev = project->state();
    QJsonObject stateNext = statePrev;
    
    QJsonArray phases;
    for(int i=0; i<mItems.size(); ++i)
        phases.append(((PhaseItem*)mItems[i])->phase());
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
    QJsonArray constraints = state[STATE_PHASES_CONSTRAINTS].toArray();
    
    QList<int> phases_ids;
    for(int i=0; i<phases.size(); ++i)
        phases_ids << phases[i].toObject()[STATE_PHASE_ID].toInt();
    
    QList<int> constraints_ids;
    for(int i=0; i<constraints.size(); ++i)
        constraints_ids << constraints[i].toObject()[STATE_PHASE_CONSTRAINT_ID].toInt();
    
    mUpdatingItems = true;
    
    // ------------------------------------------------------
    //  Delete items not in current state
    // ------------------------------------------------------
    for(int i=mItems.size()-1; i>=0; --i)
    {
        PhaseItem* item = (PhaseItem*)mItems[i];
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
            QJsonObject itemPhase = ((PhaseItem*)mItems[j])->phase();
            if(itemPhase[STATE_PHASE_ID].toInt() == phase[STATE_PHASE_ID].toInt())
            {
                itemExists = true;
                if(phase != itemPhase)
                {
                    // UPDATE ITEM
                    qDebug() << "Phase item updated : id = " << phase[STATE_PHASE_ID].toInt();
                    ((PhaseItem*)mItems[j])->setPhase(phase);
                }
            }
        }
        if(!itemExists)
        {
            // CREATE ITEM
            PhaseItem* phaseItem = new PhaseItem(this, phase);
            mItems.append(phaseItem);
            addItem(phaseItem);
            qDebug() << "Phase item created : id = " << phase[STATE_PHASE_ID].toInt();
        }
    }
    
    // ------------------------------------------------------
    //  Delete constraints not in current state
    // ------------------------------------------------------
    for(int i=mConstraintItems.size()-1; i>=0; --i)
    {
        ArrowItem* constraintItem = mConstraintItems[i];
        QJsonObject& constraint = constraintItem->constraint();
        
        if(!constraints_ids.contains(constraint[STATE_PHASE_CONSTRAINT_ID].toInt()))
        {
            qDebug() << "Constraint deleted : " << constraint[STATE_PHASE_CONSTRAINT_ID].toInt();
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
            QJsonObject constraintItem = mConstraintItems[j]->constraint();
            if(constraintItem[STATE_PHASE_CONSTRAINT_ID].toInt() == constraint[STATE_PHASE_CONSTRAINT_ID].toInt())
            {
                itemExists = true;
                if(constraint != constraintItem)
                {
                    // UPDATE ITEM
                    qDebug() << "Constraint updated : id = " << constraint[STATE_PHASE_CONSTRAINT_ID].toInt();
                    mConstraintItems[j]->setConstraint(constraint);
                }
            }
        }
        if(!itemExists)
        {
            // CREATE ITEM
            ArrowItem* constraintItem = new ArrowItem(this, ArrowItem::ePhase, constraint);
            mConstraintItems.append(constraintItem);
            addItem(constraintItem);
            qDebug() << "Constraint created : id = " << constraint[STATE_PHASE_CONSTRAINT_ID].toInt();
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
        for(int i=0; i<mItems.size(); ++i)
        {
            QJsonObject& phase = ((PhaseItem*)mItems[i])->phase();
            phase[STATE_PHASE_IS_SELECTED] = mItems[i]->isSelected();
            phase[STATE_PHASE_IS_CURRENT] = false;
        }
        QJsonObject phase;
        PhaseItem* curItem = (PhaseItem*)currentItem();
        if(curItem)
        {
            QJsonObject& p = curItem->phase();
            p[STATE_PHASE_IS_CURRENT] = true;
            phase = p;
        }
        emit ProjectManager::getProject()->currentPhaseChanged(phase);
        sendUpdateProject(tr("phases selection updated"), false, false);
    }
}

#pragma mark Utilities
AbstractItem* PhasesScene::currentItem()
{
    QList<QGraphicsItem*> items = selectedItems();
    if(items.size() > 0)
    {
        PhaseItem* item = dynamic_cast<PhaseItem*>(items[0]);
        if(item)
            return item;
    }
    return 0;
}

AbstractItem* PhasesScene::collidingItem(QGraphicsItem* item)
{
    for(int i=0; i<mItems.size(); ++i)
    {
        bool isPhase = (dynamic_cast<PhaseItem*>(mItems[i]) != 0);
        if(item != mItems[i] && isPhase && item->collidesWithItem(mItems[i]))
            return mItems[i];
    }
    return 0;
}


#pragma mark Phase Items Events
void PhasesScene::itemDoubleClicked(AbstractItem* item, QGraphicsSceneMouseEvent* e)
{
    AbstractScene::itemDoubleClicked(item, e);
    if(!mDrawingArrow)
    {
        Project* project = ProjectManager::getProject();
        project->updatePhase(((PhaseItem*)item)->phase());
    }
}

void PhasesScene::constraintDoubleClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    Project* project = ProjectManager::getProject();
    project->updatePhaseConstraint(item->constraint()[STATE_PHASE_CONSTRAINT_ID].toInt());
}


#pragma mark Check state
void PhasesScene::updateCheckedPhases()
{
    /*Project* project = ProjectManager::getProject();
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
    }*/
}
