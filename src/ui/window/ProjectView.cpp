#include "ProjectView.h"
#include "ModelView.h"
#include "ResultsView.h"
#include "Painting.h"
#include <QtWidgets>


ProjectView::ProjectView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags)
{
    mModelView = new ModelView();
    mResultsView = new ResultsView();
    mLogEdit = new QTextEdit();
    
    mLogEdit->setReadOnly(true);
    QPalette palette = mLogEdit->palette();
    palette.setColor(QPalette::Base, QColor(0, 0, 0, 255));
    palette.setColor(QPalette::Text, Qt::white);
    mLogEdit->setPalette(palette);
    QFont font = mLogEdit->font();
    font.setPointSizeF(pointSize(11));
    mLogEdit->setFont(font);
    
    mStack = new QStackedWidget();
    mStack->addWidget(mModelView);
    mStack->addWidget(mResultsView);
    mStack->addWidget(mLogEdit);
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

void ProjectView::updateLog(MCMCLoopMain& loop)
{
    mLogEdit->setText(loop.getLog());
}



void ProjectView::writeSettings()
{
    mModelView->writeSettings();
}

void ProjectView::readSettings()
{
    mModelView->readSettings();
}