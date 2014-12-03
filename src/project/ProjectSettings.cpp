#include "ProjectSettings.h"
#include <QObject>
#include <QVariant>


ProjectSettings::ProjectSettings():
mTmin(0),
mTmax(0),
mStep(1)
{

}

ProjectSettings::ProjectSettings(const ProjectSettings& s)
{
    copyFrom(s);
}

ProjectSettings& ProjectSettings::operator=(const ProjectSettings& s)
{
    copyFrom(s);
    return *this;
}

bool ProjectSettings::operator!=(const ProjectSettings& s)
{
    return !isEqual(s);
}

bool ProjectSettings::operator==(const ProjectSettings& s)
{
    return isEqual(s);
}

bool ProjectSettings::isEqual(const ProjectSettings& s)
{
    if(s.mTmin != this->mTmin ||
       s.mTmax != this->mTmax ||
       s.mStep != this->mStep)
        return false;
    return true;
}

void ProjectSettings::copyFrom(const ProjectSettings& s)
{
    mTmin = s.mTmin;
    mTmax = s.mTmax;
    mStep = s.mStep;
}

ProjectSettings::~ProjectSettings()
{

}

ProjectSettings ProjectSettings::fromJson(const QJsonObject& json)
{
    ProjectSettings settings;
    settings.mTmin = json[STATE_SETTINGS_TMIN].toInt();
    settings.mTmax = json[STATE_SETTINGS_TMAX].toInt();
    settings.mStep = json[STATE_SETTINGS_STEP].toInt();
    return settings;
}

QJsonObject ProjectSettings::toJson() const
{
    QJsonObject settings;
    settings[STATE_SETTINGS_TMIN] = mTmin;
    settings[STATE_SETTINGS_TMAX] = mTmax;
    settings[STATE_SETTINGS_STEP] = mStep;
    return settings;
}