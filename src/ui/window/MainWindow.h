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
    MainWindow(QWidget* aParent = 0);
    ~MainWindow();
    
    Project* getProject();
    QJsonObject getState() const;
    AppSettings getAppSettings() const;
    QUndoStack* getUndoStack();
    QString getCurrentPath() const;
    void setCurrentPath(const QString& path);
    
    void resetInterface();
    void activateInterface(bool activate);
    void setRunEnabled(bool enabled);
    void setLogEnabled(bool enabled);
    void setResultsEnabled(bool enabled);
    QString getNameProject() const;

protected:
    void closeEvent(QCloseEvent* e) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent* e) Q_DECL_OVERRIDE;
    void changeEvent(QEvent* event) Q_DECL_OVERRIDE;

    void connectProject();
    void disconnectProject();

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
    void setAppSettings(const AppSettings& s);
    void openManual();
    void openWebsite();
    void showHelp(bool);
    void setLanguage(QAction* action);
    void mcmcFinished(Model*);
    void noResult();
    void updateProject();
    
    void changeEventsColor();
    void changeEventsMethod();
    void changeDatesMethod();
    void selectedEventInSelectedPhases();
    void doGroupedAction();
    
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
    QMenu* mPluginsMenu;
    QMenu* mLanguageMenu;

    QAction* mAppSettingsAction;
    QAction* mAboutAct;
    QAction* mAboutQtAct;
    
    QActionGroup* mLangGroup;
    QAction* mTranslateEnglishAct;
    QAction* mTranslateFrenchAct;
    
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
    
    QAction* mSelectEventsAction;
    QAction* mEventsColorAction;
    QAction* mEventsMethodAction;
    QAction* mDatesMethodAction;
    QList<QAction*> mDatesActions;

    
    QAction* mHelpAction;
    QAction* mManualAction;
    QAction* mWebsiteAction;
    
private:
    Q_DISABLE_COPY(MainWindow);
};

#endif
