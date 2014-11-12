#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Singleton.h"

class QMenu;
class QAction;
class QActionGroup;
class QStackedWidget;
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

protected:
    void closeEvent(QCloseEvent* e);
    void keyPressEvent(QKeyEvent* e);

public:
    void readSettings();
    void writeSettings();
    void updateWindowTitle();

private:
    void createActions();
    void createMenus();
    void createToolBars();

public slots:
    void newProject();
    void openProject();
    void setProject(Project* project);
    bool closeProject();
    void saveProject();
    void saveProjectAs();
    void about();
    void appSettings();
    void openManual();
    void openWebsite();
    void showHelp(bool);
    
private:
    QStackedWidget* mCentralStack;
    QScopedPointer<ProjectView> mProjectView;
    //ProjectView* mProjectView;
    Project* mProject;
    QUndoView* mUndoView;
    QDockWidget* mUndoDock;
    
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
    
    QAction* mProjectSaveAction;
    QAction* mProjectSaveAsAction;
    QAction* mProjectCloseAction;
    QAction* mProjectExportAction;
    
    QAction* mMCMCSettingsAction;
    QAction* mRunAction;
    
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
