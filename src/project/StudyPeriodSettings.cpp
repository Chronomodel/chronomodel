/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#include "StudyPeriodSettings.h"

#include "DateUtils.h"
//#include "QtCore/qjsonobject.h"
#include "StateKeys.h"

#include <QObject>
#include <QVariant>

#include <cmath>

StudyPeriodSettings::StudyPeriodSettings():
    mTmin(0.),
    mTmax(0.),
    mStep(1.),
    mStepForced(false)
{

}

StudyPeriodSettings::StudyPeriodSettings(const StudyPeriodSettings &s)
{
    copyFrom(s);
}

StudyPeriodSettings& StudyPeriodSettings::operator=(const StudyPeriodSettings &s)
{
    mTmin = s.mTmin;
    mTmax = s.mTmax;
    mStep = s.mStep;
    mStepForced = s.mStepForced;
    return *this;
}

bool StudyPeriodSettings::operator!=(StudyPeriodSettings const &s) const
{
    return !isEqual(s);
}

bool StudyPeriodSettings::operator==( StudyPeriodSettings const &s) const
{
    return isEqual(s);
}

bool StudyPeriodSettings::isEqual(StudyPeriodSettings const &s) const
{
    if (s.mTmin != mTmin ||
       s.mTmax != mTmax ||
       s.mStep != mStep ||
       s.mStepForced != mStepForced)
            return false;
    else
        return true;
}

void StudyPeriodSettings::copyFrom(const StudyPeriodSettings &s)
{
    mTmin = s.mTmin;
    mTmax = s.mTmax;
    mStep = s.mStep;
    mStepForced = s.mStepForced;
}

StudyPeriodSettings::~StudyPeriodSettings()
{

}
/**
 * @brief StudyPeriodSettings::fromJson, here we fix tmin and tmax as integer value
 * @param json
 * @return
 */
StudyPeriodSettings StudyPeriodSettings::fromJson(const QJsonObject& json)
{
    StudyPeriodSettings settings;
    //settings.mTmin = json.contains(STATE_SETTINGS_TMIN) ? json.value(STATE_SETTINGS_TMIN).toInt() : STATE_SETTINGS_TMIN_DEF;
    //settings.mTmax = json.contains(STATE_SETTINGS_TMAX) ? json.value(STATE_SETTINGS_TMAX).toInt() : STATE_SETTINGS_TMAX_DEF;
    settings.mTmin = json.contains(STATE_SETTINGS_TMIN) ? json.value(STATE_SETTINGS_TMIN).toDouble() : STATE_SETTINGS_TMIN_DEF;
    settings.mTmax = json.contains(STATE_SETTINGS_TMAX) ? json.value(STATE_SETTINGS_TMAX).toDouble() : STATE_SETTINGS_TMAX_DEF;

    settings.mStep = json.contains(STATE_SETTINGS_STEP) ? json.value(STATE_SETTINGS_STEP).toDouble() : STATE_SETTINGS_STEP_DEF;
    settings.mStepForced = json.contains(STATE_SETTINGS_STEP_FORCED) ? json.value(STATE_SETTINGS_STEP_FORCED).toBool() : STATE_SETTINGS_STEP_FORCED_DEF;

    return settings;
}

QJsonObject StudyPeriodSettings::toJson() const
{
    QJsonObject settings;
    //settings[STATE_SETTINGS_TMIN] = int (floor(mTmin));
    //settings[STATE_SETTINGS_TMAX] = int (ceil(mTmax));
    settings[STATE_SETTINGS_TMIN] = mTmin;
    settings[STATE_SETTINGS_TMAX] = mTmax;
    settings[STATE_SETTINGS_STEP] = mStep;
    settings[STATE_SETTINGS_STEP_FORCED] = mStepForced;

    return settings;
}

double StudyPeriodSettings::getStep(const double tmin, const double tmax)
{
    const double diff (tmax - tmin);
    const double linearUntil (52000.);

    if (diff <= linearUntil)
        return 1.;

    else {
        const double maxPts (52000.); //must be upper than linearUntil
        const double lambda = - log((maxPts - linearUntil)/maxPts) / linearUntil;
        const double nbPts = maxPts * (1. - exp(-lambda * diff));

        return diff / nbPts;
    }
}

double StudyPeriodSettings::getTminFormated() const
{
   return qMin(DateUtils::convertToAppSettingsFormat(mTmin), DateUtils::convertToAppSettingsFormat(mTmax));
}

double StudyPeriodSettings::getTmaxFormated() const
{
    return qMax(DateUtils::convertToAppSettingsFormat(mTmin), DateUtils::convertToAppSettingsFormat(mTmax));
}
