#ifndef ProjectView_H
#define ProjectView_H

#include <QWidget>
#include "MCMCLoopMain.h"

class QStackedWidget;
class QTextEdit;
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
    
    void doProjectConnections(Project* project);
    void resetInterface();
    
    void readSettings();
    void writeSettings();
    
public slots:
    void updateProject();
    void showModel();
    void showResults();
    void showLog();
    void showHelp(bool show);
    void updateLog(MCMCLoopMain& loop);
    
private:
    QStackedWidget* mStack;
    ModelView* mModelView;
    ResultsView* mResultsView;
    QTextEdit* mLogEdit;
};

#endif
