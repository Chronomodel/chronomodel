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
    
    MainWindow::getInstance()->getProject()->createPhaseConstraint(phaseFrom[STATE_ID].toInt(),
                                                        phaseTo[STATE_ID].toInt());
}

void PhasesScene::mergeItems(AbstractItem* itemFrom, AbstractItem* itemTo)
{
    QJsonObject phaseFrom = ((PhaseItem*)itemFrom)->phase();
    QJsonObject phaseTo = ((PhaseItem*)itemTo)->phase();
    
    MainWindow::getInstance()->getProject()->mergePhases(phaseFrom[STATE_ID].toInt(),
                                              phaseTo[STATE_ID].toInt());
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
        phases_ids << phases[i].toObject()[STATE_ID].toInt();
    
    QList<int> constraints_ids;
    for(int i=0; i<constraints.size(); ++i)
        constraints_ids << constraints[i].toObject()[STATE_ID].toInt();
    
    mUpdatingItems = true;
    
    // ------------------------------------------------------
    //  Delete items not in current state
    // ------------------------------------------------------
    bool hasDeleted = false;
    for(int i=mItems.size()-1; i>=0; --i)
    {
        PhaseItem* item = (PhaseItem*)mItems[i];
        QJsonObject& phase = item->phase();
        
        if(!phases_ids.contains(phase[STATE_ID].toInt()))
        {
#if DEBUG
            qDebug() << "=> Phase item deleted : " << phase[STATE_ID].toInt();
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
    for(int i=0; i<phases.size(); ++i)
    {
        QJsonObject phase = phases[i].toObject();
        
        bool itemExists = false;
        for(int j=0; j<mItems.size(); ++j)
        {
            PhaseItem* item = (PhaseItem*)mItems[j];
            QJsonObject itemPhase = item->phase();
            if(itemPhase[STATE_ID].toInt() == phase[STATE_ID].toInt())
            {
                itemExists = true;
                if(phase != itemPhase)
                {
                    // UPDATE ITEM
#if DEBUG
                    qDebug() << "Phase item updated : id = " << phase[STATE_ID].toInt();
#endif
                    item->setPhase(phase);
                }
            }
        }
        if(!itemExists)
        {
            // CREATE ITEM
            PhaseItem* phaseItem = new PhaseItem(this, phase);
            mItems.append(phaseItem);
            addItem(phaseItem);
            
            // Pratique
            clearSelection();
            phaseItem->setSelected(true);
            
            // Note : setting an event in (0, 0) tells the scene that this item is new!
            // Thus the scene will move it randomly around the currently viewed center point.
            QPointF pos = phaseItem->pos();
            if(pos.isNull())
            {
                QList<QGraphicsView*> gviews = views();
                if(gviews.size() > 0)
                {
                    QGraphicsView* gview = gviews[0];
                    QPointF pt = gview->mapToScene(gview->width()/2, gview->height()/2);
                    int posDelta = 100;
                    phaseItem->setPos(pt.x() + rand() % posDelta - posDelta/2,
                                      pt.y() + rand() % posDelta - posDelta/2);
                }
            }
            
            hasCreated = true;
#if DEBUG
            qDebug() << "Phase item created : id = " << phase[STATE_ID].toInt();
#endif
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
#if DEBUG
            qDebug() << "Phase Constraint deleted : " << constraint[STATE_ID].toInt();
#endif
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
#if DEBUG
                    qDebug() << "Constraint updated : id = " << constraint[STATE_ID].toInt();
#endif
                    mConstraintItems[j]->setData(constraint);
                }
            }
        }
        if(!itemExists)
        {
            // CREATE ITEM
            ArrowItem* constraintItem = new ArrowItem(this, ArrowItem::ePhase, constraint);
            mConstraintItems.append(constraintItem);
            addItem(constraintItem);
#if DEBUG
            qDebug() << "Constraint created : id = " << constraint[STATE_ID].toInt();
#endif
        }
    }
    
    mUpdatingItems = false;
    
    // Deleting an item that was selected involves changing the selection (and updating properties view)
    // Nothing has been triggered so far because of the mUpdatingItems flag, so we need to trigger it now!
    // As well, creating an item changes the selection because we want the newly created item to be selected.
    if(hasDeleted || hasCreated)
    {
        updateSelection(true);
    }
    
    adjustSceneRect();
    adaptItemsForZoom(mZoom);
    updateEyedPhases();
}

void PhasesScene::clean()
{
    // ------------------------------------------------------
    //  Delete all items
    // ------------------------------------------------------
    for(int i=mItems.size()-1; i>=0; --i)
    {
        PhaseItem* item = (PhaseItem*)mItems[i];
        
#if DEBUG
        QJsonObject& phase = item->phase();
        qDebug() << "=> Phase item deleted : " << phase[STATE_ID].toInt();
#endif
        mItems.removeAt(i);
        
        // ????? This breaks the program!!! Delete abose does the jobs but is it safe?
        //removeItem(item);
        
        // This is a QObject : call deleteLater instead of delete
        item->deleteLater();
    }
    
    // ------------------------------------------------------
    //  Delete all constraints
    // ------------------------------------------------------
    for(int i=mConstraintItems.size()-1; i>=0; --i)
    {
        ArrowItem* constraintItem = mConstraintItems[i];
        QJsonObject& constraint = constraintItem->data();
#if DEBUG
        qDebug() << "Phase Constraint deleted : " << constraint[STATE_ID].toInt();
#endif
        removeItem(constraintItem);
        mConstraintItems.removeOne(constraintItem);
        delete constraintItem;
    }
    
    update(sceneRect());
}


#pragma mark Selection & Current
void PhasesScene::updateSelection(bool forced)
{
    if(!mUpdatingItems)
    {
        bool modified = false;
        
        for(int i=0; i<mItems.size(); ++i)
        {
            QJsonObject& phase = ((PhaseItem*)mItems[i])->phase();
            bool selected = mItems[i]->isSelected();
            if(phase[STATE_IS_SELECTED].toBool() != selected)
            {
                phase[STATE_IS_SELECTED] = selected;
                modified = true;
            }
            if(phase[STATE_IS_CURRENT].toBool())
            {
                phase[STATE_IS_CURRENT] = false;
                modified = true;
            }
        }
        QJsonObject phase;
        PhaseItem* curItem = (PhaseItem*)currentItem();
        if(curItem)
        {
            QJsonObject& p = curItem->phase();
            if(!p[STATE_IS_CURRENT].toBool())
            {
                p[STATE_IS_CURRENT] = true;
                phase = p;
                modified = true;
            }
        }
        if(modified || forced)
        {
            emit MainWindow::getInstance()->getProject()->currentPhaseChanged(phase);
            sendUpdateProject(tr("phases selection updated : phases marked as selected"), false, false);
            MainWindow::getInstance()->getProject()->sendPhasesSelectionChanged();
        }
    }
}

void PhasesScene::adaptItemsForZoom(double prop)
{
    mZoom = prop;
    for(int i=0; i<mItems.size(); ++i)
    {
        PhaseItem* item = (PhaseItem*)mItems[i];
        item->setControlsVisible(mZoom > 0.6);
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
    project->updatePhaseConstraint(item->data()[STATE_ID].toInt());
}

void PhasesScene::constraintClicked(ArrowItem* item, QGraphicsSceneMouseEvent* e)
{
    Q_UNUSED(item);
    Q_UNUSED(e);
}

void PhasesScene::updateEyedPhases()
{
    QMap<int, bool> mEyedPhases;
    for(int i=0; i<mItems.size(); ++i)
    {
        PhaseItem* item = ((PhaseItem*)mItems[i]);
        mEyedPhases.insert(item->mData[STATE_ID].toInt(), item->mEyeActivated);
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
        if(event[STATE_IS_SELECTED].toBool())
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
        int id = phase[STATE_ID].toInt();
        
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
