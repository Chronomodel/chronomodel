#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Singleton.h"
#include "AppSettings.h"

class QMenu;
class QAction;
class QActionGroup;
class QStackedWidget;
class QUndoStack;
class QUndoView;
class QUndoView;
class QDockWidget;
class ProjectView;
class Project;
class Event;


class MainWindow : public QMainWindow, public Singleton<MainWindow>
{
    Q_OBJECT
    friend class Singleton<MainWindow>;
    
public:
    MainWindow(QWidget* aParent = 0);
    ~MainWindow();
    
    Project* getProject();
    QJsonObject getState();
    AppSettings getAppSettings() const;
    QUndoStack* getUndoStack();
    QString getCurrentPath();
    void setCurrentPath(const QString& path);
    
    void resetInterface();
    void activateInterface(bool activate);
    void setRunEnabled(bool enabled);
    void setLogEnabled(bool enabled);

protected:
    void closeEvent(QCloseEvent* e);
    void keyPressEvent(QKeyEvent* e);

public:
    void readSettings(const QString& defaultFilePath);
    void writeSettings();
    void updateWindowTitle();

private:
    void createActions();
    void createMenus();
    void createToolBars();

public slots:
    void newProject();
    void openProject();
    void closeProject();
    void saveProject();
    void saveProjectAs();
    void about();
    void appSettings();
    void openManual();
    void openWebsite();
    void showHelp(bool);
    void mcmcFinished();
    
private:
    QStackedWidget* mCentralStack;
    ProjectView* mProjectView;
    Project* mProject;
    QUndoStack* mUndoStack;
    QUndoView* mUndoView;
    QDockWidget* mUndoDock;
    
    AppSettings mAppSettings;
    QString mLastPath;
    
    QMenu* mProjectMenu;
    QMenu* mEditMenu;
    QMenu* mMCMCMenu;
    QMenu* mViewMenu;
    QMenu* mHelpMenu;

    QAction* mAppSettingsAction;
    QAction* mAboutAct;
    QAction* mAboutQtAct;
    
    QAction* mNewProjectAction;
    QAction* mOpenProjectAction;
    QAction* mCloseProjectAction;
    
    QAction* mProjectSaveAction;
    QAction* mProjectSaveAsAction;
    QAction* mProjectExportAction;
    
    QAction* mMCMCSettingsAction;
    QAction* mRunAction;
    QAction* mResetMCMCAction;
    
    QActionGroup* mViewGroup;
    QAction* mViewModelAction;
    QAction* mViewResultsAction;
    QAction* mViewLogAction;
    
    QAction* mUndoAction;
    QAction* mRedoAction;
    QAction* mUndoViewAction;
    
    QAction* mHelpAction;
    QAction* mManualAction;
    QAction* mWebsiteAction;
    
private:
    Q_DISABLE_COPY(MainWindow);
};

#endif
