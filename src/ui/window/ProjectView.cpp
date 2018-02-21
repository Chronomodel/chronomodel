#include "ProjectView.h"
#include "ModelView.h"
#include "ResultsView.h"
#include "Painting.h"
#include "AppSettings.h"

#include <QtWidgets>

// Constructor / Destructor / Init
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

    mLogTabs = new Tabs(this);
    mLogTabs->addTab(mLogModelEdit,   tr("Model Description"));
    mLogTabs->addTab(mLogMCMCEdit,    tr("MCMC Initialization"));
    mLogTabs->addTab(mLogResultsEdit, tr("Posterior Distrib. Stats"));

   // mLogTabs->setContentsMargins(15, 15, 15, 15);
    connect(mLogTabs, &Tabs::tabClicked, this, &ProjectView::showLogTab);

    const int logTabHusefull (height() - mLogTabs->tabHeight() - AppSettings::heigthUnit());

    mLogModelEdit->resize( width() -  AppSettings::widthUnit(), logTabHusefull );
    mLogMCMCEdit->resize( width() - AppSettings::widthUnit(), logTabHusefull );
    mLogResultsEdit->resize( width() -AppSettings::widthUnit() , logTabHusefull );
    mLogTabs->resize(mLogTabs->minimalWidth(), mLogTabs->minimalHeight());

    mStack = new QStackedWidget();
    mStack->addWidget(mModelView);
    mStack->addWidget(mResultsView);
    mStack->addWidget(mLogTabs);
    mStack->setCurrentIndex(0);
    
    QHBoxLayout* layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mStack);
    setLayout(layout);
    
    connect(mResultsView, &ResultsView::resultsLogUpdated, this, &ProjectView::updateResultsLog);

    setAppSettingsFont();
    mLogTabs->setTab(2, false);
    mLogTabs->showWidget(2);
}

ProjectView::~ProjectView()
{
    
}

void ProjectView::resizeEvent(QResizeEvent* e)
{
    (void) e;
    const int logTabHusefull (height() - mLogTabs->tabHeight() - AppSettings::heigthUnit());

    mLogModelEdit->resize( width() - AppSettings::widthUnit(), logTabHusefull );
    mLogMCMCEdit->resize( width() - AppSettings::widthUnit(), logTabHusefull );
    mLogResultsEdit->resize( width() -AppSettings::widthUnit() , logTabHusefull );
    mLogTabs->resize(width(), mLogTabs->minimalHeight());
}

void ProjectView::doProjectConnections(Project* project)
{
    mModelView   -> setProject(project);
    mResultsView -> doProjectConnections(project);
}

void ProjectView::resetInterface()
{
    showModel();
    mModelView   -> resetInterface();
    mResultsView -> clearResults();
}

void ProjectView::showHelp(bool show)
{
    mModelView->showHelp(show);
}

void ProjectView::showModel()
{
    mStack->setCurrentIndex(0);
}

/**
 * @brief ProjectView::changeDesign slot connected to Project::projectDesignChange() in MainWindows
 * @param refresh
 */
void ProjectView::changeDesign(bool refresh)
{
   (void) refresh;
  mRefreshResults = true;
}

void ProjectView::setAppSettingsFont()
{
    setFont(AppSettings::font());

    mModelView->applyAppSettings();
    mResultsView->applyAppSettings();
    mLogModelEdit->setFont(AppSettings::font());
    mLogMCMCEdit->setFont(AppSettings::font());
    mLogResultsEdit->setFont(AppSettings::font());
    mLogTabs->setFont(AppSettings::font());

    const int logTabHusefull (height() - mLogTabs->tabHeight() - AppSettings::heigthUnit());

    mLogModelEdit->resize( width() -  AppSettings::widthUnit(), logTabHusefull );
    mLogMCMCEdit->resize( width() - AppSettings::widthUnit(), logTabHusefull );
    mLogResultsEdit->resize( width() -AppSettings::widthUnit(), logTabHusefull );
    mLogTabs->resize(mLogTabs->minimalWidth(), mLogTabs->minimalHeight());

}

void ProjectView::showResults()
{
   // if (mRefreshResults) {
        mResultsView->clearResults();
        mResultsView->updateModel(); // update Design e.g. Name and color //updateResults() is call inside
        mRefreshResults = false;
   // }
    mStack->setCurrentIndex(1);
    // come from mViewResultsAction and  updateResults send repaint on mStack
}


void ProjectView::showLog()
{
    mResultsView->mModel->generateResultsLog();
    updateResultsLog(mResultsView->mModel->getResultsLog());
    mStack->setCurrentIndex(2);
}

/**
 * @brief Update All model views (Scenes, ...) after pushing state
 */
void ProjectView::updateProject()
{
    mModelView->updateProject();
}

void ProjectView::createProject()
{
    mModelView->createProject();
}

void ProjectView::newPeriod()
{
    mModelView->modifyPeriod();
}

void ProjectView:: applyFilesSettings(Model* model)
{
    // Rebuild all calibration curve

    QJsonObject state = mModelView->getProject()->state();
    ProjectSettings s = ProjectSettings::fromJson(state.value(STATE_SETTINGS).toObject());
    bool calibrate = mModelView->findCalibrateMissing();
    if (calibrate)
        mModelView->calibrateAll(s);

    applySettings(model);
}

void ProjectView::applySettings(Model* model)
{
    setAppSettingsFont();
    if (model) {

        mResultsView->updateFormatSetting(model);

        // force to regenerate the densities
        mResultsView->initResults(model);

        model->generateModelLog();
        mLogModelEdit->setText(model->getModelLog());

        mLogMCMCEdit->setText(model->getMCMCLog());

        model->generateResultsLog();
        updateResultsLog(model->getResultsLog());
    }
}

void ProjectView::updateMultiCalibration()
{
    mModelView->updateMultiCalibration();
}

void ProjectView::updateResults(Model* model)
{
    if (model) {
        mResultsView->updateResults(model);

        model->generateModelLog();
        mLogModelEdit->setText(model->getModelLog());

        mLogMCMCEdit->setText(model->getMCMCLog());

        model->generateResultsLog();
        mLogResultsEdit->setText(model->getResultsLog());

        mStack->setCurrentIndex(1);
    }
}

void ProjectView::initResults(Model* model)
{
    qDebug()<<"ProjectView::initResults()";
    if (model) {
        mResultsView->clearResults();
        mResultsView->updateFormatSetting(model);
        
        mResultsView->initResults(model);
        mRefreshResults = true;
        mResultsView->update();

        model->generateModelLog();
        mLogModelEdit->setText(model->getModelLog());

        mLogMCMCEdit->setText(model->getMCMCLog());

        model->generateResultsLog();
        mLogResultsEdit->setText(model->getResultsLog());

       // showResults();
       mStack->setCurrentIndex(1);
    }
    
}

void ProjectView::updateResultsLog(const QString& log)
{
    mLogResultsEdit->setText(log);
}

void ProjectView::showLogTab(const int &i)
{
    mLogTabs->showWidget(i);
   // mLogTabs->getCurrentWidget()->setGeometry(0, mLogTabs->getTabHeight() + 5, width(), height() - (mLogTabs->getTabHeight() + 5) );
}

//  Read/Write settings
void ProjectView::writeSettings()
{
    mModelView->writeSettings();
}

void ProjectView::readSettings()
{
    mModelView->readSettings();
}
