#include "PhasesList.h"
#include "Phase.h"
#include "ProjectManager.h"
#include "Project.h"
#include "Fact.h"
#include <QtWidgets>


PhasesList::PhasesList(QWidget* parent):QListWidget(parent)
{
    setSortingEnabled(false);
    //setDragDropMode(QAbstractItemView::DragDrop);
    //setDefaultDropAction(Qt::MoveAction);
    setSelectionMode(QAbstractItemView::SingleSelection);
    
    // ----------
    
    connect(this, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemHasChanged(QListWidgetItem*)));
    connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(updatePhase()));
    connect(this, SIGNAL(currentRowChanged(int)), this, SLOT(phaseClickedAt(int)));
    
    // ----------
    
    Project* project = ProjectManager::getInstance()->getCurrentProject();
    
    connect(project, SIGNAL(phaseCreated(Phase*)), this, SLOT(resetPhasesList(Phase*)));
    connect(project, SIGNAL(phaseUpdated(Phase*)), this, SLOT(resetPhasesList(Phase*)));
    connect(project, SIGNAL(phaseDeleted()), this, SLOT(resetPhasesList()));
    connect(project, SIGNAL(currentPhaseChanged(Phase*)), this, SLOT(setCurrentPhase(Phase*)));
    
    // ----------
    
    connect(project, SIGNAL(currentFactChanged(Fact*)), this, SLOT(checkFactPhases(Fact*)));
}

PhasesList::~PhasesList()
{
    
}

void PhasesList::updatePhase()
{
    Project* project = ProjectManager::getInstance()->getCurrentProject();
    project->updatePhaseAt(currentRow());
}

void PhasesList::phaseClickedAt(int row)
{
    qDebug() << "Phase list row : " << row;
    
    Project* project = ProjectManager::getInstance()->getCurrentProject();
    project->setCurrentPhaseIndex(row);
}

void PhasesList::setCurrentPhase(Phase* phase)
{
    Project* project = ProjectManager::getInstance()->getCurrentProject();
    std::vector<Phase*>& phases = project->mPhases;
    
    for(int i=0; i<(int)phases.size(); ++i)
    {
        if(phases[i] == phase && currentRow() != i)
        {
            qDebug() << "List setCurrentPhase. row : " << i << ", id : " << phase->mId;
            setCurrentRow(i);
            break;
        }
    }
}

void PhasesList::resetPhasesList(Phase* currentPhase)
{
    Project* project = ProjectManager::getInstance()->getCurrentProject();
    std::vector<Phase*>& phases = project->mPhases;
    clear();
    for(unsigned i=0; i<phases.size(); ++i)
    {
        QListWidgetItem* item = new QListWidgetItem();
        item->setFlags(Qt::ItemIsSelectable
                       | Qt::ItemIsDragEnabled
                       | Qt::ItemIsDropEnabled
                       | Qt::ItemIsUserCheckable
                       | Qt::ItemIsEnabled
                       | Qt::ItemNeverHasChildren);
        
        QColor color(phases[i]->mRed, phases[i]->mGreen, phases[i]->mBlue);
        QPixmap pixmap(20, 20);
        QPainter p(&pixmap);
        p.fillRect(0, 0, pixmap.width(), pixmap.height(), color);
        
        QIcon icon(pixmap);
        item->setIcon(icon);
        
        item->setText(phases[i]->mName.c_str());
        item->setCheckState(Qt::Unchecked);
        
        addItem(item);
        
        if(currentPhase == phases[i])
        {
            setCurrentItem(item);
        }
    }
}

#pragma mark Facts phase attribution

void PhasesList::checkFactPhases(Fact* fact)
{
    if(fact)
    {
        Project* project = ProjectManager::getInstance()->getCurrentProject();
        std::vector<Phase*>& phases = project->mPhases;
        
        for(int i=0; i<(int)phases.size(); ++i)
        {
            bool found = false;
            for(int j=0; j<(int)fact->mPhases.size(); ++j)
            {
                if(fact->mPhases[j] == phases[i])
                {
                    item(i)->setCheckState(Qt::Checked);
                    found = true;
                    break;
                }
            }
            if(!found)
                item(i)->setCheckState(Qt::Unchecked);
        }
    }
}

void PhasesList::itemHasChanged(QListWidgetItem* item)
{
    Project* project = ProjectManager::getInstance()->getCurrentProject();
    std::vector<Phase*>& phases = project->mPhases;
    
    int index = row(item);
    if(index >= 0 && index < (int)phases.size())
    {
        project->setCurrentFactInPhase(phases[index], item->checkState() == Qt::Checked);
    }
}

#pragma mark Events
void PhasesList::keyPressEvent(QKeyEvent* keyEvent)
{
    if(keyEvent->key() == Qt::Key_Delete)
    {
        Project* project = ProjectManager::getInstance()->getCurrentProject();
        project->deleteCurrentPhase();
    }
    else if(keyEvent->key() == Qt::Key_N)
    {
        Project* project = ProjectManager::getInstance()->getCurrentProject();
        project->createPhase();
    }
    QListWidget::keyPressEvent(keyEvent);
}