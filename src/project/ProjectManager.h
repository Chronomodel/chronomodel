#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <QString>
#include <QUndoStack>

class Project;


class ProjectManager
{
public:
    static Project* newProject();
    static bool saveProject();
    static void deleteProject();
    static Project* getProject();
    
    static QString getCurrentPath();
    static void setCurrentPath(const QString& path);

    static QUndoStack& getUndoStack();
    
    static void readSettings();
    static void writeSettings();

private:
    static Project* mProject;
    static QString mLastPath;
    static QUndoStack mStack;

private:
    ProjectManager(){}
    ~ProjectManager(){}
    
    Q_DISABLE_COPY(ProjectManager)
};

#endif
