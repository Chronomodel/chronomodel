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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Project.h"
#include "Singleton.h"
#include "SwitchAction.h"

#include <QMainWindow>
#include <QUndoStack>
#include <QUndoView>
#include <QDockWidget>
#include <QStatusBar>
#include <QActionGroup>
#include <QPushButton>
#include <QStackedWidget>
#include <QWidgetAction>


class ProjectView;

class Event;
class Model;

class MainWindow : public QMainWindow, public Singleton<MainWindow>
{
    Q_OBJECT
    friend class Singleton<MainWindow>;

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    const std::shared_ptr<Project>& getProject();
    QJsonObject &getState() const;

    inline void updateEvent(const QJsonObject &event, const Project::ReasonId id) {mProject->updateEvent(event, id);};
    QUndoStack* getUndoStack();

    QString getCurrentPath() const;
    void setCurrentPath(const QString& path);

    void clearInterface();
    void resetInterface();
    void activateInterface(bool activate);
    void setRunEnabled(bool enabled);
    void setLogEnabled(bool enabled);
    void setResultsEnabled(bool enabled);
    QString getNameProject() const;

    void readSettings(const QString& defaultFilePath);
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

    QIcon iconWithStatusBadge(const QIcon& baseIcon, const QColor& badgeColor, int size = -1);

public slots:
    void newProject();
    void openProject();
    void insertProject();
    void closeProject();
    void saveProject();
    void saveProjectAs();
    void about();
    void appSettings();
    void setAppSettings();
    void updateAppSettings();

    void setAppFilesSettings();
    void openManual();
    void openWebsite();
    void showHelp(bool);
    void setLanguage(QAction* action);
    void mcmcFinished();
    void setConvergenceStatus( MCMCDiagnostic::ConvergenceStatus status);

    void noResult();
    void updateProject();
    void toggleCurve(bool checked);

    void rebuildExportCurve();
    void changeEventsColor();
#ifndef FIXEDPRIOR
    void changeEventsMethod();
    void changeDatesMethod();
#endif
    void selectAllEvents();
    void selectEventInSelectedPhases();
    void selectEventWithString();
    void doGroupedAction();


private:
    QToolBar* mToolBar;

    QStackedWidget* mCentralStack;
    ProjectView* mProjectView;
    std::shared_ptr<Project> mProject;

    QUndoStack* mUndoStack;
    QUndoView* mUndoView;
    QDockWidget* mUndoDock;

    QString mLastPath;

    QMenu* mProjectMenu;
    QMenu* mEditMenu;
    QMenu* mMCMCMenu;
    QMenu* mViewMenu;
    QMenu* mHelpMenu;
    QMenu* mActionsMenu;
    QMenu* mLanguageMenu;

    QAction* mAppSettingsAction;
    QAction* mAboutAct;
    QAction* mAboutQtAct;

    QActionGroup* mLangGroup;
    QAction* mTranslateEnglishAct;
    QAction* mTranslateFrenchAct;

    QAction* mNewProjectAction;
    QAction* mOpenProjectAction;
    QAction* mInsertProjectAction;
    QAction* mCloseProjectAction;

    QAction* mProjectSaveAction;
    QAction* mProjectSaveAsAction;

    QAction* mMCMCSettingsAction;
    QAction* mRunAction;
    // QAction* mResetMCMCAction;
    
    SwitchAction* mCurveAction;

    QActionGroup* mViewGroup;
    QAction* mViewModelAction;
    QAction* mViewResultsAction;
    QAction* mViewLogAction;

    QAction* mUndoAction;
    QAction* mRedoAction;
    QAction* mUndoViewAction;
    QAction* mSelectAllAction;

    QAction* mSelectEventsAction;
    QAction* mSelectEventsNameAction;
    QAction* mRescaleCurveAction;

    QAction* mEventsColorAction;
#ifndef FIXEDPRIOR
    QAction* mEventsMethodAction;
    QAction* mDatesMethodAction;
#endif
    QList<QAction*> mDatesActions;

    QAction* mHelpAction;
    QAction* mManualAction;
    QAction* mWebsiteAction;

    QPushButton* testButton;
private:
    Q_DISABLE_COPY(MainWindow)
};



#endif
