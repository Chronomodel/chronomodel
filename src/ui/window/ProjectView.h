#ifndef ProjectView_H
#define ProjectView_H

#include <QWidget>
#include "MCMCLoopMain.h"

class QStackedWidget;
class QTextEdit;
class QTabWidget;

class ModelView;
class ResultsView;
class Event;
class Project;


class ProjectView: public QWidget
{
    Q_OBJECT
public:
    ProjectView(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~ProjectView();
    
    bool mRefreshResults;

    void doProjectConnections(Project* project);
    void resetInterface();
    
    void readSettings();
    void writeSettings();
    
public slots:
    void updateProject();
    void showModel();
    void showResults();

    void changeDesign(bool refresh);
    void showLog();
    void showHelp(bool show);
    
    void applySettings(Model* model, const AppSettings* appSet);

    void initResults(Model*, const AppSettings* appSet);
    void updateResults(Model*);
    void updateResultsLog(const QString& log);
    
private:
    QStackedWidget* mStack;
    ModelView* mModelView;
    ResultsView* mResultsView;
    QWidget* mLogView;
    QTabWidget* mLogTabs;
    QTextEdit* mLogModelEdit;
    QTextEdit* mLogMCMCEdit;
    QTextEdit* mLogResultsEdit;
};

#endif
