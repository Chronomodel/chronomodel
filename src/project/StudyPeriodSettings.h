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

#ifndef STUDYPERIODSETTINGS_H
#define STUDYPERIODSETTINGS_H

#include <QString>
#include <QJsonObject>

#define STATE_SETTINGS_TMIN_DEF 0
#define STATE_SETTINGS_TMAX_DEF 0
#define STATE_SETTINGS_STEP_DEF 1
#define STATE_SETTINGS_STEP_FORCED_DEF false


class StudyPeriodSettings
{
public:
    StudyPeriodSettings();
    StudyPeriodSettings(const StudyPeriodSettings& s);
    StudyPeriodSettings& operator=(const StudyPeriodSettings& s);
    bool operator!=(StudyPeriodSettings const& s) const;
    bool operator==(StudyPeriodSettings const& s) const;
    bool isEqual(StudyPeriodSettings const& s) const;
    void copyFrom(const StudyPeriodSettings& s);
    virtual ~StudyPeriodSettings();

    static StudyPeriodSettings fromJson(const QJsonObject& json);
    QJsonObject toJson() const;

    static double getStep(const double tmin, const double tmax);
    double step() const;

    double getTminFormated() const;
    double getTmaxFormated() const ;

public:
    double mTmin;
    double mTmax;
    double mStep;
    bool mStepForced;
};

#endif
