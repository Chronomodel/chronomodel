#include "ProjectManager.h"
#include "Project.h"
#include <QDir>
#include <QSettings>


Project* ProjectManager::mProject = 0;
QString ProjectManager::mLastPath = QDir::homePath();
QUndoStack ProjectManager::mStack;


Project* ProjectManager::newProject()
{
    deleteProject();
    mProject = new Project();
    mStack.clear();
    mStack.setUndoLimit(10);
    return mProject;
}

bool ProjectManager::saveProject()
{
    if(mProject)
    {
        return mProject->askToSave();
    }
    return true;
}

void ProjectManager::deleteProject()
{
    if(mProject)
    {
        delete mProject;
        mProject = 0;
    }
}

Project* ProjectManager::getProject()
{
    return mProject;
}

QString ProjectManager::getCurrentPath()
{
    return mLastPath;
}

void ProjectManager::setCurrentPath(const QString& path)
{
    mLastPath = path;
}

QUndoStack& ProjectManager::getUndoStack()
{
    return mStack;
}

void ProjectManager::writeSettings()
{
    QSettings settings;
    settings.beginGroup("ProjectManager");
    settings.setValue("last_project_dir", mProject ? mProject->mProjectFileDir : "");
    settings.setValue("last_project_filename", mProject ? mProject->mProjectFileName : "");
    settings.endGroup();
}

void ProjectManager::readSettings()
{
    QSettings settings;
    settings.beginGroup("ProjectManager");
    QString dir = settings.value("last_project_dir", "").toString();
    QString filename = settings.value("last_project_filename", "").toString();
    if(!mProject)
        newProject();
    mProject->load(dir + "/" + filename);
    settings.endGroup();
}

