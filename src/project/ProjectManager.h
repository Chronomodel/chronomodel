#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <QString>
#include <QUndoStack>
#include "Settings.h"

class Project;


class ProjectManager
{
public:
    static Project* newProject(bool askToSave);
    static bool saveProject();
    static void deleteProject();
    static Project* getProject();
    
    static QString getCurrentPath();
    static void setCurrentPath(const QString& path);

    static QUndoStack& getUndoStack();
    
    static Settings& getSettings();
    static void setSettings(const Settings& settings);
    
    static void readSettings();
    static void writeSettings();

private:
    static Project* mProject;
    static QString mLastPath;
    static QUndoStack mStack;
    
    static Settings mSettings;

private:
    ProjectManager(){}
    ~ProjectManager(){}
    
    Q_DISABLE_COPY(ProjectManager)
};

#endif
