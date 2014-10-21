#ifndef ProjectView_H
#define ProjectView_H

#include <QWidget>

class QStackedWidget;
class ModelView;
class ResultsView;
class Event;


class ProjectView: public QWidget
{
    Q_OBJECT
public:
    ProjectView(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~ProjectView();
    
public slots:
    void updateProject();
    void showModel();
    void showResults();
    
private:
    QStackedWidget* mStack;
    ModelView* mModelView;
    ResultsView* mResultsView;
};

#endif
