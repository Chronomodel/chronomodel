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

#ifndef REFCURVE_H
#define REFCURVE_H

#include <QString>
#include <QMap>

class RefCurve
{
public:
    QString mName;

    QMap<double, double> mDataMean;
    QMap<double, double> mDataError;
    QMap<double, double> mDataSup;
    QMap<double, double> mDataInf;

    double mTmin;
    double mTmax;

    double mMinStep;

    double mDataMeanMin;
    double mDataMeanMax;

    double mDataErrorMin;
    double mDataErrorMax;

    double mDataSupMin;
    double mDataSupMax;

    double mDataInfMin;
    double mDataInfMax;

public:
    RefCurve();
    virtual ~RefCurve();
    double interpolate_mean(const double t) const;
    double interpolate_error(const double t) const;


};

#endif
