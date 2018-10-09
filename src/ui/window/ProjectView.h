#ifndef ProjectView_H
#define ProjectView_H

#include <QWidget>
#include "MCMCLoopMain.h"
#include "AppSettings.h"
#include "Project.h"
#include "Tabs.h"

class QStackedWidget;
class QTextEdit;
class QTabWidget;

class ModelView;
class ResultsView;
class Event;
//class Project;


class ProjectView: public QWidget
{
    Q_OBJECT
public:
    ProjectView(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    ~ProjectView();

    void resizeEvent(QResizeEvent* e);

    bool mRefreshResults;

    void doProjectConnections(Project* project);
    void resetInterface();
    
    void readSettings();
    void writeSettings();
    void createProject();
   // void setFont(const QFont &font);
    void newPeriod();

    void updateMultiCalibration();

public slots:
    void updateProject();
    void showModel();
    void showResults();
    void showLogTab(const int &i);

    void changeDesign(bool refresh);
    void showLog();
    void showHelp(bool show);
    
    void applySettings(Model* model);
    void applyFilesSettings(Model* model);
    void initResults(Model*);
    void updateResults(Model*);
    void updateResultsLog(const QString& log);
    void setAppSettings();

    
private:
    QStackedWidget* mStack;
    ModelView* mModelView;
    ResultsView* mResultsView;

    Tabs* mLogTabs;
    QTextEdit* mLogModelEdit;
    QTextEdit* mLogMCMCEdit;
    QTextEdit* mLogResultsEdit;
};

#endif
