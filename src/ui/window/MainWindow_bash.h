/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#ifndef MAINWINDOW_BASH_H
#define MAINWINDOW_BASH_H

#include "Project.h"
#include "Singleton.h"

#include <QMainWindow>

class QMenu;
class QAction;
class QActionGroup;
class QStackedWidget;
class QUndoStack;
class QUndoView;
class QDockWidget;
class ProjectView;
class Project;
class Event;
class Model;

class MainWindow : public QMainWindow, public Singleton<MainWindow>
{
    Q_OBJECT
    friend class Singleton<MainWindow>;

public:
    MainWindow(QWidget* aParent = nullptr);
    ~MainWindow();

    //Project* getProject();
    const std::shared_ptr<Project>& getProject();
    QJsonObject getState() const;

    QUndoStack* getUndoStack();
    QString getCurrentPath() const;
    void setCurrentPath(const QString& path);

    void resetInterface();
    void activateInterface(bool activate);

    QString getNameProject() const;

    void readSettings(const QString&);
    void writeSettings();
    void updateWindowTitle();
    void setFont(const QFont& font);

protected:
    void closeEvent(QCloseEvent* e) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent* e) Q_DECL_OVERRIDE;
    void changeEvent(QEvent* event) Q_DECL_OVERRIDE;

    void connectProject();
    void disconnectProject();

private:
    void createActions();
    void createMenus();
    void createToolBars();
    void runModel();
    void calibrateAll();

public slots:
 //   void newProject();
    void openProject();
//    void insertProject();
    void closeProject();
    void saveProject();
//    void saveProjectAs();
    void about();
    void appSettings();
    void setAppSettings();
    void setAppFilesSettings();
    void openManual();
    void openWebsite();
    void showHelp(bool);
    void setLanguage(QAction* action);

    //void mcmcFinished(Model* model) { mProject->mModel = model; };
    void mcmcFinished() {};//
    void noResult() {};
 //   void updateProject();
    void toggleCurve(bool);
/*
    void changeEventsColor();
    void changeEventsMethod();
    void changeDatesMethod();
    void selectAllEvents();
    void selectEventInSelectedPhases();
    void selectEventWithString();
    void doGroupedAction();
*/
    void setRunEnabled(bool enabled);
    void setLogEnabled(bool enabled);
    void setResultsEnabled(bool enabled);

private:
    QToolBar* mToolBar;

    QStackedWidget* mCentralStack;
    ProjectView* mProjectView;
    //Project* mProject;
    std::shared_ptr<Project> mProject;
    QUndoStack* mUndoStack;
    QUndoView* mUndoView;
    QDockWidget* mUndoDock;

    QString mLastPath;

    QMenu* mProjectMenu;
    QMenu* mEditMenu;

    QAction* mAppSettingsAction;
    QAction* mAboutAct;
    QAction* mAboutQtAct;

    QActionGroup* mLangGroup;
    QAction* mTranslateEnglishAct;
    QAction* mTranslateFrenchAct;

    QAction* mOpenProjectAction;

    QAction* mProjectSaveAction;

    QAction* mRunAction;
    QAction* mResetMCMCAction;
    

    QActionGroup* mViewGroup;
    //QAction* mViewModelAction;
    QAction* mViewResultsAction;
    QAction* mViewLogAction;

    /*QAction* mHelpAction;
    QAction* mManualAction;
    QAction* mWebsiteAction;
   */

private:
    Q_DISABLE_COPY(MainWindow)
};

#endif
