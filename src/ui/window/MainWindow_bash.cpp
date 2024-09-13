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

#include "MainWindow_bash.h"

#include "ProjectView_bash.h"

#include "ModelCurve.h"
#include "Project.h"

#include "AboutDialog.h"
#include "AppSettingsDialog.h"
#include "AppSettings.h"
#include "QtUtilities.h"
#include "StateKeys.h"

#include <QtWidgets>
#include <QLocale>
#include <QFont>


// Constructor / Destructor
MainWindow::MainWindow(QWidget* aParent):
        QMainWindow(aParent)
{
#ifdef DEBUG
    setWindowTitle(qApp->applicationName() + " " + qApp->applicationVersion() + " DEBUG Mode ");
#else
    setWindowTitle(qApp->applicationName() + " " + qApp->applicationVersion() );
#endif

    QPalette tooltipPalette;
    tooltipPalette.setColor(QPalette::ToolTipBase, Qt::white);
    tooltipPalette.setColor(QPalette::ToolTipText, Qt::black);
    QToolTip::setPalette(tooltipPalette);
    QFont tooltipFont(font());
    tooltipFont.setItalic(true);


    QToolTip::setFont(tooltipFont);

    mLastPath = QDir::homePath();

    mProject = nullptr;

    /* Creation of ResultsView and ModelView */
    mProjectView = new ProjectView(mProject);
    setCentralWidget(mProjectView);

    mUndoStack = new QUndoStack();
    mUndoStack->setUndoLimit(1000);

    mUndoView = new QUndoView(mUndoStack);
    mUndoView->setEmptyLabel(tr("Initial state"));
    mUndoDock = new QDockWidget(this);
    //mUndoDock->setFixedWidth(250);
    mUndoDock->setWidget(mUndoView);
    mUndoDock->setAllowedAreas(Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, mUndoDock);
    mUndoDock->setVisible(false);

    createActions();
    createMenus();
    createToolBars();

    statusBar()->showMessage(tr("Ready"));
    //setUnifiedTitleAndToolBarOnMac(true);
    setWindowIcon(QIcon(":chronomodel_bash.png"));

    resize(AppSettings::mLastSize);

    connect(mProjectSaveAction, static_cast<void (QAction::*)(bool)> (&QAction::triggered), this, &MainWindow::saveProject);
    // connect(mProjectSaveAsAction, static_cast<void (QAction::*)(bool)> (&QAction::triggered), this, &MainWindow::saveProjectAs);

    /*connect(mViewModelAction, static_cast<void (QAction::*)(bool)> (&QAction::triggered), mProjectView, &ProjectView::showModel );
    connect(mViewLogAction, static_cast<void (QAction::*)(bool)> (&QAction::triggered), mProjectView, &ProjectView::showLog);
    connect(mViewResultsAction, static_cast<void (QAction::*)(bool)> (&QAction::triggered), mProjectView, &ProjectView::showResults);
*/
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

    activateInterface(true);
}

MainWindow::~MainWindow()
{

}

// Accessors

const std::shared_ptr<Project> &MainWindow::getProject()
{
    return mProject;
}

QJsonObject MainWindow::getState() const
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

    mAppSettingsAction = new QAction(QIcon(":settings.png"), tr("Settings"), this);
    connect(mAppSettingsAction, &QAction::triggered, this, &MainWindow::appSettings);

    //-----------------------------------------------------------------
    // Project Actions
    //-----------------------------------------------------------------

    mOpenProjectAction = new QAction(QIcon(":open_p.png"), tr("Open"), this);
    mOpenProjectAction->setShortcuts(QKeySequence::Open);
    mOpenProjectAction->setStatusTip(tr("Open an existing project"));
    connect(mOpenProjectAction, &QAction::triggered, this, &MainWindow::openProject);

    mProjectSaveAction = new QAction(QIcon(":save_p.png"), tr("&Save"), this);
    mProjectSaveAction->setShortcuts(QKeySequence::Save);
    mProjectSaveAction->setStatusTip(tr("Save the current project in the same place with the same name"));


    //-----------------------------------------------------------------
    // MCMC Actions
    //-----------------------------------------------------------------

    mRunAction = new QAction(QIcon(":run_p.png"), tr("Run"), this);
    //runAction->setIcon(qApp->style()->standardIcon(QStyle::SP_MediaPlay));
    mRunAction->setIconText(tr("Run"));
    mRunAction->setIconVisibleInMenu(true);
    mRunAction->setToolTip(tr("Run Model"));

    //-----------------------------------------------------------------
    // Help/About Menu
    //-----------------------------------------------------------------
    /*mAboutAct = new QAction(QIcon(":light.png"), tr("About"), this);
    connect(mAboutAct, &QAction::triggered, this, &MainWindow::about);*/

    mAboutQtAct = new QAction(QIcon(":qt.png"), tr("About Qt"), this);
    connect(mAboutQtAct, &QAction::triggered, qApp, QApplication::aboutQt);

  /*  mHelpAction = new QAction(QIcon(":help_p.png"), tr("Help"), this);
    mHelpAction->setCheckable(true);
    connect(mHelpAction, &QAction::triggered, this, &MainWindow::showHelp);

    mManualAction = new QAction(QIcon(":pdf_p.png"), tr("Manual Online"), this);
    connect(mManualAction, &QAction::triggered, this, &MainWindow::openManual);

    mWebsiteAction = new QAction(QIcon(":web_p.png"), tr("Website"), this);
    connect(mWebsiteAction, &QAction::triggered, this, &MainWindow::openWebsite);
*/

}

void MainWindow::createMenus()
{
    //-----------------------------------------------------------------
    // Project menu
    //-----------------------------------------------------------------
    const  QFont ft (font());
    mProjectMenu = menuBar()->addMenu(tr("File"));
    mProjectMenu->addAction(mAppSettingsAction);
    //mProjectMenu->addAction(mNewProjectAction);
    mProjectMenu->addAction(mOpenProjectAction);
   // mProjectMenu->addAction(mInsertProjectAction);
//    mProjectMenu->addAction(mCloseProjectAction);

    mProjectMenu->addSeparator();

  //  mProjectMenu->addAction(mProjectSaveAction);
  //  mProjectMenu->addAction(mProjectSaveAsAction);

    mProjectMenu->addSeparator();

 //   mProjectMenu->addAction(mProjectExportAction);
    mProjectMenu->setFont(ft);

    //-----------------------------------------------------------------
    // Edit menu
    //-----------------------------------------------------------------
    mEditMenu = menuBar()->addMenu(tr("Edit"));

  //  mEditMenu->addAction(mUndoAction);
  //  mEditMenu->addAction(mRedoAction);
//    mEditMenu->addAction(mSelectAllAction);
    mEditMenu->setFont(ft);

    //-----------------------------------------------------------------
    // MCMC menu
    //-----------------------------------------------------------------
/*    mMCMCMenu = menuBar()->addMenu(tr("MCMC"));
    mMCMCMenu->addAction(mRunAction);
    mMCMCMenu->addAction(mMCMCSettingsAction);
    mMCMCMenu->addSeparator();
    mMCMCMenu->addAction(mResetMCMCAction);
*/
    //-----------------------------------------------------------------
    // View menu
    //-----------------------------------------------------------------
 /*   mViewMenu = menuBar()->addMenu(tr("View"));
    mViewMenu->addAction(mViewModelAction);
    mViewMenu->addAction(mViewResultsAction);
    mViewMenu->addAction(mViewLogAction);
*/
    //-----------------------------------------------------------------
    // Help/About Menu this menu depend of the system. On MacOs it's in Chronomodel menu
    //-----------------------------------------------------------------
/*    mHelpMenu = menuBar()->addMenu(tr("About"));
    mHelpMenu->menuAction()->setShortcut(Qt::Key_Question);
    mHelpMenu->addAction(mAboutAct);
    mHelpMenu->addAction(mAboutQtAct);
*/
    //-----------------------------------------------------------------
    // Grouped Actions Menu
    //-----------------------------------------------------------------
/*    mActionsMenu = menuBar()->addMenu(tr("Actions"));
    mActionsMenu->addAction((mSelectEventsNameAction));
    mActionsMenu->addAction(mSelectEventsAction);
    mActionsMenu->addSeparator();
    mActionsMenu->addAction(mEventsColorAction);
    mActionsMenu->addAction(mEventsMethodAction);
    mActionsMenu->addAction(mDatesMethodAction);
    mActionsMenu->addSeparator();

    for (int i=0; i<mDatesActions.size(); ++i)
        mActionsMenu->addAction(mDatesActions[i]);
*/
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

  //  mToolBar->addAction(mNewProjectAction);
    mToolBar->addAction(mOpenProjectAction);
    mToolBar->addAction(mProjectSaveAction);
 //   mToolBar->addAction(mProjectExportAction);

 //   mToolBar->addAction(mUndoAction);
 //   mToolBar->addAction(mRedoAction);

    QWidget* separator3 = new QWidget(this);
    separator3->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mToolBar->addWidget(separator3);

 /*   mToolBar->addAction(mCurveAction);
    mToolBar->addAction(mViewModelAction);
    mToolBar->addAction(mMCMCSettingsAction);
 */
    mToolBar->addAction(mRunAction);
   /* mToolBar->addAction(mViewResultsAction);
    mToolBar->addAction(mViewLogAction);
*/
    QWidget* separator4 = new QWidget(this);
    separator4->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mToolBar->addWidget(separator4);

   // mToolBar->addAction(mHelpAction);
   // mToolBar->addAction(mManualAction);
   // mToolBar->addAction(mWebsiteAction);
    /* toolBar->addAction(mAboutAct);*/
    mToolBar->addAction(mAboutQtAct);
    mToolBar->setFont(qApp->font()); // must be after all addAction

}


// -----------
/*
void MainWindow::newProject()
{
    // Ask to save the previous project.
    // Return true if the project doesn't need to be saved.
    // Returns true if the user saves the project or if the user doesn't want to save it.
    // Returns false if the user cancels.
    bool yesCreate= false;

    if ((mProject == nullptr) || (mProject->askToSave(tr("Save current project as...") )))
        yesCreate= true;

    if (yesCreate) {
        Project* newProject = new Project();
         // just update mAutoSaveTimer to avoid open the save() dialog box
        newProject-> mAutoSaveTimer->stop();

        if (newProject->saveAs(tr("Save new project as..."))) {
            //mUndoStack->clear();

            // resetInterface Disconnect also the scene
          //  resetInterface();

            //activateInterface(true);


            newProject->initState(NEW_PROJECT_REASON);// emit showStudyPeriodWarning();

            delete mProject;

            mProject = newProject;

            activateInterface(true); // mProject doit exister

            // Create project connections
            connectProject();

            // Apply app settings to the project
            mProject->setAppSettings();

            // Send the project to the views
         //   mProjectView->setProject(mProject);

            // Ask for the a Study Period (open dialog)
          //  mProjectView->newPeriod();

            // Open the Model View
          //  mViewModelAction->trigger();

            // Disable the Result View
            mViewResultsAction->setEnabled(false);

            updateWindowTitle();

            mUndoStack->clear();

        } else {
            delete newProject;
        }
    }
}
*/

void MainWindow::openProject()
{
    const QString currentPath = getCurrentPath();
    QString path = QFileDialog::getOpenFileName(this,
                                                      tr("Insert File"),
                                                      currentPath,
                                                      tr("Chronomodel Bash Project (*.chb)"));

    QFileInfo checkFile (path);
    if (!checkFile.exists() || !checkFile.isFile()) {
        QMessageBox message(QMessageBox::Critical,
                            tr("Error loading project file"),
                            tr("The project file could not be loaded.") + "\r" +
                            path  +
                            tr("Could not be find"),
                            QMessageBox::Ok,
                            qApp->activeWindow());
        message.exec();

    }

    qDebug() << "in Project::load Loading Bash Project file : " << path;
    QFile bashFile (path);
    if (bashFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QFileInfo info(path);
        MainWindow::getInstance()->setCurrentPath(info.absolutePath());

        AppSettings::mLastDir = info.absolutePath();
        AppSettings::mLastFile = info.fileName();

        QByteArray saveData = bashFile.readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc (QJsonDocument::fromJson(saveData, &error));

        if (error.error !=  QJsonParseError::NoError) {
            QMessageBox message(QMessageBox::Critical,
                                tr("Error loading Bash project file"),
                                tr("The Bash project file could not be loaded.") + "\r" +
                                tr("Error message") + " : " + error.errorString(),
                                QMessageBox::Ok,
                                qApp->activeWindow());
            message.exec();

        } else {

            mProjectView->mTable->clear();
            mProjectView->mTable->setRowCount(0);
            mProjectView->mTable->setHorizontalHeaderLabels({"Project Files"});
            mProjectView->mTable->setTextElideMode(Qt::ElideNone);

            QJsonObject loadingState = jsonDoc.object();
            QJsonArray list = loadingState.value("project_list").toArray();

            for (int i = 0; i <list.count(); ++i) {
                QString iPath = list.at(i).toString();
                mProjectView->mTable->setRowCount(mProjectView->mTable->rowCount()+1);

                QTableWidgetItem *newItem = new QTableWidgetItem(iPath);

                mProjectView->mTable->setItem(mProjectView->mTable->rowCount()-1 , 0, newItem );
                mProjectView->mTable->item(mProjectView->mTable->rowCount()-1 , 0)->setToolTip(iPath);
            }

            statusBar()->showMessage(tr("Loading the project list Bash file : %1").arg(path));
            //activateInterface(true); // do several connection
        }
    }

}



void MainWindow::connectProject()
{
    connect(mProject.get(), &Project::mcmcFinished, this, &MainWindow::mcmcFinished);

    connect(mRunAction, &QAction::triggered, mProject.get(), &Project::run);
}

void MainWindow::disconnectProject()
{
    disconnect(mProject.get(), &Project::mcmcFinished, this, &MainWindow::mcmcFinished);
    disconnect(mRunAction, &QAction::triggered, mProject.get(), &Project::run);
}

void MainWindow::closeProject()
{
   if (mProject) {
        if ( mProject->askToSave(tr("Save current project as...")) == true)
             mProject->saveProjectToFile();

        mUndoStack->clear();

        mProject->initState(CLOSE_PROJECT_REASON);
        mProject->mLastSavedState = mProject->mState;//emptyState();
        AppSettings::mLastDir = QString();

        // Go back to model tab :
        //mViewModelAction->trigger();
        mProject->clear_and_shrink_model();
        disconnectProject();

        resetInterface();

        activateInterface(false);
        mViewResultsAction->setEnabled(false);

        updateWindowTitle();

        mProject.reset();

   } else // if there is no project, we suppose it means to close the programm
       QApplication::exit(0);
}

void MainWindow::saveProject()
{
    QString path = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                           "Save current bash project as...",
                                           MainWindow::getInstance()->getCurrentPath(),
                                           tr("Chronomodel Bash Project (*.chb)"));


    if (!path.isEmpty()) {
        QFileInfo info (path);
        MainWindow::getInstance()->setCurrentPath(info.absolutePath());

        QFile file_chb (path);

        QJsonObject mBash;

        mBash[STATE_APP_VERSION] = QApplication::applicationVersion();

        QJsonArray mList ;
      //  QJsonArray seeds;
        for (int i = 0; i<mProjectView->mTable->rowCount(); ++i) {
            auto itemText = mProjectView->mTable->item(i, 0)->text();
            mList.append (QJsonValue::fromVariant(itemText));
        }
        mBash["project_list"] = mList;

        QJsonDocument jsonDoc (mBash);
        QByteArray textDoc = jsonDoc.toJson (QJsonDocument::Indented);

        if (file_chb.open(QIODevice::ReadWrite | QIODevice::Text)) {
#ifdef DEBUG
            qDebug() << "[Mainwindows_bash::saveProject] Project Bash saved to : " << path;
#endif
            file_chb.write (textDoc);

            file_chb.resize (file_chb.pos());
            file_chb.close ();
        }


    }

}

void MainWindow::updateWindowTitle()
{
#ifdef DEBUG
    setWindowTitle(qApp->applicationName() + " " + qApp->applicationVersion() + " DEBUG Mode "+ (AppSettings::mLastFile.isEmpty() ?  "" : QString(" - ") + AppSettings::mLastFile));
#else
    setWindowTitle(qApp->applicationName() + " " + qApp->applicationVersion() + (AppSettings::mLastFile.isEmpty() ?  "" : QString(" - ") + AppSettings::mLastFile));
#endif
}



void MainWindow::toggleCurve(bool )
{

}

// Settings & About
void MainWindow::about()
{
    AboutDialog dialog(qApp->activeWindow());
    dialog.exec();
}

void MainWindow::appSettings()
{
    AppSettingsDialog dialog(qApp->activeWindow());

    dialog.setSettings();
    connect(&dialog, &AppSettingsDialog::settingsChanged, this, &MainWindow::setAppSettings);
    connect(&dialog, &AppSettingsDialog::settingsFilesChanged, this, &MainWindow::setAppFilesSettings);
    dialog.exec();

}

void MainWindow::setAppFilesSettings()
{
    QLocale::Language newLanguage = AppSettings::mLanguage;
    QLocale::Country newCountry= AppSettings::mCountry;

    QLocale newLoc = QLocale(newLanguage,newCountry);
    newLoc.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale::setDefault(newLoc);
    //statusBar()->showMessage(tr("Language") + " : " + QLocale::languageToString(QLocale().language()));

    QFont tooltipFont(font());
    tooltipFont.setItalic(true);

    QToolTip::setFont(tooltipFont);

    writeSettings();
}

void MainWindow::setAppSettings()
{
    QLocale::Language newLanguage = AppSettings::mLanguage;
    QLocale::Country newCountry= AppSettings::mCountry;

    QLocale newLoc = QLocale(newLanguage, newCountry);
    newLoc.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale::setDefault(newLoc);

    setFont(qApp->font());

    QFont tooltipFont(font());
    tooltipFont.setItalic(true);

    QToolTip::setFont(tooltipFont);

    writeSettings();
}

void MainWindow::openManual()
{
    QDesktopServices::openUrl(QUrl("https://chronomodel.com/storage/medias/59_manuel_release_2_0_version_1_04_03_2019.pdf", QUrl::TolerantMode));

}

void MainWindow::showHelp(bool show)
{
   /* if (show)
        QWhatsThis::enterWhatsThisMode();
    else
        QWhatsThis::leaveWhatsThisMode();*/
    AppSettings::mShowHelp = show;
   // mProjectView->showHelp(show);
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

// Language
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

// Grouped Actions


// Events
/**
 * @todo Fix app close event called twice when updating with Qt >= 5.6
 */
void MainWindow::closeEvent(QCloseEvent* e)
{
    (void) e;
    QGuiApplication::exit(0);
}

void MainWindow::keyPressEvent(QKeyEvent* keyEvent)
{
    if (keyEvent->matches(QKeySequence::Undo))
        mUndoStack->undo();

    else if (keyEvent->matches(QKeySequence::Redo))
        mUndoStack->redo();

    QMainWindow::keyPressEvent(keyEvent);
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        // TODO : set every text that needs translation !!!
        qDebug() << "--> MainWindow language updated";
       // mNewProjectAction->setText(tr("&New"));
    } else
        QMainWindow::changeEvent(event);

}

// Settings
void MainWindow::writeSettings()
{
   // mProjectView->writeSettings();
    AppSettings::mLastSize = size();
    AppSettings::mLastPosition = pos();

    AppSettings::writeSettings();

}

void MainWindow::readSettings(const QString& )
{
    move( AppSettings::mLastPosition);
    if (AppSettings::mLastSize.width() >50)
        resize( AppSettings::mLastSize);
    else
        resize( QSize(400, 400));

}

void MainWindow::resetInterface()
{
    mProjectView->setVisible(false);
    disconnect(mRunAction, &QAction::triggered, this, &MainWindow::runModel);
}

void MainWindow::activateInterface(bool activate)
{
    mProjectView->setVisible(activate);
     if (activate)
        connect(mRunAction, &QAction::triggered, this, &MainWindow::runModel);
    else
        disconnect(mRunAction, &QAction::triggered, this, &MainWindow::runModel);

}

void MainWindow::calibrateAll()
{
    QJsonObject state = mProject->state();

    StudyPeriodSettings s = StudyPeriodSettings::fromJson(state.value(STATE_SETTINGS).toObject());
    QJsonArray Qevents = state.value(STATE_EVENTS).toArray();

    /* If the Events Scene isEmpty (i.e. when the project is created)
    * There is no date to calibrate
    */
    if (!Qevents.isEmpty()) {
        mProject->mCalibCurves.clear();
        QList<Event> events;
        for (auto&& Qev: Qevents)
            events.append(Event::fromJson(Qev.toObject()));

        //QProgressDialog *progress = new QProgressDialog("Calibration curve generation -----2","Wait" , 1, 10, qApp->activeWindow(), Qt::Widget);
        QProgressDialog *progress = new QProgressDialog("Calibration in progress...","Wait" , 1, 10);
        progress->setWindowModality(Qt::WindowModal);
        progress->setCancelButton(nullptr);
        progress->setMinimumDuration(4);
        progress->setMinimum(0);
        //progress->setMinimumWidth(7 * AppSettings::widthUnit());
        progress->setMinimumWidth(int (progress->fontMetrics().boundingRect(progress->labelText()).width() * 1.5));

        int position(0);
        for (auto& ev : events)
            position += ev.mDates.size();

        progress->setMaximum(position);

        position = 0;
        // rebuild all calibration
        for (auto&& ev : events) {
                 const QJsonArray listDates = ev.toJson().value(STATE_EVENT_DATES).toArray();

                for (auto&& date : listDates) {
                    Date d (date.toObject());
                    d.autoSetTiSampler(true);

                    d.calibrate(s, mProject, true);

                    ++position;
                    progress->setValue(position);
                }
          }

    }


}


void MainWindow::runModel()
{
    QElapsedTimer startTime;
    startTime.start();

    mProjectView->mLog->append(QDateTime::currentDateTime().toString("dddd dd MMMM yyyy"));
    mProjectView->mLog->append(textBold( textRed("Start Bash Project at ")) + QTime::currentTime().toString("hh:mm:ss.zzz") );

    for (auto i = 0 ; i < mProjectView->mTable->rowCount(); i++) {
        const QString path = mProjectView->mTable->item(i, 0)->text();
        QFileInfo chrFile (path);

        AppSettings::mLastDir = chrFile.path(); // usefull for project.save()
        AppSettings::mLastFile = chrFile.baseName();

        qDebug()<<"Run file"<< AppSettings::mLastDir + "/" + AppSettings::mLastFile;
        mProjectView->mLog->append("Run File : " + textBold(chrFile.baseName()));
        if (!mProject)
            mProject.reset(new Project());

        else if (mProject->mModel) {
            mProject->mModel->clear_and_shrink();

        }
        try {
            mProject->load(path, true);
            AppSettings::mAutoSave = true; // usefull to disable savebox in run()
            AppSettings::mIsSaved = true;
            mProjectView->mLog->append("    Calibration");
            mProjectView->repaint();
            calibrateAll();
            mProjectView->mLog->append("    Start Run");
            startTime.start();
            mProject->run();
            if (mProject->mModel->mEvents.at(0)->mTheta.mRawTrace->empty())
                throw QObject::tr("Error in RUN");

            mProjectView->mLog->append("    End Run, time elapsed "+ DHMS(startTime.elapsed()));
            mProject->setNoResults(false);
            mProjectView->repaint();
            mProjectView->mLog->append("    Start Saving");
            AppSettings::mAutoSave = true; // usefull to disable savebox in run()
            AppSettings::mIsSaved = false;
            mProject->save();

            mProjectView->mLog->append("    Saved File : " + textBold( AppSettings::mLastDir + "/" + AppSettings::mLastFile + ".res"));

            mProjectView->mLog->append(textGreen("    End Saving") );
            mProjectView->mLog->append(textGreen("") );

        } catch (...) {
            mProjectView->mLog->append(textRed("Error with file : "+ path));

        }

    }

    AppSettings::mAutoSave = false; // usefull to stop autosaving
    mProjectView->mLog->append(QDateTime::currentDateTime().toString("dddd dd MMMM yyyy"));
    mProjectView->mLog->append(textBold("End Bash Project at ") + QTime::currentTime().toString("hh:mm:ss.zzz") );
}

void MainWindow::setRunEnabled(bool enabled)
{
    mRunAction->setEnabled(enabled);
}

void MainWindow::setLogEnabled(bool enabled)
{
 (void) enabled;
}

void MainWindow::setResultsEnabled(bool enabled)
{
 (void) enabled;
}

