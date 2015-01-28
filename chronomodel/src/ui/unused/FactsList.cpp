#include "FactsList.h"
#include "ProjectManager.h"
#include "Project.h"
#include "FactsListItemDelegate.h"
#include "MainWindow.h"
#include "ModelUtilities.h"
#include <QtWidgets>


FactsList::FactsList(QWidget* parent):QListWidget(parent)
{
    setSortingEnabled(false);
    //mFactsList->setDragDropMode(QAbstractItemView::DragDrop);
    //mFactsList->setDefaultDropAction(Qt::MoveAction);
    setSelectionMode(QAbstractItemView::SingleSelection);
    
    
    FactsListItemDelegate* delegate = new FactsListItemDelegate();
    setItemDelegate(delegate);
    
    // ----------
    
    Project* project = ProjectManager::getInstance()->getCurrentProject();
    
    //connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)), project, SLOT(updateCurrentFact()));
    connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)), MainWindow::getInstance(), SLOT(showDatesIfApplyable()));
    
    connect(project, SIGNAL(factCreated(Fact*)), this, SLOT(resetFactsList()));
    connect(project, SIGNAL(factUpdated(Fact*)), this, SLOT(resetFactsList()));
    connect(project, SIGNAL(factDeleted()), this, SLOT(resetFactsList()));
    
    // This is to update the number of dates in the fact in th yhe custom list view :
    connect(project, SIGNAL(dateCreated(Date*)), this, SLOT(resetFactsList()));
    connect(project, SIGNAL(dateUpdated(Date*)), this, SLOT(resetFactsList()));
    
    connect(this, SIGNAL(currentRowChanged(int)), this, SLOT(factClickedAt(int)));
    connect(project, SIGNAL(currentFactChanged(Fact*)), this, SLOT(setCurrentFact(Fact*)));
}

FactsList::~FactsList()
{
    
}

void FactsList::factClickedAt(int row)
{
    Project* project = ProjectManager::getInstance()->getCurrentProject();
    project->setCurrentFactIndex(row);
}

void FactsList::setCurrentFact(Fact* fact)
{
    Project* project = ProjectManager::getInstance()->getCurrentProject();
    std::vector<Fact*>& facts = project->mFacts;
    
    for(int i=0; i<(int)facts.size(); ++i)
    {
        if(facts[i] == fact && currentRow() != i)
        {
            setCurrentRow(i);
            break;
        }
    }
}

void FactsList::resetFactsList()
{
    Project* project = ProjectManager::getInstance()->getCurrentProject();
    std::vector<Fact*>& facts = project->mFacts;
    Fact* currentFact = project->getCurrentFact();
    clear();
    
    for(unsigned i=0; i<facts.size(); ++i)
    {
        Fact* fact = facts[i];
        
        QListWidgetItem* item = new QListWidgetItem();
        item->setFlags(Qt::ItemIsSelectable
                       | Qt::ItemIsEnabled
                       | Qt::ItemNeverHasChildren);
        
        item->setText(fact->mName.c_str());
        item->setIcon(ModelUtilities::getFactIcon(fact->mType));
        
        item->setData(0x0101, fact->mName.c_str());
        item->setData(0x0102, (int)fact->mType);
        item->setData(0x0103, (int)fact->mDates.size());
        item->setData(0x0104, fact->mRed);
        item->setData(0x0105, fact->mGreen);
        item->setData(0x0106, fact->mBlue);
        
        QString toolTipText = tr("Dates") + ": " + QString::number(fact->mDates.size());
        toolTipText += "<br>" + tr("Type") + ": " + ModelUtilities::getFactTypeText(fact->mType);
        
        switch(fact->mType)
        {
            case Fact::eClassic:
                toolTipText += "<br>" + tr("Method") + ": " + ModelUtilities::getFactMethodText(fact->mMethod);
                break;
            case Fact::eUnvariant:
            default:
                break;
        }
        item->setToolTip(toolTipText);
        
        addItem(item);
        
        if(currentFact == fact)
        {
            setCurrentItem(item);
        }
    }
}

#pragma mark Events
void FactsList::keyPressEvent(QKeyEvent* keyEvent)
{
    if(keyEvent->key() == Qt::Key_Delete)
    {
        Project* project = ProjectManager::getInstance()->getCurrentProject();
        project->deleteCurrentFact();
    }
    else if(keyEvent->key() == Qt::Key_N)
    {
        Project* project = ProjectManager::getInstance()->getCurrentProject();
        project->createFact();
    }
    QListWidget::keyPressEvent(keyEvent);
}

