/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#include "StudyPeriodSettings.h"

#include "DateUtils.h"
//#include "QtCore/qjsonobject.h"
#include "StateKeys.h"

#include <QObject>
#include <QVariant>

#include <cmath>

StudyPeriodSettings::StudyPeriodSettings():
    mTmin(0.0),
    mTmax(0.0),
    mStep(1.0),
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

    settings.mTmin = json.contains(STATE_SETTINGS_TMIN) ? json.value(STATE_SETTINGS_TMIN).toDouble() : STATE_SETTINGS_TMIN_DEF;
    settings.mTmax = json.contains(STATE_SETTINGS_TMAX) ? json.value(STATE_SETTINGS_TMAX).toDouble() : STATE_SETTINGS_TMAX_DEF;

    settings.mStep = json.contains(STATE_SETTINGS_STEP) ? json.value(STATE_SETTINGS_STEP).toDouble() : STATE_SETTINGS_STEP_DEF;
    settings.mStepForced = json.contains(STATE_SETTINGS_STEP_FORCED) ? json.value(STATE_SETTINGS_STEP_FORCED).toBool() : STATE_SETTINGS_STEP_FORCED_DEF;

    return settings;
}

QJsonObject StudyPeriodSettings::toJson() const
{
    QJsonObject settings;

    settings[STATE_SETTINGS_TMIN] = mTmin;
    settings[STATE_SETTINGS_TMAX] = mTmax;
    settings[STATE_SETTINGS_STEP] = mStep;
    settings[STATE_SETTINGS_STEP_FORCED] = mStepForced;

    return settings;
}

/* old code
double StudyPeriodSettings::getStep(const double tmin, const double tmax)
{
    const double diff (tmax - tmin);
    constexpr double linearUntil  = 52000.0;

    if (diff <= linearUntil)
        return 1.0;

    else {
        constexpr double maxPts = 52000.0; //must be upper than linearUntil
        const double lambda = - log((maxPts - linearUntil)/maxPts) / linearUntil;
        const double nbPts = maxPts * (1.0 - exp(-lambda * diff));

        return diff / nbPts;
    }
}
*/

double StudyPeriodSettings::getStep(double tmin, double tmax)
{
    double diff = tmax - tmin;

    if (diff <= 0)
        return 0.0;

    double stepBrut = diff / 1000.0;

    // --- Calcul de la puissance de 10 inférieure à stepBrut ---
    double power = 1.0;
    if (stepBrut >= 1.0) {
        while (power * 10.0 <= stepBrut) power *= 10.0;
    } else {
        while (power * 0.1 >= stepBrut) power *= 0.1;
    }

    // --- Normalisation ---
    double scaled = stepBrut / power;
    double rounded;
    if      (scaled <= 1.0) rounded = 1.0;
    else if (scaled <= 2.0) rounded = 2.0;
    else if (scaled <= 5.0) rounded = 5.0;
    else                    rounded = 10.0;

    return rounded * power;
}

double StudyPeriodSettings::step() const
{
    if (mStepForced) {
        return mStep;
    } else
        return getStep(mTmin, mTmax);
}

double StudyPeriodSettings::getTminFormated() const
{
   return std::min(DateUtils::convertToAppSettingsFormat(mTmin), DateUtils::convertToAppSettingsFormat(mTmax));
}

double StudyPeriodSettings::getTmaxFormated() const
{
    return std::max(DateUtils::convertToAppSettingsFormat(mTmin), DateUtils::convertToAppSettingsFormat(mTmax));
}
