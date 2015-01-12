#include "ProjectView.h"
#include "ModelView.h"
#include "ResultsView.h"
#include "Painting.h"
#include <QtWidgets>


ProjectView::ProjectView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags)
{
    mModelView = new ModelView();
    mResultsView = new ResultsView();
    
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Text, Qt::black);
    
    
    mLogModelEdit = new QTextEdit();
    mLogModelEdit->setReadOnly(true);
    mLogModelEdit->setAcceptRichText(true);
    mLogModelEdit->setFrameStyle(QFrame::NoFrame);
    mLogModelEdit->setPalette(palette);
    
    mLogInitEdit = new QTextEdit();
    mLogInitEdit->setReadOnly(true);
    mLogInitEdit->setAcceptRichText(true);
    mLogInitEdit->setFrameStyle(QFrame::NoFrame);
    mLogInitEdit->setPalette(palette);
    
    mLogResultsEdit = new QTextEdit();
    mLogResultsEdit->setReadOnly(true);
    mLogResultsEdit->setAcceptRichText(true);
    mLogResultsEdit->setFrameStyle(QFrame::NoFrame);
    mLogResultsEdit->setPalette(palette);
    
    QFont font = mLogInitEdit->font();
    font.setPointSizeF(pointSize(11));
    
    mLogModelEdit->setFont(font);
    mLogInitEdit->setFont(font);
    mLogResultsEdit->setFont(font);
    
    mLogTabs = new QTabWidget();
    mLogTabs->addTab(mLogModelEdit, tr("Model"));
    mLogTabs->addTab(mLogInitEdit, tr("MCMC"));
    mLogTabs->addTab(mLogResultsEdit, tr("Results"));
    mLogTabs->setContentsMargins(15, 15, 15, 15);
    
    mLogView = new QWidget();
    QVBoxLayout* logLayout = new QVBoxLayout();
    logLayout->addWidget(mLogTabs);
    mLogView->setLayout(logLayout);
    
    mStack = new QStackedWidget();
    mStack->addWidget(mModelView);
    mStack->addWidget(mResultsView);
    mStack->addWidget(mLogView);
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
    showModel();
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