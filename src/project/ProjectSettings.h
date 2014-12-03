#ifndef PROJECTSETTINGS_H
#define PROJECTSETTINGS_H

#include <QString>
#include <QJsonObject>

#define STATE_SETTINGS_TMIN "tmin"
#define STATE_SETTINGS_TMAX "tmax"
#define STATE_SETTINGS_STEP "step"


class ProjectSettings
{
public:
    ProjectSettings();
    ProjectSettings(const ProjectSettings& s);
    ProjectSettings& operator=(const ProjectSettings& s);
    bool operator!=(const ProjectSettings& s);
    bool operator==(const ProjectSettings& s);
    bool isEqual(const ProjectSettings& s);
    void copyFrom(const ProjectSettings& s);
    virtual ~ProjectSettings();
    
    static ProjectSettings fromJson(const QJsonObject& json);
    QJsonObject toJson() const;

public:
    int mTmin;
    int mTmax;
    int mStep;
};

#endif
