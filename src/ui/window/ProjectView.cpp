#include "ProjectView.h"
#include "ModelView.h"
#include "ResultsView.h"
#include "ProjectManager.h"
#include "Project.h"
#include <QtWidgets>


ProjectView::ProjectView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags)
{
    mModelView = new ModelView();
    mResultsView = new ResultsView();
    
    mStack = new QStackedWidget();
    mStack->addWidget(mModelView);
    mStack->addWidget(mResultsView);
    mStack->setCurrentIndex(0);
    
    QHBoxLayout* layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mStack);
    setLayout(layout);
}

ProjectView::~ProjectView()
{
    
}

void ProjectView::updateProject()
{
    mModelView->updateProject();
    //mResultsView->updateResults();
}

void ProjectView::showModel()
{
    mStack->setCurrentIndex(0);
}

void ProjectView::showResults()
{
    mStack->setCurrentIndex(1);
}

void ProjectView::showHelp(bool show)
{
    mModelView->showHelp(show);
    //mResultsView->showHelp(show);
}
