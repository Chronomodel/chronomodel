#include "MainWindow.h"
#include "ProjectManager.h"
#include "Project.h"
#include "ProjectView.h"
#include "../PluginAbstract.h"
#include "AboutDialog.h"
#include "SettingsDialog.h"
#include <QtWidgets>


MainWindow::MainWindow(QWidget* aParent):QMainWindow(aParent),
mProjectView(0),
mProject(0)
{
    setWindowTitle("Chronomodel");

    mCentralStack = new QStackedWidget();
    setCentralWidget(mCentralStack);

    mUndoView = new QUndoView(&ProjectManager::getUndoStack());
    mUndoView->setEmptyLabel(tr("Initial state"));
    mUndoDock = new QDockWidget(this);
    mUndoDock->setFixedWidth(250);
    mUndoDock->setWidget(mUndoView);
    mUndoDock->setAllowedAreas(Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, mUndoDock);
    mUndoDock->setVisible(false);
    
    createActions();
    createMenus();
    createToolBars();
    
    statusBar()->showMessage(tr("Ready"));
    setUnifiedTitleAndToolBarOnMac(true);
    setWindowIcon(QIcon(":chronomodel.png"));
    setMinimumSize(1000, 700);
}

void MainWindow::createActions()
{
    mAppSettingsAction = new QAction(QIcon(":settings.png"), tr("Settings"), this);
    connect(mAppSettingsAction, SIGNAL(triggered()), this, SLOT(appSettings()));
    
    //-----------------------------------------------------------------
    // Project Actions
    //-----------------------------------------------------------------
    
    mNewProjectAction = new QAction(QIcon(":new.png"), tr("&New"), this);
    mNewProjectAction->setShortcuts(QKeySequence::New);
    mNewProjectAction->setStatusTip(tr("Create a new project"));
    connect(mNewProjectAction, SIGNAL(triggered()), this, SLOT(newProject()));
    
    mOpenProjectAction = new QAction(QIcon(":open.png"), tr("Open"), this);
    mOpenProjectAction->setShortcuts(QKeySequence::Open);
    mOpenProjectAction->setStatusTip(tr("Open an existing project"));
    connect(mOpenProjectAction, SIGNAL(triggered()), this, SLOT(openProject()));
    
    mProjectCloseAction = new QAction(tr("Close"), this);
    mProjectCloseAction->setShortcuts(QKeySequence::Close);
    mProjectCloseAction->setEnabled(false);
    connect(mProjectCloseAction, SIGNAL(triggered()), this, SLOT(closeProject()));
    
    mProjectSaveAction = new QAction(QIcon(":save.png"), tr("&Save"), this);
    mProjectSaveAction->setShortcuts(QKeySequence::Save);
    mProjectSaveAction->setEnabled(false);
    
    mProjectSaveAsAction = new QAction(QIcon(":save.png"), tr("Save as..."), this);
    
    mProjectExportAction = new QAction(QIcon(":export.png"), tr("Export"), this);
    mProjectExportAction->setEnabled(false);
    mProjectExportAction->setVisible(false);
    
    mUndoAction = ProjectManager::getUndoStack().createUndoAction(this);
    mUndoAction->setIcon(QIcon(":undo.png"));
    mUndoAction->setText(tr("Undo"));
    
    mRedoAction = ProjectManager::getUndoStack().createRedoAction(this);
    mRedoAction->setIcon(QIcon(":redo.png"));
    
    mUndoViewAction = mUndoDock->toggleViewAction();
    mUndoViewAction->setText(tr("Show Undo Stack"));
    
    //-----------------------------------------------------------------
    // MCMC Actions
    //-----------------------------------------------------------------
    mMCMCSettingsAction = new QAction(QIcon(":settings.png"), tr("MCMC"), this);
    
    mRunAction = new QAction(QIcon(":run.png"), tr("Run"), this);
    //runAction->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaPlay));
    mRunAction->setIconText(tr("Run"));
    mRunAction->setIconVisibleInMenu(true);
    mRunAction->setToolTip(tr("Run Model"));
    
    //-----------------------------------------------------------------
    // View Actions
    //-----------------------------------------------------------------
    mViewModelAction = new QAction(QIcon(":model.png"), tr("Model"), this);
    mViewModelAction->setCheckable(true);
    
    mViewResultsAction = new QAction(QIcon(":results.png"), tr("Results"), this);
    mViewResultsAction->setCheckable(true);
    
    mViewLogAction = new QAction(QIcon(":results.png"), tr("Log"), this);
    mViewLogAction->setCheckable(true);
    
    mViewGroup = new QActionGroup(this);
    mViewGroup->addAction(mViewModelAction);
    mViewGroup->addAction(mViewResultsAction);
    mViewGroup->addAction(mViewLogAction);
    mViewModelAction->setChecked(true);
    
    //-----------------------------------------------------------------
    // Help/About Menu
    //-----------------------------------------------------------------
    mAboutAct = new QAction(QIcon(":light.png"), tr("About"), this);
    connect(mAboutAct, SIGNAL(triggered()), this, SLOT(about()));
    
    mAboutQtAct = new QAction(QIcon(":qt.png"), tr("About Qt"), this);
    connect(mAboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    
    mHelpAction = new QAction(QIcon(":help.png"), tr("Help"), this);
    mHelpAction->setCheckable(true);
    connect(mHelpAction, SIGNAL(toggled(bool)), this, SLOT(showHelp(bool)));
    
    mManualAction = new QAction(QIcon(":help.png"), tr("Manual"), this);
    connect(mManualAction, SIGNAL(triggered()), this, SLOT(openManual()));
    
    mWebsiteAction = new QAction(QIcon(":web.png"), tr("Website"), this);
    connect(mWebsiteAction, SIGNAL(triggered()), this, SLOT(openWebsite()));
}

void MainWindow::createMenus()
{
    //-----------------------------------------------------------------
    // Project menu
    //-----------------------------------------------------------------
    mProjectMenu = menuBar()->addMenu(tr("Project"));

    mProjectMenu->addAction(mAppSettingsAction);
    mProjectMenu->addAction(mNewProjectAction);
    mProjectMenu->addAction(mOpenProjectAction);
    mProjectMenu->addAction(mProjectCloseAction);

    mProjectMenu->addSeparator();

    mProjectMenu->addAction(mProjectSaveAction);
    mProjectMenu->addAction(mProjectSaveAsAction);
    
    mProjectMenu->addSeparator();

    mProjectMenu->addAction(mProjectExportAction);
    
    //-----------------------------------------------------------------
    // Edit menu
    //-----------------------------------------------------------------
    mEditMenu = menuBar()->addMenu(tr("Edit"));
    
    mEditMenu->addAction(mUndoAction);
    mEditMenu->addAction(mRedoAction);
    mEditMenu->addAction(mUndoViewAction);
    
    //-----------------------------------------------------------------
    // MCMC menu
    //-----------------------------------------------------------------
    mMCMCMenu = menuBar()->addMenu(tr("MCMC"));
    mMCMCMenu->addAction(mRunAction);
    mMCMCMenu->addAction(mMCMCSettingsAction);
    
    //-----------------------------------------------------------------
    // View menu
    //-----------------------------------------------------------------
    mViewMenu = menuBar()->addMenu(tr("View"));
    mViewMenu->addAction(mViewModelAction);
    mViewMenu->addAction(mViewResultsAction);
    mViewMenu->addAction(mViewLogAction);
    
    //-----------------------------------------------------------------
    // Help/About Menu
    //-----------------------------------------------------------------
    mHelpMenu = menuBar()->addMenu(tr("&Help"));
    mHelpMenu->addAction(mAboutAct);
    mHelpMenu->addAction(mAboutQtAct);
}

void MainWindow::createToolBars()
{
    //-----------------------------------------------------------------
    // Main ToolBar
    //-----------------------------------------------------------------
    QToolBar* toolBar = addToolBar("Main Tool Bar");
    toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolBar->setMovable(false);
    toolBar->setAllowedAreas(Qt::TopToolBarArea);
    
    toolBar->addAction(mNewProjectAction);
    toolBar->addAction(mOpenProjectAction);
    toolBar->addAction(mProjectSaveAction);
    toolBar->addAction(mProjectExportAction);
    
    toolBar->addAction(mUndoAction);
    toolBar->addAction(mRedoAction);
    
    QWidget* separator3 = new QWidget(this);
    separator3->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toolBar->addWidget(separator3);
    
    toolBar->addAction(mViewModelAction);
    toolBar->addAction(mMCMCSettingsAction);
    toolBar->addAction(mRunAction);
    toolBar->addAction(mViewResultsAction);
    toolBar->addAction(mViewLogAction);
    
    QWidget* separator4 = new QWidget(this);
    separator4->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toolBar->addWidget(separator4);
    
    toolBar->addAction(mHelpAction);
    toolBar->addAction(mManualAction);
    toolBar->addAction(mWebsiteAction);
    /*toolBar->addAction(mAboutAct);
    toolBar->addAction(mAboutQtAct);*/
}


// -----------

void MainWindow::openProject()
{
    QString path = QFileDialog::getOpenFileName(qApp->activeWindow(),
                                                tr("Open File"),
                                                ProjectManager::getCurrentPath(),
                                                tr("Chronomodel Project (*.chr)"));
    
    if(!path.isEmpty())
    {
        if(closeProject())
        {
            QFileInfo info(path);
            ProjectManager::setCurrentPath(info.absolutePath());
            Project* project = ProjectManager::newProject(false);
            
            setProject(project);
            project->load(path);
            
            updateWindowTitle();
        }
    }
}

void MainWindow::newProject()
{
    if(closeProject())
    {
        Project* project = ProjectManager::newProject(true);
        if(project)
            setProject(project);
    }
}

void MainWindow::setProject(Project* project)
{
    mProject = project;
    if(mProject)
    {
        connect(mProjectSaveAction, SIGNAL(triggered()), this, SLOT(saveProject()));
        connect(mProjectSaveAsAction, SIGNAL(triggered()), this, SLOT(saveProjectAs()));
        connect(mMCMCSettingsAction, SIGNAL(triggered()), mProject, SLOT(mcmcSettings()));
        connect(mProjectExportAction, SIGNAL(triggered()), mProject, SLOT(exportAsText()));
        connect(mRunAction, SIGNAL(triggered()), mProject, SLOT(run()));
        connect(mProject, SIGNAL(mcmcFinished(MCMCLoopMain&)), mViewResultsAction, SLOT(trigger()));
        
        mProjectSaveAction->setEnabled(true);
        mProjectCloseAction->setEnabled(true);
        mProjectExportAction->setEnabled(true);
        mMCMCSettingsAction->setEnabled(true);
        
        mProjectView = new ProjectView();
        mCentralStack->addWidget(mProjectView);
        mProjectView->updateProject();
        
        bool showHelp = ProjectManager::getSettings().mShowHelp;
        mProjectView->showHelp(showHelp);
        mHelpAction->setChecked(showHelp);
        
        updateWindowTitle();
        
        connect(mProject, SIGNAL(projectStateChanged()), mProjectView, SLOT(updateProject()));
        connect(mViewModelAction, SIGNAL(triggered()), mProjectView, SLOT(showModel()));
        connect(mViewResultsAction, SIGNAL(triggered()), mProjectView, SLOT(showResults()));
        connect(mViewLogAction, SIGNAL(triggered()), mProjectView, SLOT(showLog()));
        connect(mProject, SIGNAL(mcmcFinished(MCMCLoopMain&)), mProjectView, SLOT(updateLog(MCMCLoopMain&)));
    }
}

bool MainWindow::closeProject()
{
    if(mProject)
    {
        if(ProjectManager::saveProject())
        {
            disconnect(mProjectSaveAction, SIGNAL(triggered()), this, SLOT(saveProject()));
            disconnect(mProjectSaveAsAction, SIGNAL(triggered()), this, SLOT(saveProjectAs()));
            disconnect(mMCMCSettingsAction, SIGNAL(triggered()), mProject, SLOT(mcmcSettings()));
            disconnect(mProjectExportAction, SIGNAL(triggered()), mProject, SLOT(exportAsText()));
            disconnect(mRunAction, SIGNAL(triggered()), mProject, SLOT(run()));
            disconnect(mViewModelAction, SIGNAL(triggered()), mProjectView, SLOT(showModel()));
            
            disconnect(mViewResultsAction, SIGNAL(triggered()), mProjectView, SLOT(showResults()));
            disconnect(mViewLogAction, SIGNAL(triggered()), mProjectView, SLOT(showLog()));
            
            disconnect(mProject, SIGNAL(mcmcFinished(MCMCLoopMain&)), mViewResultsAction, SLOT(trigger()));
            disconnect(mProject, SIGNAL(mcmcFinished(MCMCLoopMain&)), mProjectView, SLOT(updateLog(MCMCLoopMain&)));
            
            mProjectSaveAction->setEnabled(false);
            mProjectCloseAction->setEnabled(false);
            mProjectExportAction->setEnabled(false);
            mMCMCSettingsAction->setEnabled(false);
            
            mCentralStack->removeWidget(mProjectView);
            delete mProjectView;
            mProjectView = 0;
            
            mProject = 0;
            
            ProjectManager::deleteProject();
            return true;
        }
        return false;
    }
    return true;
}

void MainWindow::saveProject()
{
    if(mProject)
    {
        mProject->save();
        updateWindowTitle();
    }
}

void MainWindow::saveProjectAs()
{
    if(mProject)
    {
        mProject->saveAs();
        updateWindowTitle();
    }
}

void MainWindow::updateWindowTitle()
{
    setWindowTitle(qApp->applicationName() + " " + qApp->applicationVersion() + (mProject ? QString(" - ") + mProject->mProjectFileName : ""));
}

void MainWindow::about()
{
    AboutDialog dialog;
    dialog.exec();
}

void MainWindow::appSettings()
{
    SettingsDialog dialog;
    dialog.setSettings(ProjectManager::getSettings());
    if(dialog.exec() == QDialog::Accepted)
    {
        ProjectManager::setSettings(dialog.getSettings());
    }
}

void MainWindow::openManual()
{
    QString path = qApp->applicationDirPath();
#ifdef Q_OS_MAC
    QDir dir(path);
    dir.cdUp();
    path = dir.absolutePath() + "/Resources";
#endif
    path += "/Chronomodel.pdf";
    QDesktopServices::openUrl(QUrl("file:///" + path, QUrl::TolerantMode));
}

void MainWindow::showHelp(bool show)
{
    Settings settings = ProjectManager::getSettings();
    settings.mShowHelp = show;
    ProjectManager::setSettings(settings);
    
    mProjectView->showHelp(show);
}

void MainWindow::openWebsite()
{
    QDesktopServices::openUrl(QUrl("http://www.chronomodel.com", QUrl::TolerantMode));
}

// ---------

void MainWindow::closeEvent(QCloseEvent* aEvent)
{
    writeSettings();
    ProjectManager::writeSettings();
    
    if(closeProject())
    {
        aEvent->accept();
    }
    else
    {
        aEvent->ignore();
    }
}

void MainWindow::keyPressEvent(QKeyEvent* keyEvent)
{
    if(keyEvent->matches(QKeySequence::Undo))
    {
        ProjectManager::getUndoStack().undo();
    }
    else if(keyEvent->matches(QKeySequence::Redo))
    {
        ProjectManager::getUndoStack().redo();
    }
    QMainWindow::keyPressEvent(keyEvent);
}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.beginGroup("MainWindow");
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.endGroup();
}

void MainWindow::readSettings()
{
    QSettings settings;
    settings.beginGroup("MainWindow");
    resize(settings.value("size", QSize(400, 400)).toSize());
    move(settings.value("pos", QPoint(200, 200)).toPoint());
    settings.endGroup();
}

