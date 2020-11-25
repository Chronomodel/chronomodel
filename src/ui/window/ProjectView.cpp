/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#include "ProjectView.h"
#include "ModelView.h"
#include "ResultsView.h"
#include "Painting.h"
#include "AppSettings.h"

// Constructor / Destructor / Init
ProjectView::ProjectView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags)
{
    setScreenDefinition();
    
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
    mLogTabs->addTab(mLogMCMCEdit,    tr("MCMC Initialisation"));
    mLogTabs->addTab(mLogResultsEdit, tr("Posterior Distrib. Stats"));

    connect(mLogTabs, &Tabs::tabClicked, this, &ProjectView::showLogTab);

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

    mLogTabs->setTab(2, false);
    mLogTabs->showWidget(2);
}

ProjectView::~ProjectView()
{

}

void ProjectView::setScreenDefinition()
{
    /* find screen definition */
    int numScreen (QApplication::desktop()->screenNumber(this));
    QScreen *screen = QApplication::screens().at(numScreen);

    //qreal mm_per_cm = 10;
    const qreal cm_per_in = 2.54;
    
    int unitX = int(screen->physicalDotsPerInchX() / cm_per_in);
    AppSettings::setWidthUnit(unitX);

    int unitY = int(screen->physicalDotsPerInchY() / cm_per_in);
    AppSettings::setHeigthUnit(unitY);
}
                              
void ProjectView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    
    setScreenDefinition();
    
    const int logTabHusefull (height() - mLogTabs->tabHeight() - AppSettings::heigthUnit());
    
    mLogModelEdit->resize(width() - AppSettings::widthUnit(), logTabHusefull);
    mLogMCMCEdit->resize(width() - AppSettings::widthUnit(), logTabHusefull);
    mLogResultsEdit->resize(width() - AppSettings::widthUnit() , logTabHusefull);
}

void ProjectView::setProject(Project* project)
{
    mModelView->setProject(project);
    mResultsView->setProject(project);
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

void ProjectView::showResults()
{
    mResultsView->clearResults();
    mStack->setCurrentIndex(1);
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

void ProjectView::newPeriod()
{
    mModelView->modifyPeriod();
}

void ProjectView::applyFilesSettings(Model* model)
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
    mModelView->applyAppSettings();
    
    if(model)
    {
        model->updateFormatSettings();
        model->generateModelLog();
        model->generateResultsLog();
        
        mLogModelEdit->setText(model->getModelLog());
        mLogMCMCEdit->setText(model->getMCMCLog());
        updateResultsLog(model->getResultsLog());
    }
}

void ProjectView::updateMultiCalibration()
{
    mModelView->updateMultiCalibration();
}

/**
 * This function is called when  MCMC has finished
 * We want to show the results :
 *  - The Model may have changed since the last display : update it in results
 *  - The Model densities have to be computed
 *  - The Model Logs have to be generated
 */
void ProjectView::initResults(Model* model)
{
    model->updateDesignFromJson();
    model->initDensities();
    
    model->generateModelLog();
    model->generateResultsLog();
    
    mResultsView->updateModel(model);
    
    // Update log view
    mLogModelEdit->setText(model->getModelLog());
    mLogMCMCEdit->setText(model->getMCMCLog());
    mLogResultsEdit->setText(model->getResultsLog());
    
    // Show results :
    mStack->setCurrentIndex(1);
}

void ProjectView::updateResultsLog(const QString& log)
{
#ifdef Q_OS_MAC
    const QFont font (qApp->font());
    QString styleSh = "QLineEdit { border-radius: 5px; font: "+ QString::number(font.pointSize()) + "px ;font-family: "+font.family() + ";}";
    mLogResultsEdit->setStyleSheet(styleSh);
#endif
    mLogResultsEdit->setHtml(log);
}

void ProjectView::showLogTab(const int &i)
{
    mLogTabs->showWidget(i);
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

void ProjectView::toggleChronocurve(bool toggle)
{
    mModelView->toggleChronocurve(toggle);
}
