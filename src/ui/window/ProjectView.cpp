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
    
    mLogMCMCEdit = new QTextEdit();
    mLogMCMCEdit->setReadOnly(true);
    mLogMCMCEdit->setAcceptRichText(true);
    mLogMCMCEdit->setFrameStyle(QFrame::NoFrame);
    mLogMCMCEdit->setPalette(palette);
    
    mLogResultsEdit = new QTextEdit();
    mLogResultsEdit->setReadOnly(true);
    mLogResultsEdit->setAcceptRichText(true);
    mLogResultsEdit->setFrameStyle(QFrame::NoFrame);
    mLogResultsEdit->setPalette(palette);
    
    QFont font = mLogMCMCEdit->font();
    font.setPointSizeF(pointSize(11));
    
    mLogModelEdit->setFont(font);
    mLogMCMCEdit->setFont(font);
    mLogResultsEdit->setFont(font);
    
    mLogTabs = new QTabWidget();
    mLogTabs->addTab(mLogModelEdit,   tr("Model"));
    mLogTabs->addTab(mLogMCMCEdit,    tr("MCMC"));
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
    
    connect(mResultsView, SIGNAL(resultsLogUpdated(const QString&)), this, SLOT(updateResultsLog(const QString&)));
}

ProjectView::~ProjectView()
{
    
}

void ProjectView::doProjectConnections(Project* project)
{
    mModelView   -> doProjectConnections(project);
    mResultsView -> doProjectConnections(project);
}

void ProjectView::resetInterface()
{
    showModel();
    mModelView   -> resetInterface();
    mResultsView -> clearResults();
}

void ProjectView::updateProject()
{
    mModelView->updateProject();
}

void ProjectView::showModel()
{
    
    mStack->setCurrentIndex(0);
}

void ProjectView::updateResults(Model* model)
{
    if(model)
    {
        mResultsView -> mHasPhases = (model->mPhases.size() > 0);
        mResultsView -> updateResults(model);
        
        model->mLogModel=model->modelLog();
        mLogModelEdit->setText(model->mLogModel);
        
        mLogMCMCEdit->setText(model->mLogMCMC);
        
        model->mLogResults=model->resultsLog();
        mLogResultsEdit->setText(model->mLogResults);

        
        showResults();
        //mStack->setCurrentIndex(1);
    }
}

void ProjectView::showResults()
{
    //mStack -> setCurrentWidget(mResultsView);
    mStack->setCurrentIndex(1);
    mStack->update();
}

void ProjectView::showLog()
{
    mStack->setCurrentIndex(2);
}

void ProjectView::showHelp(bool show)
{
    mModelView->showHelp(show);
}

void ProjectView::updateLog(Model* model)
{
    if(model)
    {
        
        mResultsView->updateResults(model);
        mResultsView->updateGraphs();
        
        model->mLogModel=model->modelLog();
        mLogModelEdit->setText(model->mLogModel);

        mLogMCMCEdit->setText(model->mLogMCMC);

        model->mLogResults=model->resultsLog();
        mLogResultsEdit->setText(model->mLogResults);
        
        showResults();
    }
}
void ProjectView::updateResultsLog(const QString& log)
{
    mLogResultsEdit->setText(log);
}


void ProjectView::writeSettings()
{
    mModelView->writeSettings();
}

void ProjectView::readSettings()
{
    mModelView->readSettings();
}
void ProjectView::updateFormatDate()
{
    mModelView->updateFormatDate();
    
}