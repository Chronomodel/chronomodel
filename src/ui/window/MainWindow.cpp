/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2025

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

#include "MainWindow.h"

#include "ModelCurve.h"
#include "Project.h"
#include "ProjectView.h"
#include "PluginAbstract.h"
#include "AboutDialog.h"
#include "AppSettingsDialog.h"
#include "PluginManager.h"
#include "ResultsView.h"
#include "RebuildCurveDialog.h"
#include "AppSettings.h"
#include "version.h"

#include <iostream>

#include <QLocale>
#include <QFont>
#include <QMenuBar>
#include <QMenu>
#include <QTimer>
#include <QToolBar>
#include <QDesktopServices>
#include <QFileDialog>
#include <QToolTip>
#include <QInputDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QKeyEvent>


// Constructor / Destructor
MainWindow::MainWindow(QWidget* parent):
    QMainWindow(parent),
    undo_action(false),
    redo_action(false),
    mProject(new Project())
{

    setMouseTracking(true);

    mLastPath = QDir::homePath();

    /* Creation of ResultsView and ModelView */
    mProjectView = new ProjectView(this);
    setCentralWidget(mProjectView);

    mUndoStack = new QUndoStack();
    mUndoStack->setUndoLimit(1000);

    // special view of the undo-redo stack
    mUndoView = new QUndoView(mUndoStack);
    mUndoView->setEmptyLabel(tr("Initial state"));
    mUndoDock = new QDockWidget(this);
    mUndoDock->setWidget(mUndoView);
    mUndoDock->setAllowedAreas(Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, mUndoDock);
    mUndoDock->setVisible(false); // toggle to see the undo-list

    createActions();
    createMenus();
    createToolBars();

    statusBar()->showMessage(tr("Ready"));
    //setUnifiedTitleAndToolBarOnMac(true);
    setWindowIcon(QIcon(":chronomodel.png"));

    resize(AppSettings::mLastSize);

    connect(mProjectSaveAction, static_cast<void (QAction::*)(bool)> (&QAction::triggered), this, &MainWindow::saveProject);
    connect(mProjectSaveAsAction, static_cast<void (QAction::*)(bool)> (&QAction::triggered), this, &MainWindow::saveProjectAs);

    connect(mViewModelAction, static_cast<void (QAction::*)(bool)> (&QAction::triggered), mProjectView, &ProjectView::showModel );
    connect(mViewLogAction, static_cast<void (QAction::*)(bool)> (&QAction::triggered), mProjectView, &ProjectView::showLog);
    connect(mViewResultsAction, static_cast<void (QAction::*)(bool)> (&QAction::triggered), mProjectView, &ProjectView::showResults);

    QLocale newLoc(QLocale::system());
    AppSettings::mLanguage = newLoc.language();

#if QT_DEPRECATED_SINCE(6, 6)
    AppSettings::mCountry = newLoc.territory();
#else
    AppSettings::mCountry = newLoc.country();
#endif
    newLoc.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale::setDefault(newLoc);

    if (newLoc.decimalPoint()==',') {
        AppSettings::mCSVCellSeparator=";";
        AppSettings::mCSVDecSeparator=",";

    } else {
        AppSettings::mCSVCellSeparator=",";
        AppSettings::mCSVDecSeparator=".";
    }

    activateInterface(false);
    setMinimumSize(1000, 700);
}

MainWindow::~MainWindow()
{

}

// Accessors
const std::shared_ptr<Project> &MainWindow::getProject()
{
    return mProject;
}

QJsonObject &MainWindow::getState() const
{
    return mProject->mState;
}

QString MainWindow::getNameProject() const
{
    return mProject->mName;
}


QUndoStack* MainWindow::getUndoStack()
{
    return mUndoStack;
}

QString MainWindow::getCurrentPath() const
{
    return mLastPath;
}

void MainWindow::setCurrentPath(const QString& path)
{
    mLastPath = path;
}

// Actions & Menus
void MainWindow::createActions()
{
    //QWhatsThis::createAction();

    mAppSettingsAction = new QAction(QIcon(":settings_p.png"), tr("Settings"), this);
    connect(mAppSettingsAction, &QAction::triggered, this, &MainWindow::appSettings);

    //-----------------------------------------------------------------
    // Project Actions
    //-----------------------------------------------------------------

    mNewProjectAction = new QAction(QIcon(":new_p.png"), tr("&New"), this);
    mNewProjectAction->setShortcuts(QKeySequence::New);
    mNewProjectAction->setStatusTip(tr("Create a new project"));
    mNewProjectAction->setToolTip(tr("New project"));
    mNewProjectAction->setWhatsThis(tr("What's This? :Create a new project"));

    connect(mNewProjectAction, &QAction::triggered, this, &MainWindow::newProject);

    mOpenProjectAction = new QAction(QIcon(":open_p.png"), tr("Open"), this);
    mOpenProjectAction->setShortcuts(QKeySequence::Open);
    mOpenProjectAction->setStatusTip(tr("Open an existing project"));
    connect(mOpenProjectAction, &QAction::triggered, this, &MainWindow::openProject);

    mInsertProjectAction = new QAction(QIcon(":open_p.png"), tr("Insert"), this);
    mInsertProjectAction->setStatusTip(tr("Insert an existing project"));
    connect(mInsertProjectAction, &QAction::triggered, this, &MainWindow::insertProject);

    mCloseProjectAction = new QAction(tr("Close"), this);
    mCloseProjectAction->setShortcuts(QKeySequence::Close);
    mCloseProjectAction->setStatusTip(tr("Open an existing project"));
    connect(mCloseProjectAction, &QAction::triggered, this, &MainWindow::closeProject);

    mProjectSaveAction = new QAction(QIcon(":save_p.png"), tr("&Save"), this);
    mProjectSaveAction->setShortcuts(QKeySequence::Save);
    mProjectSaveAction->setStatusTip(tr("Save the current project in the same place with the same name"));

    mProjectSaveAsAction = new QAction(QIcon(":save_p.png"), tr("Save as..."), this);
    mProjectSaveAsAction->setStatusTip(tr("Change the current project on an other name or on an other place"));

    mUndoAction = mUndoStack->createUndoAction(this);
    mUndoAction->setShortcuts(QKeySequence::Undo);
    mUndoAction->setIcon(QIcon(":undo_p.png"));
    mUndoAction->setText(tr("Undo"));
    mUndoAction->setToolTip(tr("Undo"));

    connect(mUndoAction, &QAction::triggered, this, &MainWindow::toggleUndo);

    mRedoAction = mUndoStack->createRedoAction(this);
    mRedoAction->setShortcuts(QKeySequence::Redo);
    mRedoAction->setIcon(QIcon(":redo_p.png"));
    mRedoAction->setText(tr("Redo"));
    mRedoAction->setToolTip(tr("Redo"));

    connect(mRedoAction, &QAction::triggered, this, &MainWindow::toggleRedo);

    mUndoViewAction = mUndoDock->toggleViewAction();
    mUndoViewAction->setText(tr("Show Undo Stack"));

    mSelectAllAction = new QAction( tr("&Select All Events"), this);
    mSelectAllAction->setShortcuts(QKeySequence::SelectAll);
    mSelectAllAction->setStatusTip(tr("Select All Events"));
    connect(mSelectAllAction, &QAction::triggered, this, &MainWindow::selectAllEvents);


    //-----------------------------------------------------------------
    // MCMC Actions
    //-----------------------------------------------------------------
    mMCMCSettingsAction = new QAction(QIcon(":settings_p.png"), tr("MCMC"), this);
    mMCMCSettingsAction->setToolTip(tr("Change MCMC Settings"));

    mRunAction = new QAction(QIcon(":run_p.png"), tr("Run"), this);
    //runAction->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaPlay));
    mRunAction->setIconText(tr("Run"));
    mRunAction->setIconVisibleInMenu(true);
    mRunAction->setToolTip(tr("Run Model"));

    mResetMCMCAction = new QAction(tr("Reset Events and Data samplers"), this);

    //-----------------------------------------------------------------
    // View Actions
    //-----------------------------------------------------------------
    mCurveAction = new SwitchAction(this);
    mCurveAction->setCheckable(true);
    mCurveAction->setChecked(false);

    mViewModelAction = new QAction(QIcon(":model_p.png"), tr("Model"), this);
    mViewModelAction->setCheckable(true);

    mViewResultsAction = new QAction(QIcon(":results_p.png"), tr("Results"), this);
    mViewResultsAction->setCheckable(true);
    mViewResultsAction->setEnabled(false);

    mViewLogAction = new QAction(QIcon(":log_p.png"), tr("Log"), this);
    mViewLogAction->setCheckable(true);
    mViewLogAction->setEnabled(false);

    mViewGroup = new QActionGroup(this);
    mViewGroup->addAction(mViewModelAction);
    mViewGroup->addAction(mViewResultsAction);
    mViewGroup->addAction(mViewLogAction);
    mViewModelAction->setChecked(true);

    //-----------------------------------------------------------------
    //  Dates Actions (Plugins specific)
    //-----------------------------------------------------------------
    const QList<PluginAbstract*>& plugins = PluginManager::getPlugins();
    for (int i=0; i<plugins.size(); ++i) {
        QList<QHash<QString, QVariant>> groupedActions = plugins.at(i)->getGroupedActions();
        for (int j=0; j<groupedActions.size(); ++j) {
            QAction* act = new QAction(groupedActions[j]["title"].toString(), this);
            act->setData(QVariant(groupedActions[j]));
            connect(act, &QAction::triggered, this, &MainWindow::doGroupedAction);
            mDatesActions.append(act);
        }
    }

    //-----------------------------------------------------------------
    //  Grouped actions
    //-----------------------------------------------------------------
    mEventsColorAction = new QAction(tr("Selected Events: Change Colour"), this);
    connect(mEventsColorAction, &QAction::triggered, this, &MainWindow::changeEventsColor);

    mEventsMethodAction = new QAction(tr("Selected Events: Change Event MCMC"), this);
    connect(mEventsMethodAction, &QAction::triggered, this, &MainWindow::changeEventsMethod);

    mDatesMethodAction = new QAction(tr("Selected Events: Change Data MCMC"), this);
    connect(mDatesMethodAction, &QAction::triggered, this, &MainWindow::changeDatesMethod);

    mSelectEventsAction = new QAction(tr("Select All Events of the Selected Phases"), this);
    connect(mSelectEventsAction, &QAction::triggered, this, &MainWindow::selectEventInSelectedPhases);

    mSelectEventsNameAction = new QAction(tr("Select All Events with string"), this);
    connect(mSelectEventsNameAction, &QAction::triggered, this, &MainWindow::selectEventWithString);

    mRescaleCurveAction = new QAction(tr("Rescale Density Plots"), this);
    connect(mRescaleCurveAction, &QAction::triggered, this, &MainWindow::rebuildExportCurve);
    //-----------------------------------------------------------------
    // Help/About Menu
    //-----------------------------------------------------------------
    mAboutAct = new QAction(QIcon(":light.png"), tr("About"), this);
    connect(mAboutAct, &QAction::triggered, this, &MainWindow::about);

    mAboutQtAct = new QAction(QIcon(":qt.png"), tr("About Qt"), this);
    connect(mAboutQtAct, &QAction::triggered, qApp, QApplication::aboutQt);

    mHelpAction = new QAction(QIcon(":help_p.png"), tr("Help"), this);
    mHelpAction->setCheckable(true);
    connect(mHelpAction, &QAction::triggered, this, &MainWindow::showHelp);

    mManualAction = new QAction(QIcon(":pdf_p.png"), tr("Manual Online"), this);
    connect(mManualAction, &QAction::triggered, this, &MainWindow::openManual);

    mWebsiteAction = new QAction(QIcon(":web_p.png"), tr("Website"), this);
    connect(mWebsiteAction, &QAction::triggered, this, &MainWindow::openWebsite);

    //-----------------------------------------------------------------
    // Translation Menu
    //-----------------------------------------------------------------
    mTranslateEnglishAct = new QAction(tr("English"), this);
    mTranslateEnglishAct->setCheckable(true);
    mTranslateEnglishAct->setData("en");

    mTranslateFrenchAct = new QAction(tr("French"), this);
    mTranslateFrenchAct->setCheckable(true);
    mTranslateFrenchAct->setData("fr");

    mLangGroup = new QActionGroup(this);
    mLangGroup->addAction(mTranslateEnglishAct);
    mLangGroup->addAction(mTranslateFrenchAct);
    mTranslateEnglishAct->setChecked(true);
    connect(mLangGroup, &QActionGroup::triggered, this, &MainWindow::setLanguage);
}

void MainWindow::createMenus()
{
    //-----------------------------------------------------------------
    // Project menu
    //-----------------------------------------------------------------
    const  QFont ft (font());
    mProjectMenu = menuBar()->addMenu(tr("File"));
    mProjectMenu->addAction(mAppSettingsAction);
    mProjectMenu->addAction(mNewProjectAction);
    mProjectMenu->addAction(mOpenProjectAction);
    mProjectMenu->addAction(mInsertProjectAction);
    mProjectMenu->addAction(mCloseProjectAction);

    mProjectMenu->addSeparator();

    mProjectMenu->addAction(mProjectSaveAction);
    mProjectMenu->addAction(mProjectSaveAsAction);

    mProjectMenu->addSeparator();

    //mProjectMenu->addAction(mProjectExportAction);
    mProjectMenu->setFont(ft);

    //-----------------------------------------------------------------
    // Edit menu
    //-----------------------------------------------------------------
    mEditMenu = menuBar()->addMenu(tr("Edit"));

    mEditMenu->addAction(mUndoAction);
    mEditMenu->addAction(mRedoAction);
    mEditMenu->addAction(mSelectAllAction);
    mEditMenu->setFont(ft);

    //-----------------------------------------------------------------
    // MCMC menu
    //-----------------------------------------------------------------
    mMCMCMenu = menuBar()->addMenu(tr("MCMC"));
    mMCMCMenu->addAction(mRunAction);
    mMCMCMenu->addAction(mMCMCSettingsAction);
    mMCMCMenu->addSeparator();
    mMCMCMenu->addAction(mResetMCMCAction);

    //-----------------------------------------------------------------
    // View menu
    //-----------------------------------------------------------------
    mViewMenu = menuBar()->addMenu(tr("View"));
    mViewMenu->addAction(mViewModelAction);
    mViewMenu->addAction(mViewResultsAction);
    mViewMenu->addAction(mViewLogAction);

    //-----------------------------------------------------------------
    // Grouped Actions Menu
    //-----------------------------------------------------------------
    mActionsMenu = menuBar()->addMenu(tr("Actions"));
    mActionsMenu->addAction((mSelectEventsNameAction));
    mActionsMenu->addAction(mSelectEventsAction);
    mActionsMenu->addSeparator();
    mActionsMenu->addAction(mEventsColorAction);
    mActionsMenu->addAction(mEventsMethodAction);
    mActionsMenu->addAction(mDatesMethodAction);
    mActionsMenu->addSeparator();

    for (int i=0; i<mDatesActions.size(); ++i)
        mActionsMenu->addAction(mDatesActions[i]);

    mActionsMenu->addSeparator();
    mActionsMenu->addAction(mRescaleCurveAction);

    //-----------------------------------------------------------------
    // Help/About Menu this menu depend of the system. On MacOs it's in Chronomodel menu
    //-----------------------------------------------------------------
    mHelpMenu = menuBar()->addMenu(tr("About"));
    mHelpMenu->menuAction()->setShortcut(Qt::Key_Question);
    mHelpMenu->addAction(mAboutAct);
    mHelpMenu->addAction(mAboutQtAct);

}
/**
 * @brief MainWindow::createToolBars
 * Create the ToolBar with the icons, under the application menu
 */
void MainWindow::createToolBars()
{
    //-----------------------------------------------------------------
    // Main ToolBar
    //-----------------------------------------------------------------


    mToolBar = addToolBar("Main Tool Bar"); /* all types of tool button */
    //QString sty = palette().text().color().name(); // find the color defined by the system/theme
   // mToolBar->setStyleSheet("QToolButton { color :"+ sty +" ;}");

    mToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);//ToolButtonTextUnderIcon); // offer to write the text under the icon
    mToolBar->setMovable(false);
    mToolBar->setAllowedAreas(Qt::TopToolBarArea);

    mToolBar->addAction(mNewProjectAction);
    mToolBar->addAction(mOpenProjectAction);
    mToolBar->addAction(mProjectSaveAction);
    //mToolBar->addAction(mProjectExportAction);

    mToolBar->addAction(mUndoAction);
    mToolBar->addAction(mRedoAction);

    QWidget* separator3 = new QWidget(this);
    separator3->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mToolBar->addWidget(separator3);

    mToolBar->addAction(mCurveAction);
    mToolBar->addAction(mViewModelAction);
    mToolBar->addAction(mMCMCSettingsAction);
    mToolBar->addAction(mRunAction);
    mToolBar->addAction(mViewResultsAction);
    mToolBar->addAction(mViewLogAction);

    QWidget* separator4 = new QWidget(this);
    separator4->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mToolBar->addWidget(separator4);

    mToolBar->addAction(mHelpAction);
    mToolBar->addAction(mManualAction);
    mToolBar->addAction(mWebsiteAction);
    /* toolBar->addAction(mAboutAct);
    toolBar->addAction(mAboutQtAct); */
    mToolBar->setFont(qApp->font()); // must be after all addAction

}


// -----------

void MainWindow::newProject()
{
    // Ask to save the previous project.
    // Return true if the project doesn't need to be saved.
    // Returns true if the user saves the project or if the user doesn't want to save it.
    // Returns false if the user cancels.
    bool yesCreate = false;

    if ((mProject == nullptr) || (mProject->mState == Project::emptyState()) || (mProject->askToSave(tr("Save current project as...") )))
        yesCreate= true;

    if (yesCreate) {
        if (mProject->mModel != nullptr && !mProject->mModel->mEvents.empty()) {
            if (mProjectView->mResultsView)
                mProjectView->mResultsView->clearResults();
            mProject->clear_and_shrink_model();
            mProject->clear_calibCurves();
        }
        mProject.reset(new Project());

        // just update mAutoSaveTimer to avoid open the save() dialog box
        mProject-> mAutoSaveTimer->stop();

        /* Ask to save the new project.
         * Returns true only if a new file is created.
         * Note : at this point, the project state is still the previous project state.*/
        if (mProject->saveAs(tr("Save new project as..."))) {

            // resetInterface Disconnect also the scene
            resetInterface();


            /* Reset the project state and the MCMC Setting to the default value
             * and then send a notification to update the views : send desabled */

            mProject->initState(NEW_PROJECT_REASON);// emit showStudyPeriodWarning();


            activateInterface(true); // mProject doit exister

            // Create project connections
            connectProject();

            // Apply app settings to the project
            mProject->setAppSettingsAutoSave();

            // Send the project to the views
            mProjectView->setProject();

            // Ask for the a Study Period (open dialog)
            mProjectView->newPeriod();

            // Open the Model View
            mViewModelAction->trigger();

            // Disable the Result View
            mViewResultsAction->setEnabled(false);

            updateWindowTitle();

            mUndoStack->clear();

            if (AppSettings::mAutoSave) {
                mProject->mAutoSaveTimer->setInterval(AppSettings::mAutoSaveDelay * 1000);
                mProject->mAutoSaveTimer->start();

            } else
                mProject->mAutoSaveTimer->stop();

        }
    }
}

void MainWindow::openProject()
{
    const QString currentPath = getCurrentPath();
    // Qt keeps the QFileDialog window in memory for later. This is normal operation
    const QString path = QFileDialog::getOpenFileName(this,
                                                      tr("Open File"),
                                                      currentPath,
                                                      tr("Chronomodel Project (*.chr)"));



    if (!path.isEmpty()) {
        if (mProject != nullptr) { // if project is closed
            if (mProject->mState != Project::emptyState()) {
                mProject->askToSave(tr("Save current project as..."));

                disconnectProject();

                //resetInterface(): clear mEventsScene and mPhasesScene
                resetInterface(); // do mResultsView->clearResults();

            }
            else if (mProjectView->mResultsView)
                    mProjectView->mResultsView->clearResults();

            if (mProject->mModel != nullptr && !mProject->mModel->mEvents.empty()) {
                mProject->mModel->clear_and_shrink();
            }
            mProject->clear_calibCurves();
        }

        // assign new project
        if (mProject == nullptr || mProject->mState.isEmpty() || mProject->mState != Project::emptyState())
            mProject.reset(new Project());

        connectProject();

        mProject->setAppSettingsAutoSave();
        const QFileInfo info(path);
        setCurrentPath(info.absolutePath());

        // look MainWindows::readSetting()
        statusBar()->showMessage(tr("Loading project : %1").arg(path));
        if (mProject->load(path) == true) { // load() set AppSettings::mIsSaved

            if (mProjectView->mResultsView)
                mProjectView->mResultsView->clearResults();

            activateInterface(true);

            // Create mEventsScene and mPhasesScenes
            AppSettings::mIsSaved = true;
            if ( !mProject->mModel->mChains.empty()) {
                mcmcFinished(); //do initDensities()
                mProjectView->mModelView->setProject(); // build scene
                connect(mProject.get(), &Project::mcmcStarted, mProjectView->mResultsView, &ResultsView::clearResults);

            } else {
                mProjectView->setProject();
            }

            mProject->pushProjectState(mProject->mState, PROJECT_LOADED_REASON, true);

            updateWindowTitle();

            if (AppSettings::mAutoSave) {
                mProject->mAutoSaveTimer->setInterval(AppSettings::mAutoSaveDelay * 1000);
                mProject->mAutoSaveTimer->start();

            } else
                mProject->mAutoSaveTimer->stop();
         }

        mUndoStack->clear();
        statusBar()->showMessage(tr("Ready"));
    }

}


void MainWindow::insertProject()
{
    const QString currentPath = getCurrentPath();
    QString path = QFileDialog::getOpenFileName(this,
                                                      tr("Insert File"),
                                                      currentPath,
                                                      tr("Chronomodel Project (*.chr)"));

    if (!path.isEmpty()) {

        statusBar()->showMessage(tr("Insert project : %1").arg(path));

        const QFileInfo info(path);
        setCurrentPath(info.absolutePath());

        // look MainWindows::readSetting()
        QJsonObject new_state;
        if (mProject->insert(path, new_state)) {
            mProjectView->setShowAllThumbs(true);
            mProject->pushProjectState(new_state, INSERT_PROJECT_REASON, true);

        }

        //mUndoStack->clear();
        statusBar()->showMessage(tr("Ready"));
    }

}

void MainWindow::connectProject()
{
    connect(mProject.get(), &Project::noResult, this, &MainWindow::noResult);
    connect(mProject.get(), &Project::mcmcFinished, this, &MainWindow::mcmcFinished);
    connect(mProject.get(), &Project::projectStateChanged, this, &MainWindow::updateProject);

    connect(mMCMCSettingsAction, &QAction::triggered, mProject.get(), &Project::mcmcSettings);
    connect(mResetMCMCAction, &QAction::triggered, mProject.get(), &Project::resetMCMC);

    connect(mRunAction, &QAction::triggered, mProject.get(), &Project::run);
}

void MainWindow::disconnectProject()
{
    disconnect(mProject.get(), &Project::noResult, this, &MainWindow::noResult);
    disconnect(mProject.get(), &Project::mcmcFinished, this, &MainWindow::mcmcFinished);
    disconnect(mProject.get(), &Project::projectStateChanged, this, &MainWindow::updateProject);

    disconnect(mMCMCSettingsAction, &QAction::triggered, mProject.get(), &Project::mcmcSettings);
    disconnect(mResetMCMCAction, &QAction::triggered, mProject.get(), &Project::resetMCMC);

    disconnect(mRunAction, &QAction::triggered, mProject.get(), &Project::run);


}

void MainWindow::closeProject()
{
   if (mProject) {
       mProject->askToSave(tr("Save current project as...")); // Saver if anserd is Yes
        /*if ( mProject->askToSave(tr("Save current project as...")) == true)
             mProject->saveProjectToFile(); */

        mUndoStack->clear();

        mProject->initState(CLOSE_PROJECT_REASON);
        mProject->mAutoSaveTimer->stop();

        AppSettings::mLastDir = QString();
        AppSettings::mLastFile = QString();

        // Go back to model tab :
        mViewModelAction->trigger();

        disconnectProject();

        clearInterface();

        activateInterface(false);
        mViewResultsAction->setEnabled(false);
        if (mProjectView->mResultsView)
            mProjectView->mResultsView->clearResults();
        //delete mProjectView->mResultsView;
        mProject->clear_and_shrink_model();
        mProject->clear_calibCurves();
        mProject.reset();
        updateWindowTitle();



   } else // if there is no project, we suppose it means to close the programm
       QApplication::exit(0);
}

void MainWindow::saveProject()
{
    mProject->save();
}

void MainWindow::saveProjectAs()
{
    mProject->saveAs(tr("Save current project as..."));
}

void MainWindow::updateWindowTitle()
{
    const QString saved_sign = AppSettings::mIsSaved ?  " ✓ " : QString(" ● ");

#ifdef DEBUG
    #ifdef Q_OS_WIN
        const QString file_name = (AppSettings::mLastFile.isEmpty() ?  "" : QString(" - ") + AppSettings::mLastFile  + saved_sign);
    #else
        const QString file_name = " DEBUG Mode " + (AppSettings::mLastFile.isEmpty() ?  "" : QString(" - ") + AppSettings::mLastFile  + saved_sign);
    #endif
#else
    const QString file_name = (AppSettings::mLastFile.isEmpty() ?  "" : QString(" - ") + AppSettings::mLastFile + saved_sign);
#endif

#ifdef Q_OS_WIN
    setWindowTitle(file_name);

#else
    std::cout << "MainWindow::updateWindowTitle] version " << VERSION_STRING << std::endl;
    setWindowTitle(qApp->applicationName() + " v" + VERSION_STRING + " " + file_name);// see main.cpp for the application name
#endif
}

/**
 * @brief MainWindow::updateProject come from undo and redo action and Project::projectStateChanged() and Project::pushProjectState(
 */
void MainWindow::updateProject()
{
    qDebug()<<"[MainWindow::updateProject]";
   /*
    if (mProject->structureIsChanged())
        noResult(); // done by push
*/
    mRunAction->setEnabled(true);
    mProjectView->updateProject();
    updateWindowTitle();

}

void MainWindow::toggleCurve(bool checked)
{
    mProjectView->toggleCurve(checked);
}

#pragma mark Settings & About
void MainWindow::about()
{
    AboutDialog dialog(qApp->activeWindow());
    dialog.exec();
}

void MainWindow::appSettings()
{
    AppSettingsDialog dialog(qApp->activeWindow());

    dialog.setSettings();
    connect(&dialog, &AppSettingsDialog::settingsChanged, this, &MainWindow::updateAppSettings);
    connect(&dialog, &AppSettingsDialog::settingsFilesChanged, this, &MainWindow::setAppFilesSettings);
    dialog.exec();

}

void MainWindow::setAppFilesSettings()
{
    QLocale::Language newLanguage = AppSettings::mLanguage;

#if QT_DEPRECATED_SINCE(6, 6)
    QLocale::Territory newCountry= AppSettings::mCountry;
#else
    QLocale::Country newCountry= AppSettings::mCountry;
#endif

    QLocale newLoc = QLocale(newLanguage, newCountry);
    newLoc.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale::setDefault(newLoc);
    //statusBar()->showMessage(tr("Language") + " : " + QLocale::languageToString(QLocale().language()));

    QFont tooltipFont(font());
    tooltipFont.setItalic(true);

    QToolTip::setFont(tooltipFont);

    if (mProject) {
        mProject->setAppSettingsAutoSave();
        mProjectView->applyFilesSettings(mProject->mModel);
    }
    writeSettings();
}

void MainWindow::setAppSettings()
{
    QLocale::Language newLanguage = AppSettings::mLanguage;

#if QT_DEPRECATED_SINCE(6, 6)
    QLocale::Territory newCountry= AppSettings::mCountry;
#else
    QLocale::Country newCountry= AppSettings::mCountry;
#endif

    QLocale newLoc = QLocale(newLanguage, newCountry);
    newLoc.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale::setDefault(newLoc);

    setFont(qApp->font());

    QFont tooltipFont(font());
    tooltipFont.setItalic(true);

    QToolTip::setFont(tooltipFont);

    if (mProject) {
        mProject->setAppSettingsAutoSave();
    }
    writeSettings();
}

void MainWindow::updateAppSettings()
{
    QLocale::Language newLanguage = AppSettings::mLanguage;

#if QT_DEPRECATED_SINCE(6, 6)
    QLocale::Territory newCountry= AppSettings::mCountry;
#else
    QLocale::Country newCountry= AppSettings::mCountry;
#endif

    QLocale newLoc = QLocale(newLanguage, newCountry);
    newLoc.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale::setDefault(newLoc);

    setFont(qApp->font());

    QFont tooltipFont(font());
    tooltipFont.setItalic(true);

    QToolTip::setFont(tooltipFont);

    if (mProject) {
        mProject->setAppSettingsAutoSave();
        mProjectView->updateMultiCalibrationAndEventProperties();
        mProjectView->applySettings(mProject->mModel);

    }
    writeSettings();
}

void MainWindow::openManual()
{
    QDesktopServices::openUrl(QUrl("https://chronomodel.com/storage/medias/83_chronomodel_v32_user_manual_2024_05_13_min.pdf", QUrl::TolerantMode));

}

void MainWindow::showHelp(bool show)
{
    /*
    if (show)
        QWhatsThis::enterWhatsThisMode();
    else
        QWhatsThis::leaveWhatsThisMode();*/

    AppSettings::mShowHelp = show;
    mProjectView->showHelp(show);

}

void MainWindow::openWebsite()
{
    QDesktopServices::openUrl(QUrl("http://chronomodel.com", QUrl::TolerantMode));
}

void MainWindow::setFont(const QFont &font)
{
    mToolBar->setFont(font);
    mProjectView->setFont(font);
    mUndoView->setFont(font);
    mUndoDock->setFont(font);
}

#pragma mark Language
void MainWindow::setLanguage(QAction* action)
{
    QString lang = action->data().toString();
    QLocale locale = QLocale(lang);
    QLocale::setDefault(locale);
/*
    QTranslator qtTranslator;
    if (qtTranslator.load("qt_" + locale.name(), QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        QCoreApplication::installTranslator(&qtTranslator);

    QTranslator translator;
    if (translator.load(locale, ":/Chronomodel", "_")) {
        qDebug() << "-> Locale set to : " << QLocale::languageToString(locale.language()) << "(" << locale.name() << ")";
        qApp->installTranslator(&translator);
    }
*/
}

#pragma mark Grouped Actions
void MainWindow::selectAllEvents() {
    if (mProject && !mProject->mState.value(STATE_EVENTS).toArray().isEmpty()) {
        mProject->selectAllEvents();
        mProjectView->eventsAreSelected();
    }

}

void MainWindow::selectEventInSelectedPhases() {
    if (mProject)
        if (mProject->selectEventsFromSelectedPhases())
            mProjectView->eventsAreSelected();
}

void MainWindow::selectEventWithString()
{
    if (mProject) {
        bool ok;
        const QString text = QInputDialog::getText(this, tr("Find events containing the text"),
                                              tr("Text to search"), QLineEdit::Normal, QString(), &ok);
         if (ok && !text.isEmpty())
            if (mProject->selectedEventsWithString(text) )
                mProjectView->eventsAreSelected();
    }
}

void MainWindow::rebuildExportCurve()
{
    if (!mProject || !mProject->isCurve() || !mProject->mModel
        || mProject->mModel->mPosteriorMeanG.gx.vecG.empty())
        return;


    /*ComputeY is different from displayY,
     * for example for the vector case, you have to compute on the 3 components, but display only 2*/

    std::shared_ptr<ModelCurve> curveModel = mProject->mModel;


    // Setting actual minmax value
    std::vector<std::pair<double, double>> tabMinMax;
    tabMinMax.push_back(curveModel->mPosteriorMeanG.gx.mapG.minMaxY());
    if (curveModel->compute_Y)
        tabMinMax.push_back(curveModel->mPosteriorMeanG.gy.mapG.minMaxY());

    if (curveModel->compute_XYZ)
        tabMinMax.push_back(curveModel->mPosteriorMeanG.gz.mapG.minMaxY());

    std::vector<std::pair<double, double>> tabMinMaxGP;
    tabMinMaxGP.push_back(curveModel->mPosteriorMeanG.gx.mapGP.minMaxY());
    if (curveModel->compute_Y)
        tabMinMaxGP.push_back(curveModel->mPosteriorMeanG.gy.mapGP.minMaxY());
    if (curveModel->compute_XYZ)
        tabMinMaxGP.push_back(curveModel->mPosteriorMeanG.gz.mapGP.minMaxY());


    std::pair<unsigned, unsigned> mapSizeXY = std::pair<unsigned, unsigned> {curveModel->mPosteriorMeanG.gx.mapG.column(), curveModel->mPosteriorMeanG.gx.mapG.row()};

    // Display Rebuild Window
#ifdef DEBUG
    //std::pair<double, double> minMaxPFilter (curveModel->mPosteriorMeanG.gx.mapGP.rangeY.first*10, curveModel->mPosteriorMeanG.gx.mapGP.rangeY.second*10);
    std::pair<double, double> minMaxPFilter (-INFINITY, +INFINITY);

    RebuildCurveDialog qDialog = RebuildCurveDialog(curveModel->getCurvesName(), &tabMinMax, &tabMinMaxGP, &minMaxPFilter, mapSizeXY);

#else
    RebuildCurveDialog qDialog = RebuildCurveDialog(curveModel->getCurvesName(), &tabMinMax, &tabMinMaxGP, mapSizeXY);
#endif

    if (qDialog.exec()) {

        auto newMapSizeXY = qDialog.getMapSize();
        const int XGrid = newMapSizeXY.first;
        const int YGrid = newMapSizeXY.second;
        tabMinMax = qDialog.getYTabMinMax();
        tabMinMaxGP = qDialog.getYpTabMinMax();
#ifdef DEBUG
        minMaxPFilter = qDialog.getYpMinMaxFilter();
#endif
        // ____

        const auto &runTrace = curveModel->fullRunSplineTrace(curveModel->mChains);

        // init Posterior MeanG and map

        PosteriorMeanGComposante clearCompo;
        clearCompo.mapG = CurveMap (YGrid, XGrid); // Attention invesion ->explicit CurveMap(unsigned row, unsigned col)
        clearCompo.mapG.setRangeX(curveModel->mSettings.mTmin, curveModel->mSettings.mTmax);
        clearCompo.mapG.setMinValue(+INFINITY);
        clearCompo.mapG.setMaxValue(0);

        clearCompo.mapGP = CurveMap (YGrid, XGrid); // Attention invesion ->explicit CurveMap(unsigned row, unsigned col)
        clearCompo.mapGP.setRangeX(curveModel->mSettings.mTmin, curveModel->mSettings.mTmax);
        clearCompo.mapGP.setMinValue(+INFINITY);
        clearCompo.mapGP.setMaxValue(0);

        clearCompo.vecG = std::vector<double> (XGrid);
        clearCompo.vecGP = std::vector<double> (XGrid);
        clearCompo.vecGS = std::vector<double> (XGrid);
        clearCompo.vecVarG = std::vector<double> (XGrid);
        clearCompo.vecVarianceG = std::vector<double> (XGrid);
        clearCompo.vecVarErrG = std::vector<double> (XGrid);

        PosteriorMeanG meanG;
        meanG.gx = clearCompo;
        meanG.gx.mapG.setRangeY(tabMinMax[0].first, tabMinMax[0].second);
        meanG.gx.mapGP.setRangeY(tabMinMaxGP[0].first, tabMinMaxGP[0].second);

        if (curveModel->compute_Y) {
            meanG.gy = clearCompo;
            meanG.gy.mapG.setRangeY(tabMinMax[1].first, tabMinMax[1].second);
            meanG.gy.mapGP.setRangeY(tabMinMaxGP[1].first, tabMinMaxGP[1].second);
            if (curveModel->compute_XYZ) {
                meanG.gz = clearCompo;
                meanG.gz.mapG.setRangeY(tabMinMax[2].first, tabMinMax[2].second);
                meanG.gz.mapGP.setRangeY(tabMinMaxGP[2].first, tabMinMaxGP[2].second);
            }
        }

        int totalIterAccepted = 1;
        if (!curveModel->compute_Y) {
            for (auto &splineXYZ : runTrace) {
#ifdef DEBUG

                curveModel->memo_PosteriorG_filtering(meanG.gx, splineXYZ.splineX, totalIterAccepted, minMaxPFilter );
#else
                curveModel->memo_PosteriorG(meanG.gx, splineXYZ.splineX,  totalIterAccepted );
#endif

                totalIterAccepted++;
            }

        } else {
            for (auto &splineXYZ : runTrace) {
#if VERSION_MAJOR == 3 && VERSION_MINOR == 3 && VERSION_PATCH >= 5
                curveModel->memo_PosteriorG_3D_335(meanG, splineXYZ, curveModel->mCurveSettings.mProcessType,  totalIterAccepted );
#else
                curveModel->memo_PosteriorG_3D(meanG, splineXYZ, curveModel->mCurveSettings.mProcessType,  totalIterAccepted );
#endif
                totalIterAccepted++;
            }
        }

        meanG.gx.mapG.setMinValue(*std::min_element(meanG.gx.mapG.begin(), meanG.gx.mapG.end()));
        meanG.gx.mapGP.setMinValue(*std::min_element(meanG.gx.mapGP.begin(), meanG.gx.mapGP.end()));

        if (curveModel->compute_Y) {
            meanG.gy.mapG.setMinValue(*std::min_element(meanG.gy.mapG.begin(), meanG.gy.mapG.end()));
            meanG.gy.mapGP.setMinValue(*std::min_element(meanG.gy.mapGP.begin(), meanG.gy.mapGP.end()));

            if (curveModel->compute_XYZ) {
                meanG.gz.mapG.setMinValue(*std::min_element(meanG.gz.mapG.begin(), meanG.gz.mapG.end()));
                meanG.gz.mapGP.setMinValue(*std::min_element(meanG.gz.mapGP.begin(), meanG.gz.mapGP.end()));
            }
        }
        curveModel->mPosteriorMeanG = std::move(meanG);

        // update mPosteriorMeanGByChain
        for (size_t i = 0; i<curveModel->mChains.size(); i++) {
            const auto &runTraceByChain = curveModel->runSplineTraceForChain(curveModel->mChains, i);
            PosteriorMeanG meanGByChain;
            meanGByChain.gx = clearCompo;
            meanGByChain.gx.mapG.setRangeY(tabMinMax[0].first, tabMinMax[0].second);
            meanGByChain.gx.mapGP.setRangeY(tabMinMaxGP[0].first, tabMinMaxGP[0].second);

            if (curveModel->compute_Y) {
                meanGByChain.gy = clearCompo;
                meanGByChain.gy.mapG.setRangeY(tabMinMax[1].first, tabMinMax[1].second);
                meanGByChain.gy.mapGP.setRangeY(tabMinMaxGP[1].first, tabMinMaxGP[1].second);
                if (curveModel->compute_XYZ) {
                    meanGByChain.gz = clearCompo;
                    meanGByChain.gz.mapG.setRangeY(tabMinMax[2].first, tabMinMax[2].second);
                    meanGByChain.gz.mapGP.setRangeY(tabMinMaxGP[2].first, tabMinMaxGP[2].second);
                }
            }

            int totalIterAccepted = 1;
            if (!curveModel->compute_Y) {
                for (auto &splineXYZ : runTraceByChain) {

#ifdef DEBUG
                    curveModel->memo_PosteriorG_filtering(meanGByChain.gx, splineXYZ.splineX, totalIterAccepted, minMaxPFilter );
#else
                    curveModel->memo_PosteriorG(meanGByChain.gx, splineXYZ.splineX, totalIterAccepted );
#endif
                    totalIterAccepted++;
                }

            } else {
                for (auto &splineXYZ : runTraceByChain) {
#if VERSION_MAJOR == 3 && VERSION_MINOR == 3 && VERSION_PATCH >= 5
                    curveModel->memo_PosteriorG_3D_335(meanGByChain, splineXYZ, curveModel->mCurveSettings.mProcessType,  totalIterAccepted );
#else
                    curveModel->memo_PosteriorG_3D(meanGByChain, splineXYZ, curveModel->mCurveSettings.mProcessType,  totalIterAccepted );
#endif
                    totalIterAccepted++;
                }
            }

            meanGByChain.gx.mapG.setMinValue(*std::min_element(meanGByChain.gx.mapG.begin(), meanGByChain.gx.mapG.end()));
            meanGByChain.gx.mapGP.setMinValue(*std::min_element(meanGByChain.gx.mapGP.begin(), meanGByChain.gx.mapGP.end()));

            if (curveModel->compute_Y) {
                meanGByChain.gy.mapG.setMinValue( *std::min_element(meanGByChain.gy.mapG.begin(), meanGByChain.gy.mapG.end()));
                meanGByChain.gy.mapGP.setMinValue(*std::min_element(meanGByChain.gy.mapGP.begin(), meanGByChain.gy.mapGP.end()));

                if (curveModel->compute_XYZ) {
                    meanGByChain.gz.mapG.setMinValue(*std::min_element(meanGByChain.gz.mapG.begin(), meanGByChain.gz.mapG.end()));
                    meanGByChain.gz.mapGP.setMinValue(*std::min_element(meanGByChain.gz.mapGP.begin(), meanGByChain.gz.mapGP.end()));
                }
            }

            curveModel->mPosteriorMeanGByChain[i] = std::move(meanGByChain);
        }

        // update ResultView
        mProjectView->updateResults();

    } else {
        return;
    }


}

void MainWindow::changeEventsColor()
{
    if (!mProject)
        return;

    const QColor color = QColorDialog::getColor(Qt::blue, qApp->activeWindow(), tr("Change Selected Events Colour"));
    if (color.isValid() && mProject)
        mProject->updateSelectedEventsColor(color);

}

void MainWindow::changeEventsMethod()
{
    if (!mProject || mProject->isCurve())
        return;

    QStringList opts;
    opts.append(MHVariable::getSamplerProposalText(MHVariable::eMHAdaptGauss));
    opts.append(MHVariable::getSamplerProposalText(MHVariable::eBoxMuller));
    opts.append(MHVariable::getSamplerProposalText(MHVariable::eDoubleExp));

    bool ok;
    QString methodStr = QInputDialog::getItem(qApp->activeWindow(),
                                          tr("Change Events MCMC"),
                                          tr("Change Event MCMC"),
                                          opts, 0, false, &ok);
    if (ok && !methodStr.isEmpty()) {
        MHVariable::SamplerProposal method = MHVariable::getSamplerProposalFromText(methodStr);
        mProject->updateSelectedEventsMethod(method);
    }
}

void MainWindow::changeDatesMethod()
{
    if (!mProject)
        return;

    QStringList opts;
    const QList<PluginAbstract*>& plugins = PluginManager::getPlugins();
    for (int i=0; i<plugins.size(); ++i)
        opts.append(plugins[i]->getName());

    bool ok;
    QString pluginName = QInputDialog::getItem(qApp->activeWindow(),
                                             tr("Change Data MCMC"),
                                             tr("For which type of data do you want to change the sampler ?"),
                                             opts, 0, false, &ok);
    if (ok) {
        opts.clear();
        opts.append(MHVariable::getSamplerProposalText(MHVariable::eMHPrior));
        opts.append(MHVariable::getSamplerProposalText(MHVariable::eInversion));
        opts.append(MHVariable::getSamplerProposalText(MHVariable::eMHAdaptGauss));

        QString methodStr = QInputDialog::getItem(qApp->activeWindow(),
                                                  tr("Change Data MCMC"),
                                                  tr("Change Data MCMC"),
                                                  opts, 0, false, &ok);
        if (ok && !methodStr.isEmpty()) {
            MHVariable::SamplerProposal method = MHVariable::getSamplerProposalFromText(methodStr);
            PluginAbstract* plugin =PluginManager::getPluginFromName(pluginName);
            QString pluginId = plugin->getId();
            mProject->updateSelectedEventsDataMethod(method, pluginId);
        }
    }
}

void MainWindow::doGroupedAction()
{
    if (!mProject)
        return;

    QAction* act = qobject_cast<QAction*>(sender());
    QVariant groupedActionVariant = act->data();
    QHash<QString, QVariant> groupedAction = groupedActionVariant.toHash();

    if (groupedAction["inputType"] == "combo") {
        bool ok;
        QString curve = QInputDialog::getItem(qApp->activeWindow(),
                                               groupedAction["title"].toString(),
                                               groupedAction["label"].toString(),
                                               groupedAction["items"].toStringList(),
                                               0, false, &ok);
        if (ok && !curve.isEmpty()) {
            groupedAction["value"] = curve;
            mProject->updateAllDataInSelectedEvents(groupedAction);
        }
    }
}

// Events
/**
 * @todo Fix app close event called twice when updating with Qt >= 5.6
 */
void MainWindow::closeEvent(QCloseEvent* e)
{
    if (mProject) {
        QMessageBox message(QMessageBox::Question,
                            QApplication::applicationName(),
                            tr("Do you really want to quit ChronoModel ?"),
                            QMessageBox::Yes | QMessageBox::No,
                            qApp->activeWindow());

        if (message.exec() == QMessageBox::Yes) {
            if (mProject->askToSave(tr("Save project before quitting?")) == true) {
                writeSettings();
                e->accept();

                // This is a temporary Qt bug fix (should be corrected by Qt 5.6 when released)
                // The close event is called twice on Mac when closing with "cmd + Q" key or with the "Quit Chronomodel" menu.
                QApplication::exit(0);
                //QGuiApplication::exit(0);
            } else
                e->ignore();

        } else
            e->ignore();
    } else {
        e->accept();

        // This is a temporary Qt bug fix (should be corrected by Qt 5.6 when released)
        // The close event is called twice on Mac when closing with "cmd + Q" key or with the "Quit Chronomodel" menu.
        QApplication::exit(0);
        //QGuiApplication::exit(0);
    }

}

void MainWindow::keyPressEvent(QKeyEvent* keyEvent)
{
    if (keyEvent->matches(QKeySequence::Undo)) {
        mUndoStack->undo();

    } else if (keyEvent->matches(QKeySequence::Redo)) {
        mUndoStack->redo();
    }

    QMainWindow::keyPressEvent(keyEvent);
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        // TODO : set every text that needs translation !!!
        qDebug() << "--> MainWindow language updated";
        mNewProjectAction->setText(tr("&New"));
    } else
        QMainWindow::changeEvent(event);

}

#pragma mark Settings
void MainWindow::writeSettings()
{
    mProjectView->writeSettings();
    AppSettings::mLastSize = size();
    AppSettings::mLastPosition = pos();

    AppSettings::writeSettings();

}

void MainWindow::readSettings(const QString& defaultFilePath)
{
    move( AppSettings::mLastPosition);
    if (AppSettings::mLastSize.width() >50)
        resize( AppSettings::mLastSize);
    else
        resize( QSize(400, 400));

    mProjectView->showHelp(false);
    mHelpAction->setChecked(AppSettings::mShowHelp);

    if (defaultFilePath != "") {
        QFileInfo fileInfo(defaultFilePath);
        if (fileInfo.isFile()) {
            if (!mProject)
                mProject.reset(new Project());

            else if (mProject->mModel) {
                mProject->mModel->clear();
                mProjectView->resetInterface();
            }
            std::cout << "[MainWindow::readSettings]" << std::endl;

            if (mProject->load(defaultFilePath)) {
                activateInterface(true);               
                connectProject();
                std::cout << "[MainWindow::readSettings] mProject->load = true" << std::endl;
                std::cout << "[MainWindow::readSettings] (mProject->mModel == nullptr) = " << (mProject->mModel == nullptr) << std::endl;

                AppSettings::mIsSaved = true;

                if (mProject->withResults()) {
                    std::cout << "[MainWindow::readSettings] mProject->withResults = true" << std::endl;
                    mcmcFinished(); //do initDensities()

                } else {
                    std::cout << "[MainWindow::readSettings] mProject->withResults = false" << std::endl;
                }

                mProject->setAppSettingsAutoSave();

                mProjectView->setProject();

                mProject->pushProjectState(mProject->mState, PROJECT_LOADED_REASON, false); // notify false, sinon do updatProject and redo update()

                if (mProject->withResults()) {
                    mViewLogAction -> setEnabled(true);
                    mViewResultsAction -> setEnabled(true);
                    mViewResultsAction -> setChecked(true); // Just check the Result Button after computation and mResultsView is show after

                 }

            }
        }
    }

    setAppSettings();
    mProjectView->readSettings();

    if (mProject!=nullptr && mProject->mModel!=nullptr && (! mProject->mModel->mChains.empty()) ) {
        mProject->mModel->updateDesignFromJson();
        mProjectView->showResults();
   }
}

void MainWindow::clearInterface()
{
    mProjectView->clearInterface();

    mProjectSaveAction->setEnabled(false);
    mProjectSaveAsAction->setEnabled(false);

    mCurveAction->setEnabled(false);
    mViewModelAction->setEnabled(false);
    mMCMCSettingsAction->setEnabled(false);
    mResetMCMCAction->setEnabled(false);

    mSelectEventsAction->setEnabled(false);
    mEventsColorAction->setEnabled(false);
    mEventsMethodAction->setEnabled(false);
    mDatesMethodAction->setEnabled(false);
    for (auto&& act : mDatesActions)
        act->setEnabled(false);

    mRescaleCurveAction->setEnabled(false);
    mRunAction->setEnabled(false);

    mViewResultsAction->setEnabled(false);
    mViewLogAction->setEnabled(false);

}

void MainWindow::resetInterface()
{
    mProjectView->resetInterface(); 
}

void MainWindow::activateInterface(bool activate)
{
    mProjectView->setVisible(activate);

    mProjectSaveAction->setEnabled(activate);
    mProjectSaveAsAction->setEnabled(activate);

    mCurveAction->setEnabled(activate);
    mViewModelAction->setEnabled(activate);
    mMCMCSettingsAction->setEnabled(activate);
    mResetMCMCAction->setEnabled(activate);

    mSelectEventsAction->setEnabled(activate);
    mEventsColorAction->setEnabled(activate);
    mEventsMethodAction->setEnabled(activate && mProject->isCurve());
    mDatesMethodAction->setEnabled(activate);
    for (auto&& act : mDatesActions)
        act->setEnabled(activate);

    mRescaleCurveAction->setEnabled(activate && mProject->isCurve() && mProject->withResults());
    // Les actions suivantes doivent être désactivées si on ferme le projet.
    // Par contre, elles ne doivent pas être ré-activée dès l'ouverture d'un projet
    mRunAction->setEnabled(activate);

    if (activate && mProject->withResults()) {
        mViewResultsAction->setEnabled(activate);
        mViewLogAction->setEnabled(activate);

    } else {
        mViewResultsAction->setEnabled(false);
        mViewLogAction->setEnabled(false);
    }

}

void MainWindow::setRunEnabled(bool enabled)
{
    mRunAction->setEnabled(enabled);
}

void MainWindow::setResultsEnabled(bool enabled)
{
    mViewResultsAction->setEnabled(enabled);
}

void MainWindow::setLogEnabled(bool enabled)
{
    mViewLogAction->setEnabled(enabled);
}

void MainWindow::mcmcFinished()
{
    // Set Results and Log tabs enabled
    std::cout << "[MainWindow::mcmcFinished]" << std::endl;
    mViewLogAction->setEnabled(true);
    mViewResultsAction->setEnabled(true);

    // Should be elsewhere ?
    mProject->setWithResults(); // set to be able to save the file *.res

    // Just check the Result Button (the view will be shown by ProjectView::initResults below)
    mViewResultsAction->setChecked(true);

    mRescaleCurveAction->setEnabled(mProject->isCurve());

    // Tell the views to update
    mProjectView->initResults();



}

void MainWindow::noResult()
{
    mEventsMethodAction->setEnabled(!mProject->isCurve());
    mRescaleCurveAction->setEnabled(false);

    mViewLogAction -> setEnabled(false);
    mViewResultsAction -> setEnabled(false);
    mViewResultsAction -> setChecked(false);
    if (mProjectView->mResultsView)
        mProjectView->mResultsView->clearResults();

    mViewModelAction->trigger();
    mProject->setNoResults(true); // set to disable the saving the file *.res
    if (mProject->mModel != nullptr && !mProject->mModel->mEvents.empty()) {
        mProject->mModel->clear();
    }

}
