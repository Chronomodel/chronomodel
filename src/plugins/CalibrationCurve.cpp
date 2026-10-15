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

#include "CalibrationCurve.h"
#include "QtUtilities.h"
#include "StdUtilities.h"

CalibrationCurve::CalibrationCurve():
    mDescription("undefined"),
    mPluginId(""),
    mRepartition(),
    mVector(),
    mMap(),
    mTmin(-INFINITY),
    mTmax(INFINITY),
    mStep(1.),
    _name("unkown curve")
{

}

CalibrationCurve::CalibrationCurve(const CalibrationCurve &other):
    mDescription(other.mDescription),
    mPluginId (other.mPluginId),
    mRepartition(other.mRepartition),
    mVector(other.mVector),
    mMap(other.mMap),
    mTmin(other.mTmin),
    mTmax(other.mTmax),
    mStep(other.mStep),
    _name (other._name)
{

}

CalibrationCurve::~CalibrationCurve() noexcept
{
    mRepartition.clear();
    mVector.clear();
    mMap.clear();
    mPluginId.clear();
}

QDataStream &operator<<( QDataStream &stream, const CalibrationCurve &data )
{
    stream << data.getQStringName();
    stream << QString::fromStdString(data.mDescription);

    //stream << data.mRepartition;
    save_container(stream, data.mRepartition);
    //stream << &data.mVector;
    save_container(stream, data.mVector);
    stream << data.mTmin;
    stream << data.mTmax;
    stream << data.mStep;
    stream << QString::fromStdString(data.mPluginId);

    return stream;
}

QDataStream &operator>>( QDataStream &stream, CalibrationCurve &data )
{
    QString str;
    stream >> str;
    data.setName(str);
    //stream >> data.mName;
    //stream >> data.mDescription;
    stream >> str;
    data.mDescription = str.toStdString();
    load_container(stream, data.mRepartition);
    //stream >> data.mRepartition;
    load_container(stream, data.mVector);
    //stream >> data.mVector;
    stream >> data.mTmin;
    stream >> data.mTmax;
    stream >> data.mStep;
    //stream >> data.mPluginId;
    stream>> str;
    data.mPluginId = str.toStdString();

    auto vecToMap = vector_to_map(data.mVector, data.mTmin, data.mTmax, data.mStep);
    data.mMap.swap(vecToMap);

    return stream;
}
