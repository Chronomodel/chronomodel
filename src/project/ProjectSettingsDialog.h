#ifndef PROJECTSETTINGSDIALOG_H
#define PROJECTSETTINGSDIALOG_H

#include <QDialog>
#include "ProjectSettings.h"

class LineEdit;


class ProjectSettingsDialog: public QDialog
{
    Q_OBJECT
public:
    ProjectSettingsDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~ProjectSettingsDialog();

    void setSettings(const ProjectSettings& settings);
    ProjectSettings getSettings();

private:
    LineEdit* mMinEdit;
    LineEdit* mMaxEdit;
    LineEdit* mStepEdit;
};

#endif
