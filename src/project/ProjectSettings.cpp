#include "ProjectSettings.h"
#include "DateUtils.h"
#include <QObject>
#include <QVariant>
#include <cmath>

ProjectSettings::ProjectSettings():
mTmin(0.),
mTmax(0.),
mStep(1.),
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
    if (s.mTmin != mTmin ||
       s.mTmax != mTmax ||
       s.mStep != mStep ||
       s.mStepForced != mStepForced)
            return false;
    else
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
/**
 * @brief ProjectSettings::fromJson, here we fix tmin and tmax as integer value
 * @param json
 * @return
 */
ProjectSettings ProjectSettings::fromJson(const QJsonObject& json)
{
    ProjectSettings settings;
    settings.mTmin = json.contains(STATE_SETTINGS_TMIN) ? json.value(STATE_SETTINGS_TMIN).toInt() : STATE_SETTINGS_TMIN_DEF;
    settings.mTmax = json.contains(STATE_SETTINGS_TMAX) ? json.value(STATE_SETTINGS_TMAX).toInt() : STATE_SETTINGS_TMAX_DEF;
    settings.mStep = json.contains(STATE_SETTINGS_STEP) ? json.value(STATE_SETTINGS_STEP).toDouble() : STATE_SETTINGS_STEP_DEF;
    settings.mStepForced = json.contains(STATE_SETTINGS_STEP_FORCED) ? json.value(STATE_SETTINGS_STEP_FORCED).toBool() : STATE_SETTINGS_STEP_FORCED_DEF;

    return settings;
}

QJsonObject ProjectSettings::toJson() const
{
    QJsonObject settings;
    settings[STATE_SETTINGS_TMIN] = (int) floor(mTmin);
    settings[STATE_SETTINGS_TMAX] = (int) ceil(mTmax);
    settings[STATE_SETTINGS_STEP] = mStep;
    settings[STATE_SETTINGS_STEP_FORCED] = mStepForced;

    return settings;
}

double ProjectSettings::getStep(const double tmin, const double tmax)
{
    const double diff = tmax - tmin;
    const double linearUntil (10000.);
    
    if (diff <= linearUntil)
        return 1.;

    else {
        const double maxPts (50000.);
        const double lambda = - log((maxPts - linearUntil)/maxPts) / linearUntil;
        const double nbPts = maxPts * (1. - exp(-lambda * diff));
        double step = diff / nbPts;
        return step;
    }
}

double ProjectSettings::getTminFormated() const
{
   return qMin(DateUtils::convertToAppSettingsFormat(mTmin), DateUtils::convertToAppSettingsFormat(mTmax));
}

double ProjectSettings::getTmaxFormated() const
{
    return qMax(DateUtils::convertToAppSettingsFormat(mTmin), DateUtils::convertToAppSettingsFormat(mTmax));
}
