#include "ProjectSettings.h"
#include <QObject>
#include <QVariant>
#include <cmath>


ProjectSettings::ProjectSettings():
mTmin(0),
mTmax(0),
mStep(1),
mStepForced(false)
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
       s.mStep != this->mStep ||
       s.mStepForced != this->mStepForced)
        return false;
    return true;
}

void ProjectSettings::copyFrom(const ProjectSettings& s)
{
    mTmin = s.mTmin;
    mTmax = s.mTmax;
    mStep = s.mStep;
    mStepForced = s.mStepForced;
}

ProjectSettings::~ProjectSettings()
{

}

ProjectSettings ProjectSettings::fromJson(const QJsonObject& json)
{
    ProjectSettings settings;
    settings.mTmin = json.contains(STATE_SETTINGS_TMIN) ? json[STATE_SETTINGS_TMIN].toInt() : STATE_SETTINGS_TMIN_DEF;
    settings.mTmax = json.contains(STATE_SETTINGS_TMAX) ? json[STATE_SETTINGS_TMAX].toInt() : STATE_SETTINGS_TMAX_DEF;
    settings.mStep = json.contains(STATE_SETTINGS_STEP) ? json[STATE_SETTINGS_STEP].toInt() : STATE_SETTINGS_STEP_DEF;
    settings.mStepForced = json.contains(STATE_SETTINGS_STEP_FORCED) ? json[STATE_SETTINGS_STEP_FORCED].toBool() : STATE_SETTINGS_STEP_FORCED_DEF;
    
    return settings;
}

QJsonObject ProjectSettings::toJson() const
{
    QJsonObject settings;
    settings[STATE_SETTINGS_TMIN] = mTmin;
    settings[STATE_SETTINGS_TMAX] = mTmax;
    settings[STATE_SETTINGS_STEP] = mStep;
    settings[STATE_SETTINGS_STEP_FORCED] = mStepForced;
    return settings;
}

int ProjectSettings::getStep(float tmin, float tmax)
{
    float diff = tmax - tmin;
    if(diff <= 40000)
        return 1;
    else
    {
        float maxPts = 10000;
        float step = floorf(diff / maxPts);
        return (int)step;
    }
}
