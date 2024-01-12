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

#include "CalibrationCurve.h"
#include "PluginManager.h"

CalibrationCurve::CalibrationCurve():
mName(QString("unkown")),
mDescription(QString("undefined"))
{
    mPluginId = "";
    mPlugin = nullptr;
    mRepartition = QVector< double>();
    mVector = QVector< double>();
    mMap = QMap<double, double>();
    mTmin = -INFINITY;
    mTmax = +INFINITY;
    mStep = 1.;
}

CalibrationCurve::CalibrationCurve(const CalibrationCurve& other)
{
    mName = other.mName;
    mPluginId = other.mPluginId;
    mPlugin = other.mPlugin;

    mDescription = other.mDescription;

    mRepartition.resize(other.mRepartition.size());
    std::copy(other.mRepartition.begin(), other.mRepartition.end(), mRepartition.begin());

    mVector.resize(other.mVector.size());
    std::copy(other.mVector.begin(), other.mVector.end(), mVector.begin());

    mMap = other.mMap;

    mTmin = other.mTmin;
    mTmax = other.mTmax;
    mStep = other.mStep;

}
CalibrationCurve::~CalibrationCurve() noexcept
{
    mRepartition.clear();
    mVector.clear();
    mMap.clear();
    mPluginId.clear();
    mPlugin = nullptr;
}


QDataStream &operator<<( QDataStream &stream, const CalibrationCurve &data )
{
    stream << data.mName;
    stream << data.mDescription;

    stream << data.mRepartition;
    stream << data.mVector;
    stream << data.mTmin;
    stream << data.mTmax;
    stream << data.mStep;
    stream << data.mPluginId;

    return stream;

}

QDataStream &operator>>( QDataStream &stream, CalibrationCurve &data )
{
    stream >> data.mName;
    stream >> data.mDescription;

    stream >> data.mRepartition;
    stream >> data.mVector;
    stream >> data.mTmin;
    stream >> data.mTmax;
    stream >> data.mStep;

    stream >> data.mPluginId;

    data.mMap = vector_to_map(data.mVector, data.mTmin, data.mTmax, data.mStep);

    data.mPlugin = PluginManager::getPluginFromId(data.mPluginId);
    if (data.mPlugin == nullptr && data.mDescription.isEmpty())
        throw QObject::tr("Calibration plugin could not be loaded : invalid plugin : %1").arg(data.mPluginId);

    return stream;

}
