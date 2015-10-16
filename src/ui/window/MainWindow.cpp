#include "MainWindow.h"
#include "Project.h"
#include "ProjectView.h"
#include "../PluginAbstract.h"
#include "AboutDialog.h"
#include "AppSettingsDialog.h"
#include "PluginsSettingsDialog.h"
#include "PluginManager.h"
#include "ModelUtilities.h"
#include <QtWidgets>


#pragma mark constructor / destructor
MainWindow::MainWindow(QWidget* aParent):QMainWindow(aParent)
{
    setWindowTitle("ChronoModel");
    
    QPalette tooltipPalette;
    tooltipPalette.setColor(QPalette::ToolTipBase, Qt::white);
    tooltipPalette.setColor(QPalette::ToolTipText, Qt::black);
    QToolTip::setPalette(tooltipPalette);
    QFont tooltipFont;
    tooltipFont.setItalic(true);
    tooltipFont.setPointSizeF(11.f);
    QToolTip::setFont(tooltipFont);
    
    mLastPath = QDir::homePath();
    
    mProject = new Project();

    mProjectView = new ProjectView();
    setCentralWidget(mProjectView);

    mUndoStack = new QUndoStack();
    mUndoStack->setUndoLimit(1000);
    
    mUndoView = new QUndoView(mUndoStack);
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
    //setUnifiedTitleAndToolBarOnMac(true);
    setWindowIcon(QIcon(":chronomodel.png"));
    setMinimumSize(800, 500);
    
    connect(mProjectSaveAction, SIGNAL(triggered()), this, SLOT(saveProject()));
    connect(mProjectSaveAsAction, SIGNAL(triggered()), this, SLOT(saveProjectAs()));
    connect(mMCMCSettingsAction, SIGNAL(triggered()), mProject, SLOT(mcmcSettings()));
    connect(mResetMCMCAction, SIGNAL(triggered()), mProject, SLOT(resetMCMC()));
    connect(mProjectExportAction, SIGNAL(triggered()), mProject, SLOT(exportAsText()));
    connect(mRunAction, SIGNAL(triggered()), mProject, SLOT(run()));
    
    
    
    connect(mProject, SIGNAL(projectStateChanged()), this, SLOT(updateProject()));
    connect(mViewModelAction, SIGNAL(triggered()), mProjectView, SLOT(showModel()));
    
    connect(mViewLogAction, SIGNAL(triggered()), mProjectView, SLOT(showLog()));
    
    connect(mProject, SIGNAL(mcmcFinished(Model*)), this, SLOT(mcmcFinished(Model*)));
    
    connect(mViewResultsAction, SIGNAL(triggered()), mProjectView, SLOT(showResults()));
    
    
    
    mProjectView->doProjectConnections(mProject);
    

    QLocale newLoc(QLocale::system());
    mAppSettings.mLanguage = newLoc.language();
    mAppSettings.mCountry = newLoc.country();
    //QLocale newLoc = QLocale(mLanguage,mCountry);
    newLoc.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale::setDefault(newLoc);
    //if(mLanguage==QLocale::French) {
    if(newLoc.decimalPoint()==',') {
        mAppSettings.mCSVCellSeparator=";";
        mAppSettings.mCSVDecSeparator=",";
    }
    else {
        mAppSettings.mCSVCellSeparator=",";
        mAppSettings.mCSVDecSeparator=".";
    }


    activateInterface(false);
}

MainWindow::~MainWindow()
{
    
}

#pragma mark Accessors
Project* MainWindow::getProject()
{
    return mProject;
}

QJsonObject MainWindow::getState()
{
    return mProject->mState;
}

AppSettings MainWindow::getAppSettings() const
{
    return mAppSettings;
}

QUndoStack* MainWindow::getUndoStack()
{
    return mUndoStack;
}

QString MainWindow::getCurrentPath()
{
    return mLastPath;
}

void MainWindow::setCurrentPath(const QString& path)
{
    mLastPath = path;
}

#pragma mark Actions & Menus
void MainWindow::createActions()
{
    mAppSettingsAction = new QAction(QIcon(":settings.png"), tr("Settings"), this);
    connect(mAppSettingsAction, SIGNAL(triggered()), this, SLOT(appSettings()));
    
    //-----------------------------------------------------------------
    // Project Actions
    //-----------------------------------------------------------------
    
    mNewProjectAction = new QAction(QIcon(":new_p.png"), tr("&New"), this);
    mNewProjectAction->setShortcuts(QKeySequence::New);
    mNewProjectAction->setStatusTip(tr("Create a new project"));
    connect(mNewProjectAction, SIGNAL(triggered()), this, SLOT(newProject()));
    
    mOpenProjectAction = new QAction(QIcon(":open_p.png"), tr("Open"), this);
    mOpenProjectAction->setShortcuts(QKeySequence::Open);
    mOpenProjectAction->setStatusTip(tr("Open an existing project"));
    connect(mOpenProjectAction, SIGNAL(triggered()), this, SLOT(openProject()));
    
    mCloseProjectAction = new QAction(tr("Close"), this);
    mCloseProjectAction->setShortcuts(QKeySequence::Close);
    mCloseProjectAction->setStatusTip(tr("Open an existing project"));
    connect(mCloseProjectAction, SIGNAL(triggered()), this, SLOT(closeProject()));
    
    mProjectSaveAction = new QAction(QIcon(":save_p.png"), tr("&Save"), this);
    mProjectSaveAction->setShortcuts(QKeySequence::Save);
    
    mProjectSaveAsAction = new QAction(QIcon(":save_p.png"), tr("Save as..."), this);
    
    mProjectExportAction = new QAction(QIcon(":export.png"), tr("Export"), this);
    mProjectExportAction->setVisible(false);
    
    mUndoAction = mUndoStack->createUndoAction(this);
    mUndoAction->setIcon(QIcon(":undo_p.png"));
    mUndoAction->setText(tr("Undo"));
    
    mRedoAction = mUndoStack->createRedoAction(this);
    mRedoAction->setIcon(QIcon(":redo_p.png"));
    
    mUndoViewAction = mUndoDock->toggleViewAction();
    mUndoViewAction->setText(tr("Show Undo Stack"));
    
    //-----------------------------------------------------------------
    // MCMC Actions
    //-----------------------------------------------------------------
    mMCMCSettingsAction = new QAction(QIcon(":settings_p.png"), tr("MCMC"), this);
    
    mRunAction = new QAction(QIcon(":run_p.png"), tr("Run"), this);
    //runAction->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaPlay));
    mRunAction->setIconText(tr("Run"));
    mRunAction->setIconVisibleInMenu(true);
    mRunAction->setToolTip(tr("Run Model"));
    
    mResetMCMCAction = new QAction(tr("Reset MCMC Options"), this);
    
    //-----------------------------------------------------------------
    // View Actions
    //-----------------------------------------------------------------
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
    for(int i=0; i<plugins.size(); ++i){
        QString name = plugins[i]->getName();
        QList<QHash<QString, QVariant>> groupedActions = plugins[i]->getGroupedActions();
        for(int j=0; j<groupedActions.size(); ++j){
            QAction* act = new QAction(groupedActions[j]["title"].toString(), this);
            act->setData(QVariant(groupedActions[j]));
            connect(act, SIGNAL(triggered()), this, SLOT(doGroupedAction()));
            mDatesActions.append(act);
        }
    }

    //-----------------------------------------------------------------
    //  Grouped actions
    //-----------------------------------------------------------------
    mEventsColorAction = new QAction(tr("Selected events: change color"), this);
    connect(mEventsColorAction, SIGNAL(triggered()), this, SLOT(changeEventsColor()));
    
    mEventsMethodAction = new QAction(tr("Selected events: change method"), this);
    connect(mEventsMethodAction, SIGNAL(triggered()), this, SLOT(changeEventsMethod()));
    
    mDatesMethodAction = new QAction(tr("Selected events: change data method"), this);
    connect(mDatesMethodAction, SIGNAL(triggered()), this, SLOT(changeDatesMethod()));
    
    //-----------------------------------------------------------------
    // Help/About Menu
    //-----------------------------------------------------------------
    mAboutAct = new QAction(QIcon(":light.png"), tr("About"), this);
    connect(mAboutAct, SIGNAL(triggered()), this, SLOT(about()));
    
    mAboutQtAct = new QAction(QIcon(":qt.png"), tr("About Qt"), this);
    connect(mAboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    
    mHelpAction = new QAction(QIcon(":help_p.png"), tr("Help"), this);
    mHelpAction->setCheckable(true);
    connect(mHelpAction, SIGNAL(toggled(bool)), this, SLOT(showHelp(bool)));
    
    mManualAction = new QAction(QIcon(":pdf_p.png"), tr("Manual online"), this);
    connect(mManualAction, SIGNAL(triggered()), this, SLOT(openManual()));
    
    mWebsiteAction = new QAction(QIcon(":web_p.png"), tr("Website"), this);
    connect(mWebsiteAction, SIGNAL(triggered()), this, SLOT(openWebsite()));
}

void MainWindow::createMenus()
{
    //-----------------------------------------------------------------
    // Project menu
    //-----------------------------------------------------------------
    mProjectMenu = menuBar()->addMenu(tr("File"));

    mProjectMenu->addAction(mAppSettingsAction);
    mProjectMenu->addAction(mNewProjectAction);
    mProjectMenu->addAction(mOpenProjectAction);
    mProjectMenu->addAction(mCloseProjectAction);

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
    //mEditMenu->addAction(mUndoViewAction);
    
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
    // Help/About Menu
    //-----------------------------------------------------------------
    mHelpMenu = menuBar()->addMenu(tr("&Help"));
    mHelpMenu->addAction(mAboutAct);
    mHelpMenu->addAction(mAboutQtAct);
    
    //-----------------------------------------------------------------
    // Grouped Actions Menu
    //-----------------------------------------------------------------
    mPluginsMenu = menuBar()->addMenu(tr("Actions"));
    mPluginsMenu->addAction(mEventsColorAction);
    mPluginsMenu->addAction(mEventsMethodAction);
    mPluginsMenu->addAction(mDatesMethodAction);
    mPluginsMenu->addSeparator();
    
    for(int i=0; i<mDatesActions.size(); ++i){
        mPluginsMenu->addAction(mDatesActions[i]);
    }
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

#pragma mark Project Management

void MainWindow::newProject()
{
    // Ask to save the previous project.
    // Return true if the project doesn't need to be saved.
    // Returns true if the user saves the project or if the user doesn't want to save it.
    // Returns false if the user cancels.
    if(mProject->askToSave(tr("Save current project as...")))
    {
        // Ask to save the new project.
        // Returns true only if a new file is created.
        // Note : at this point, the project state is still the previous project state.
        if(mProject->saveAs(tr("Save new project as...")))
        {
            mUndoStack->clear();
            
            resetInterface();
            activateInterface(true);
            
            // Reset the project state and send a notification to update the views :
            mProject->initState(NEW_PROJECT_REASON);
        }
        
        mViewModelAction->trigger();
        //mProjectView->showModel();
        
        mViewResultsAction->setEnabled(false);
        updateWindowTitle();
    }
}

void MainWindow::openProject()
{
    QString path = QFileDialog::getOpenFileName(qApp->activeWindow(),
                                                tr("Open File"),
                                                getCurrentPath(),
                                                tr("Chronomodel Project (*.chr)"));
    
    if(!path.isEmpty())
    {
        if(mProject->askToSave(tr("Save current project as...")))
        {
            QFileInfo info(path);
            setCurrentPath(info.absolutePath());
            
            resetInterface();
            activateInterface(true);
            
            mUndoStack->clear();
            mProject->load(path);
            
            updateWindowTitle();
        }
    }
}

void MainWindow::closeProject()
{
    if(mProject->askToSave(tr("Save current project as...")))
    {
        mUndoStack->clear();
        
        mProject->initState(CLOSE_PROJECT_REASON);
        mProject->mLastSavedState = mProject->emptyState();
        mProject->mProjectFileName = QString();
        
        // Go back to model tab :
        mViewModelAction->trigger();
        mProject->clearModel();
        
        resetInterface();
        activateInterface(false);
        mViewResultsAction->setEnabled(false);
        
        updateWindowTitle();
    }
}

void MainWindow::saveProject()
{
    mProject->save(tr("Save current project as..."));
    updateWindowTitle();
}

void MainWindow::saveProjectAs()
{
    mProject->saveAs(tr("Save current project as..."));
    updateWindowTitle();
}

void MainWindow::updateWindowTitle()
{
    setWindowTitle(qApp->applicationName() + " " + qApp->applicationVersion() + (!(mProject->mProjectFileName.isEmpty()) ? QString(" - ") + mProject->mProjectFileName : ""));
}

void MainWindow::updateProject(){
    bool valid = mProject->isValid();
    mRunAction->setEnabled(valid);
    mProjectView->updateProject();
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
    dialog.setSettings(mAppSettings);
    connect(&dialog, SIGNAL(settingsChanged(const AppSettings&)), this, SLOT(setAppSettings(const AppSettings&)));
    if(dialog.exec() == QDialog::Accepted)
    {
        //setAppSettings(dialog.getSettings());
    }
}

void MainWindow::setAppSettings(const AppSettings& s)
{
    mAppSettings = s;
    QLocale::Language newLanguage = s.mLanguage;
    QLocale::Country newCountry= s.mCountry;
    
    QLocale newLoc = QLocale(newLanguage,newCountry);
    newLoc.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale::setDefault(newLoc);
    
    statusBar()->showMessage(tr("Language") + " : " + QLocale::languageToString(QLocale().language()));
    
    mProject->setAppSettings(mAppSettings);
    
    if(mViewResultsAction->isEnabled()) {
        mProjectView->updateResults(mProject->mModel);
    }
}

void MainWindow::openManual()
{
    QDesktopServices::openUrl(QUrl("http://www.chronomodel.fr/Chronomodel_User_Manual.pdf", QUrl::TolerantMode));
  

 /*   QString path = qApp->applicationDirPath();
#ifdef Q_OS_MAC
    QDir dir(path);
    dir.cdUp();
    path = dir.absolutePath() + "/Resources";
#endif
    path += "/Chronomodel_User_Manual.pdf";
    QDesktopServices::openUrl(QUrl("file:///" + path, QUrl::TolerantMode));
  */
}

void MainWindow::showHelp(bool show)
{
    mAppSettings.mShowHelp = show;
    mProjectView->showHelp(show);
}

void MainWindow::openWebsite()
{
    QDesktopServices::openUrl(QUrl("http://www.chronomodel.fr", QUrl::TolerantMode));
}

#pragma mark Grouped Actions
/*void MainWindow::datesActions()
{
    QString pluginName = sender()->objectName();
    
    const QList<PluginAbstract*>& plugins = PluginManager::getPlugins();
    for(int i=0; i<plugins.size(); ++i){
        if(plugins[i]->getName() == pluginName){
            PluginsSettingsDialog dialog(plugins[i], qApp->activeWindow());
            if(dialog.exec() == QDialog::Accepted)
            {
                // Is there anything to do ?
            }
        }
    }
}*/

void MainWindow::changeEventsColor()
{
    QColor color = QColorDialog::getColor(Qt::blue, qApp->activeWindow(), tr("Change selected events color"));
    if(color.isValid()){
        mProject->updateSelectedEventsColor(color);
    }
}
void MainWindow::changeEventsMethod()
{
    QStringList opts;
    opts.append(ModelUtilities::getEventMethodText(Event::eMHAdaptGauss));
    opts.append(ModelUtilities::getEventMethodText(Event::eBoxMuller));
    opts.append(ModelUtilities::getEventMethodText(Event::eDoubleExp));
    
    bool ok;
    QString methodStr = QInputDialog::getItem(qApp->activeWindow(),
                                          tr("Change events method"),
                                          tr("Change selected events MCMC method :"),
                                          opts, 0, false, &ok);
    if(ok && !methodStr.isEmpty()){
        Event::Method method = ModelUtilities::getEventMethodFromText(methodStr);
        mProject->updateSelectedEventsMethod(method);
    }
}
void MainWindow::changeDatesMethod()
{
    QStringList opts;
    const QList<PluginAbstract*>& plugins = PluginManager::getPlugins();
    for(int i=0; i<plugins.size(); ++i){
        //opts.append(plugins[i]->getId());
        opts.append(plugins[i]->getName());
    }
    bool ok;
    QString pluginName = QInputDialog::getItem(qApp->activeWindow(),
                                             tr("Change data method"),
                                             tr("For what type of data do you want to change the method ?"),
                                             opts, 0, false, &ok);
    if(ok){
        opts.clear();
        opts.append(ModelUtilities::getDataMethodText(Date::eMHSymetric));
        opts.append(ModelUtilities::getDataMethodText(Date::eInversion));
        opts.append(ModelUtilities::getDataMethodText(Date::eMHSymGaussAdapt));
        
        QString methodStr = QInputDialog::getItem(qApp->activeWindow(),
                                                  tr("Change data method"),
                                                  tr("Change MCMC method of data in selected events :"),
                                                  opts, 0, false, &ok);
        if(ok && !methodStr.isEmpty()){
            Date::DataMethod method = ModelUtilities::getDataMethodFromText(methodStr);
            PluginAbstract* plugin =PluginManager::getPluginFromName(pluginName);
            QString pluginId = plugin->getId();
            mProject->updateSelectedEventsDataMethod(method, pluginId);
        }
    }
}
void MainWindow::doGroupedAction()
{
    QAction* act = qobject_cast<QAction*>(sender());
    QVariant groupedActionVariant = act->data();
    QHash<QString, QVariant> groupedAction = groupedActionVariant.toHash();
    
    if(groupedAction["inputType"] == "combo"){
        bool ok;
        QString curve = QInputDialog::getItem(qApp->activeWindow(),
                                               groupedAction["title"].toString(),
                                               groupedAction["label"].toString(),
                                               groupedAction["items"].toStringList(),
                                               0, false, &ok);
        if(ok && !curve.isEmpty()){
            groupedAction["value"] = curve;
            mProject->updateAllDataInSelectedEvents(groupedAction);
        }
    }
}

#pragma mark Events
void MainWindow::closeEvent(QCloseEvent* e)
{
    QMessageBox message(QMessageBox::Question,
                        QApplication::applicationName(),
                        tr("Do you really want to quit ChronoModel ?"),
                        QMessageBox::Yes | QMessageBox::No,
                        qApp->activeWindow(),
                        Qt::Sheet);
    
    if(message.exec() == QMessageBox::Yes)
    {
        if(mProject->askToSave(tr("Save project before quitting?")))
        {
            writeSettings();
            e->accept();
        }
        else
        {
            e->ignore();
        }
    }
    else
    {
        e->ignore();
    }
}

void MainWindow::keyPressEvent(QKeyEvent* keyEvent)
{
    if(keyEvent->matches(QKeySequence::Undo))
    {
        mUndoStack->undo();
    }
    else if(keyEvent->matches(QKeySequence::Redo))
    {
        mUndoStack->redo();
    }
    QMainWindow::keyPressEvent(keyEvent);
}

#pragma mark Settings
void MainWindow::writeSettings()
{
    mProjectView->writeSettings();
    
    QSettings settings;
    
    settings.beginGroup("MainWindow");
    
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    
    settings.setValue("last_project_dir", mProject->mProjectFileDir);
    settings.setValue("last_project_filename", mProject->mProjectFileName);
    
    settings.beginGroup("AppSettings");
    settings.setValue(APP_SETTINGS_STR_AUTO_SAVE, mAppSettings.mAutoSave);
    settings.setValue(APP_SETTINGS_STR_AUTO_SAVE_DELAY_SEC, mAppSettings.mAutoSaveDelay);
    settings.setValue(APP_SETTINGS_STR_SHOW_HELP, mAppSettings.mShowHelp);
    settings.setValue(APP_SETTINGS_STR_CELL_SEP, mAppSettings.mCSVCellSeparator);
    settings.setValue(APP_SETTINGS_STR_DEC_SEP, mAppSettings.mCSVDecSeparator);
    settings.setValue(APP_SETTINGS_STR_OPEN_PROJ, mAppSettings.mOpenLastProjectAtLaunch);
    settings.setValue(APP_SETTINGS_STR_PIXELRATIO, mAppSettings.mPixelRatio);
    settings.setValue(APP_SETTINGS_STR_DPM, mAppSettings.mDpm);
    settings.setValue(APP_SETTINGS_STR_IMAGE_QUALITY, mAppSettings.mImageQuality);
    settings.setValue(APP_SETTINGS_STR_FORMATDATE, mAppSettings.mFormatDate);
    settings.setValue(APP_SETTINGS_STR_PRECISION, mAppSettings.mPrecision);
    settings.endGroup();
    
    settings.endGroup();
}

void MainWindow::readSettings(const QString& defaultFilePath)
{
    QSettings settings;
    settings.beginGroup("MainWindow");
    
    resize(settings.value("size", QSize(400, 400)).toSize());
    move(settings.value("pos", QPoint(200, 200)).toPoint());
    
    settings.beginGroup("AppSettings");
    mAppSettings.mLanguage = (QLocale::Language) settings.value(APP_SETTINGS_STR_LANGUAGE, QLocale::system().language()).toInt();
    mAppSettings.mCountry = (QLocale::Country) settings.value(APP_SETTINGS_STR_COUNTRY, QLocale::system().language()).toInt();
    mAppSettings.mAutoSave = settings.value(APP_SETTINGS_STR_AUTO_SAVE, APP_SETTINGS_DEFAULT_AUTO_SAVE).toBool();
    mAppSettings.mAutoSaveDelay = settings.value(APP_SETTINGS_STR_AUTO_SAVE_DELAY_SEC, APP_SETTINGS_DEFAULT_AUTO_SAVE_DELAY_SEC).toInt();
    mAppSettings.mShowHelp = settings.value(APP_SETTINGS_STR_SHOW_HELP, APP_SETTINGS_DEFAULT_SHOW_HELP).toBool();
    mAppSettings.mCSVCellSeparator = settings.value(APP_SETTINGS_STR_CELL_SEP, APP_SETTINGS_DEFAULT_CELL_SEP).toString();
    mAppSettings.mCSVDecSeparator = settings.value(APP_SETTINGS_STR_DEC_SEP, APP_SETTINGS_DEFAULT_DEC_SEP).toString();
    mAppSettings.mOpenLastProjectAtLaunch = settings.value(APP_SETTINGS_STR_OPEN_PROJ, APP_SETTINGS_DEFAULT_OPEN_PROJ).toBool();
    mAppSettings.mPixelRatio = settings.value(APP_SETTINGS_STR_PIXELRATIO, APP_SETTINGS_DEFAULT_PIXELRATIO).toInt();
    mAppSettings.mDpm = settings.value(APP_SETTINGS_STR_DPM, APP_SETTINGS_DEFAULT_DPM).toInt();
    mAppSettings.mImageQuality = settings.value(APP_SETTINGS_STR_IMAGE_QUALITY, APP_SETTINGS_DEFAULT_IMAGE_QUALITY).toInt();
    mAppSettings.mFormatDate = (DateUtils::FormatDate)settings.value(APP_SETTINGS_STR_FORMATDATE, APP_SETTINGS_DEFAULT_FORMATDATE).toInt();
    mAppSettings.mPrecision = settings.value(APP_SETTINGS_STR_FORMATDATE, APP_SETTINGS_DEFAULT_FORMATDATE).toInt();
    settings.endGroup();
    

    mProjectView->showHelp(mAppSettings.mShowHelp);
    mHelpAction->setChecked(mAppSettings.mShowHelp);
    
    bool fileOpened = false;
    if(!defaultFilePath.isEmpty())
    {
        QFileInfo fileInfo(defaultFilePath);
        if(fileInfo.isFile())
        {
            if(mProject->load(defaultFilePath)){
                activateInterface(true);
                updateWindowTitle();
                fileOpened = true;
            }
        }
    }
    
    if(!fileOpened && mAppSettings.mOpenLastProjectAtLaunch)
    {
        QString dir = settings.value("last_project_dir", "").toString();
        QString filename = settings.value("last_project_filename", "").toString();
        //QString filename ="";
        QString path = dir + "/" + filename;
        QFileInfo fileInfo(path);
        
        if(fileInfo.isFile())
        {
            mProject->load(path);
            activateInterface(true);
            updateWindowTitle();
        }
    }
    
    settings.endGroup();
    
    setAppSettings(mAppSettings);
    mProjectView->readSettings();
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
    mProjectExportAction->setEnabled(activate);
    
    mViewModelAction->setEnabled(activate);
    mMCMCSettingsAction->setEnabled(activate);
    mResetMCMCAction->setEnabled(activate);
    
    // Les actions suivantes doivent être désactivées si on ferme le projet.
    // Par contre, elles ne doivent pas être ré-activée dès l'ouverture d'un projet
    if(!activate)
    {
        mRunAction->setEnabled(activate);
        mViewResultsAction->setEnabled(activate);
        mViewLogAction->setEnabled(activate);
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

void MainWindow::mcmcFinished(Model* model)
{
    mViewLogAction -> setEnabled(true);
    mViewResultsAction -> setEnabled(true);
    mViewResultsAction -> setChecked(true); // Just check the Result Button after computation and mResultsView is show after
    //mViewResultsAction->trigger();
    
    mProjectView->updateResults(model);
}
