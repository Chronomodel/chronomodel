/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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

#include "MainWindow.h"
#include "ModelView.h"
#include "QtUtilities.h"
#include "ResultsView.h"
#include "AppSettings.h"
#include "StateKeys.h"

ProjectView::ProjectView( QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags)
{
    setMouseTracking(true);
    setScreenDefinition();

    mModelView = new ModelView(this);
    mResultsView = new ResultsView(this);

    QPalette palette;
    palette.setColor(QPalette::Base, Qt::white);
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

    mLogAdaptEdit = new QTextEdit();
    mLogAdaptEdit->setReadOnly(true);
    mLogAdaptEdit->setAcceptRichText(true);
    mLogAdaptEdit->setFrameStyle(QFrame::NoFrame);
    mLogAdaptEdit->setPalette(palette);

    mLogResultsEdit = new QTextEdit();
    mLogResultsEdit->setReadOnly(true);
    mLogResultsEdit->setAcceptRichText(true);
    mLogResultsEdit->setFrameStyle(QFrame::NoFrame);
    mLogResultsEdit->setPalette(palette);

    mLogTabs = new Tabs(this);
    mLogTabs->setFixedHeight(mLogTabs->tabHeight());

    mLogTabs->addTab(tr("Model Description"));
    mLogTabs->addTab(tr("MCMC Initialization"));
    mLogTabs->addTab(tr("MCMC Adaptation"));
    mLogTabs->addTab(tr("Posterior Distrib. Stats"));

    mLogLayout = new QVBoxLayout();
    mLogLayout->addWidget(mLogTabs);
    mLogLayout->addWidget(mLogResultsEdit);

    mLogView = new QWidget();
    mLogView->setLayout(mLogLayout);

    connect(mLogTabs, &Tabs::tabClicked, this, &ProjectView::showLogTab);

    mStack = new QStackedWidget();
    mStack->addWidget(mModelView);
    mStack->addWidget(mResultsView);
    mStack->addWidget(mLogView);
    mStack->setCurrentIndex(0);

    QHBoxLayout* layout = new QHBoxLayout();
    layout->setContentsMargins(0, 20, 0, 0);
    layout->addWidget(mStack);
    setLayout(layout);

    connect(mResultsView, &ResultsView::resultsLogUpdated, this, &ProjectView::updateResultsLog);

    mLogTabs->setTab(3, false);

}

ProjectView::~ProjectView()
{

}

void ProjectView::setScreenDefinition()
{
    /* find screen definition */
    QScreen *screen;

   /*  int numScreen = QApplication::desktop()->screenNumber(this));

    if (numScreen>0) {
        screen = QApplication::screens().at(numScreen);

    } else {
        screen =  QGuiApplication::primaryScreen();
      //  numScreen = 0;
    }
    */
    screen =  QApplication::primaryScreen();
    //qreal mm_per_cm = 10;

    qreal cm_per_in = 2.54;
    // look for screen definition
    //        qDebug()<<"ProjectView::resizeEvent()"<< screen->name() <<" number="<<numScreen <<"logical Dots="<<screen->logicalDotsPerInch()<<"devicePixelRatio="<<screen->devicePixelRatio()<<"availableVirtualGeometry()="<<screen->availableVirtualGeometry()<<" unitX ="<<screen->logicalDotsPerInch() / cm_per_in;
    //        qDebug()<<"ProjectView::resizeEvent()"<< numScreen << QApplication::desktop()->screenGeometry(numScreen) << QApplication::desktop()->availableGeometry(numScreen)<< QApplication::desktop()->width();
    //        qDebug()<<"ProjectView::resizeEvent() screen setWidthUnit"<< screen->physicalDotsPerInchX() / cm_per_in;
    //        qDebug()<<"ProjectView::resizeEvent() screen setHeigthUnit"<< screen->physicalDotsPerInchY() / cm_per_in;

    int unitX = int(screen->logicalDotsPerInch() / cm_per_in);
    AppSettings::setWidthUnit( unitX);

    int unitY = int(screen->logicalDotsPerInch() / cm_per_in);
    AppSettings::setHeigthUnit( unitY);



   /*const int logTabHusefull (height() - mLogTabs->tabHeight() - AppSettings::heigthUnit());

    mLogModelEdit->resize( width() - AppSettings::widthUnit(), logTabHusefull );
    mLogMCMCEdit->resize( width() - AppSettings::widthUnit(), logTabHusefull );
    mLogResultsEdit->resize( width() - AppSettings::widthUnit() , logTabHusefull );

    */

    //int unitY = int(screen->physicalDotsPerInchY() / cm_per_in);


    AppSettings::setHeigthUnit(unitY);
}

void ProjectView::resizeEvent(QResizeEvent* e)
{
    (void)e;
    setScreenDefinition();

}

void ProjectView::setProject()
{
    mModelView->setProject();
    if (getProject_ptr()->withResults())
        mResultsView->setProject();
}

void ProjectView::clearInterface()
{
    mModelView   -> clearInterface();
    mResultsView -> clearResults();
}

void ProjectView::resetInterface()
{
    showModel();
    mModelView   -> resetInterface();
    if (mResultsView)
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
    
    updateResults();
}


void ProjectView::showLog()
{
    getModel_ptr()->mLogResults.clear();
    getModel_ptr()->generateResultsLog();
    updateResultsLog(getModel_ptr()->getResultsLog());
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

void ProjectView::applyFilesSettings(std::shared_ptr<ModelCurve> &model)
{
    // Rebuild all calibration curve

    const QJsonObject &state = getProject_ptr()->state();
    const StudyPeriodSettings &s = StudyPeriodSettings::fromJson(state.value(STATE_SETTINGS).toObject());
    const bool calibrate = mModelView->findCalibrateMissing();
    if (calibrate)
        mModelView->calibrateAll(s);

    applySettings(model);
}

void ProjectView::applySettings(std::shared_ptr<ModelCurve> &model)
{
    mModelView->applyAppSettings();

    if (model && !model->mEvents.isEmpty()) {
        const double memoThreshold = model->mThreshold;
        model->mThreshold = -1;
        model->clearThreshold();
        model->updateDensities(model->mFFTLength, model->mBandwidth, memoThreshold);
        mResultsView->generateCurves();
        //emit model->newCalculus(); //redraw densities

        model->generateModelLog();
        model->generateResultsLog();

        mLogModelEdit->setText(model->getModelLog());
        mLogInitEdit->setText(model->getInitLog());
        mLogAdaptEdit->setText(model->getAdaptLog());
        updateResultsLog(model->getResultsLog());

    }
}

void ProjectView::updateMultiCalibrationAndEventProperties()
{
    mModelView->updateMultiCalibrationAndEventProperties();
}

/**
 * This function is called when  MCMC has finished
 * We want to show the results :
 *  - The Model may have changed since the last display : update it in results
 *  - The Model densities have to be computed
 *  - The Model Logs have to be generated
 */
void ProjectView::initResults()
{
    auto model = getModel_ptr();
    model->updateDesignFromJson();
    model->initDensities();

    model->generateModelLog();
    model->generateResultsLog();

    mResultsView->initModel();
    // Update log view
    mLogModelEdit->setText(model->getModelLog());
    mLogInitEdit->setText(model->getInitLog());
    mLogAdaptEdit->setText(model->getAdaptLog());
    mLogResultsEdit->setText(model->getResultsLog());

    // Show results :
    mStack->setCurrentIndex(1);

}

void ProjectView::updateResults()
{
    auto model = getModel_ptr();

    if (model) {
        model->updateDesignFromJson();

        mResultsView->updateModel();
    }
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
    mLogTabs->setTab(i, true);

    QWidget* widFrom = nullptr;

    if (mLogModelEdit->isVisible())
        widFrom = mLogModelEdit;

    else if (mLogInitEdit->isVisible())
            widFrom = mLogInitEdit;

    else if (mLogResultsEdit->isVisible())
        widFrom = mLogResultsEdit;

    else if (mLogAdaptEdit->isVisible())
        widFrom = mLogAdaptEdit;

    else
        widFrom = nullptr;

    if (mLogTabs->currentName() == tr("Model Description") ) {
        mLogModelEdit->setVisible(true);
        mLogInitEdit->setVisible(false);
        mLogAdaptEdit->setVisible(false);
        mLogResultsEdit->setVisible(false);

        mLogLayout->replaceWidget(widFrom, mLogModelEdit);

     } else if (mLogTabs->currentName() == tr("MCMC Initialization") ) {
        mLogModelEdit->setVisible(false);
        mLogInitEdit->setVisible(true);
        mLogAdaptEdit->setVisible(false);
        mLogResultsEdit->setVisible(false);

        mLogLayout->replaceWidget(widFrom, mLogInitEdit);

    } else if (mLogTabs->currentName() == tr("MCMC Adaptation") ) {
       mLogModelEdit->setVisible(false);
       mLogInitEdit->setVisible(false);
       mLogAdaptEdit->setVisible(true);
       mLogResultsEdit->setVisible(false);

       mLogLayout->replaceWidget(widFrom, mLogAdaptEdit);

     } else if (mLogTabs->currentName() == tr("Posterior Distrib. Stats") ) {
        mLogModelEdit->setVisible(false);
        mLogInitEdit->setVisible(false);
        mLogAdaptEdit->setVisible(false);
        mLogResultsEdit->setVisible(true);

        mLogLayout->replaceWidget(widFrom, mLogResultsEdit);

     } else {
        mLogModelEdit->setVisible(false);
        mLogInitEdit->setVisible(false);
        mLogAdaptEdit->setVisible(false);
        mLogResultsEdit->setVisible(false);
    }


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

void ProjectView::toggleCurve(bool toggle)
{
    mModelView->showCurveSettings(toggle);
}

void ProjectView::eventsAreSelected()
 {
     mModelView->eventsAreSelected();
 }
