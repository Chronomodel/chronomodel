#include "ProjectView.h"
#include "ModelView.h"
#include "ResultsView.h"
#include "Painting.h"
#include <QtWidgets>


ProjectView::ProjectView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags)
{
    mModelView = new ModelView();
    mResultsView = new ResultsView();
    
    mLogModelEdit = new QTextEdit();
    mLogModelEdit->setReadOnly(true);
    mLogModelEdit->setAcceptRichText(true);
    
    mLogInitEdit = new QTextEdit();
    mLogInitEdit->setReadOnly(true);
    mLogInitEdit->setAcceptRichText(true);
    
    mLogResultsEdit = new QTextEdit();
    mLogResultsEdit->setReadOnly(true);
    mLogResultsEdit->setAcceptRichText(true);
    
    QFont font = mLogInitEdit->font();
    font.setPointSizeF(pointSize(11));
    
    mLogModelEdit->setFont(font);
    mLogInitEdit->setFont(font);
    mLogResultsEdit->setFont(font);
    
    mLogTabs = new QTabWidget();
    mLogTabs->addTab(mLogModelEdit, tr("Model Log"));
    mLogTabs->addTab(mLogInitEdit, tr("MCMC Log"));
    mLogTabs->addTab(mLogResultsEdit, tr("Results Log"));
    
    mStack = new QStackedWidget();
    mStack->addWidget(mModelView);
    mStack->addWidget(mResultsView);
    mStack->addWidget(mLogTabs);
    mStack->setCurrentIndex(0);
    
    QHBoxLayout* layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mStack);
    setLayout(layout);
}

ProjectView::~ProjectView()
{
    
}

void ProjectView::doProjectConnections(Project* project)
{
    mModelView->doProjectConnections(project);
    mResultsView->doProjectConnections(project);
}

void ProjectView::resetInterface()
{
    mModelView->resetInterface();
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

void ProjectView::showLog()
{
    mStack->setCurrentIndex(2);
}

void ProjectView::showHelp(bool show)
{
    mModelView->showHelp(show);
    //mResultsView->showHelp(show);
}

void ProjectView::updateLog(Model* model)
{
    if(model)
    {
        mLogModelEdit->setText(model->modelLog());
        mLogInitEdit->setText(model->mMCMCLog);
    }
}



void ProjectView::writeSettings()
{
    mModelView->writeSettings();
}

void ProjectView::readSettings()
{
    mModelView->readSettings();
}