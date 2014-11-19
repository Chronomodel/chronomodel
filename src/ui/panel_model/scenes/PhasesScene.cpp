#include "PhasesScene.h"
#include "PhaseItem.h"
#include "ArrowItem.h"
#include "MainWindow.h"
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
    MainWindow::getInstance()->getProject()->deleteSelectedPhases();
}

void PhasesScene::createConstraint(AbstractItem* itemFrom, AbstractItem* itemTo)
{
    QJsonObject phaseFrom = ((PhaseItem*)itemFrom)->phase();
    QJsonObject phaseTo = ((PhaseItem*)itemTo)->phase();
    
    MainWindow::getInstance()->getProject()->createPhaseConstraint(phaseFrom[STATE_PHASE_ID].toInt(),
                                                        phaseTo[STATE_PHASE_ID].toInt());
}

void PhasesScene::mergeItems(AbstractItem* itemFrom, AbstractItem* itemTo)
{
    QJsonObject phaseFrom = ((PhaseItem*)itemFrom)->phase();
    QJsonObject phaseTo = ((PhaseItem*)itemTo)->phase();
    
    MainWindow::getInstance()->getProject()->mergePhases(phaseFrom[STATE_PHASE_ID].toInt(),
                                              phaseTo[STATE_PHASE_ID].toInt());
}

#pragma mark Project Update
void PhasesScene::sendUpdateProject(const QString& reason, bool notify, bool storeUndoCommand)
{
    Project* project = MainWindow::getInstance()->getProject();
    
    QJsonObject statePrev = project->state();
    QJsonObject stateNext = statePrev;
    
    QJsonArray phases;
    for(int i=0; i<mItems.size(); ++i)
        phases.append(((PhaseItem*)mItems[i])->phase());
    stateNext[STATE_PHASES] = phases;
    
    if(statePrev != stateNext)
    {
        if(storeUndoCommand)
            MainWindow::getInstance()->getProject()->pushProjectState(stateNext, reason, notify);
        else
            MainWindow::getInstance()->getProject()->sendUpdateState(stateNext, reason, notify);
    }
}

void PhasesScene::updateProject()
{
    QJsonObject state = MainWindow::getInstance()->getProject()->state();
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
        emit MainWindow::getInstance()->getProject()->currentPhaseChanged(phase);
        sendUpdateProject(tr("phases selection updated : phases marked as selected"), false, false);
        MainWindow::getInstance()->getProject()->sendPhasesSelectionChanged();
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
        Project* project = MainWindow::getInstance()->getProject();
        project->updatePhase(((PhaseItem*)item)->phase());
    }
}

void PhasesScene::constraintDoubleClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(e);
    Project* project = MainWindow::getInstance()->getProject();
    project->updatePhaseConstraint(item->constraint()[STATE_PHASE_CONSTRAINT_ID].toInt());
}

void PhasesScene::updateEyedPhases()
{
    QMap<int, bool> mEyedPhases;
    for(int i=0; i<mItems.size(); ++i)
    {
        PhaseItem* item = ((PhaseItem*)mItems[i]);
        mEyedPhases.insert(item->mPhase[STATE_PHASE_ID].toInt(), item->mEyeActivated);
    }
    emit MainWindow::getInstance()->getProject()->eyedPhasesModified(mEyedPhases);
}


#pragma mark Check state

void PhasesScene::updateCheckedPhases()
{
    QJsonObject state = MainWindow::getInstance()->getProject()->state();
    QJsonArray events = state[STATE_EVENTS].toArray();
    
    // tableau contenant les id des phases associés à leur nombre d'apparition dans les events
    QHash<int, int> phases;
    // nombre d'évènements sélectionnés
    int selectedEventsCount = 0;
    for(int i=0; i<events.size(); ++i)
    {
        QJsonObject event = events[i].toObject();
        if(event[STATE_EVENT_IS_SELECTED].toBool())
        {
            QString phaseIdsStr = event[STATE_EVENT_PHASE_IDS].toString();
            if(!phaseIdsStr.isEmpty())
            {
                QStringList phaseIds = phaseIdsStr.split(",");
                for(int j=0; j<phaseIds.size(); ++j)
                {
                    int phaseId = phaseIds[j].toInt();
                    if(phases.find(phaseId) == phases.end())
                        phases[phaseId] = 1;
                    else
                        phases[phaseId] += 1;
                }
            }
            ++selectedEventsCount;
        }
    }
    
    for(int i=0; i<mItems.size(); ++i)
    {
        PhaseItem* item = (PhaseItem*)mItems[i];
        QJsonObject phase = item->phase();
        int id = phase[STATE_PHASE_ID].toInt();
        
        if(phases.find(id) == phases.end())
        {
            item->setState(Qt::Unchecked);
        }
        else
        {
            item->setState((phases[id] == selectedEventsCount) ? Qt::Checked : Qt::PartiallyChecked);
        }
    }
}
