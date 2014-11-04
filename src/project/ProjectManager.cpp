#include "ProjectManager.h"
#include "Project.h"
#include "MainWindow.h"
#include <QDir>
#include <QSettings>


Project* ProjectManager::mProject = 0;
QString ProjectManager::mLastPath = QDir::homePath();
QUndoStack ProjectManager::mStack;
Settings ProjectManager::mSettings = Settings();


Project* ProjectManager::newProject(bool askToSave)
{
    deleteProject();
    mProject = new Project();
    mStack.clear();
    mStack.setUndoLimit(10);
    mProject->setAppSettings(mSettings);
    if(askToSave)
    {
        if(!mProject->saveAs())
        {
            delete mProject;
            mProject = 0;
        }
    }
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

Settings& ProjectManager::getSettings()
{
    return mSettings;
}

void ProjectManager::setSettings(const Settings& settings)
{
    mSettings = settings;
    if(mProject)
    {
        mProject->setAppSettings(mSettings);
    }
}

void ProjectManager::writeSettings()
{
    QSettings settings;
    settings.beginGroup("ProjectManager");
    settings.setValue("last_project_dir", mProject ? mProject->mProjectFileDir : "");
    settings.setValue("last_project_filename", mProject ? mProject->mProjectFileName : "");
    
    settings.beginGroup("settings");
    settings.setValue("auto_save", mSettings.mAutoSave);
    settings.setValue("auto_save_delay", mSettings.mAutoSaveDelay);
    settings.setValue("show_help", mSettings.mShowHelp);
    settings.endGroup();
    
    settings.endGroup();
}

void ProjectManager::readSettings()
{
    QSettings settings;
    settings.beginGroup("ProjectManager");
    
    QString dir = settings.value("last_project_dir", "").toString();
    QString filename = settings.value("last_project_filename", "").toString();
    
    settings.beginGroup("settings");
    mSettings.mAutoSave = settings.value("auto_save", true).toBool();
    mSettings.mAutoSaveDelay = settings.value("auto_save_delay", 300).toInt();
    mSettings.mShowHelp = settings.value("show_help", true).toInt();
    settings.endGroup();
    
    QString path = dir + "/" + filename;
    QFile file(path);
    if(file.exists())
    {
        if(!mProject)
            newProject(false);
        
        // ProjectView must be created before loading the project :
        MainWindow::getInstance()->setProject(mProject);
        
        mProject->load(path);
    }
    
    settings.endGroup();
}

