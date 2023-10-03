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

#ifndef CALIBRATIONCURVE_H
#define CALIBRATIONCURVE_H

#include <QString>
#include <QMap>

#include "PluginAbstract.h"

class CalibrationCurve
{
public:
    QString mName;
    QString mDescription;

    QString mPluginId;
    PluginAbstract* mPlugin;

    QVector<double> mRepartition;
    QVector<double> mVector;
    QMap<double, double> mMap; // c'est la même chose que mVector, mais dans une QMap. Pour faciliter l'accés

    double mTmin;
    double mTmax;
    double mStep;
public:
    /** Default constructor */
    CalibrationCurve();

    /** Copy constructor */
    CalibrationCurve(const CalibrationCurve& other);

    /** Move constructor */
    CalibrationCurve (CalibrationCurve&& other) noexcept : /* noexcept needed to enable optimizations in containers */
        mName (other.mName),
        mDescription (other.mDescription),

        mPluginId (other.mPluginId),

        mPlugin (other.mPlugin),
        mRepartition(other.mRepartition),
        mVector (other.mVector),
        mMap (other.mMap),
        mTmin (other.mTmin),
        mTmax (other.mTmax),
        mStep (other.mStep)
   {
        other.mName = nullptr;
        other.mDescription = nullptr;
        other.mRepartition.clear();
        other.mVector.clear();
        other.mMap.clear();

        other.mPluginId = nullptr;
        other.mPlugin = nullptr;
    }

    /** Destructor */
    ~CalibrationCurve() noexcept;

    /** Copy assignment operator */
    CalibrationCurve& operator= (const CalibrationCurve& other)
    {
        CalibrationCurve tmp(other);        // re-use copy-constructor
        *this = std::move(tmp);             // re-use move-assignment
        return *this;
    }

    /** Move assignment operator */
    CalibrationCurve& operator= (CalibrationCurve&& other) noexcept
    {
        mName = other.mName;
        mPluginId = other.mPluginId;
        mPlugin = other.mPlugin;

        mDescription = other.mDescription;

        mRepartition.resize(other.mRepartition.size());
        std::copy(other.mRepartition.begin(), other.mRepartition.end(), mRepartition.begin());

        mVector.resize(other.mVector.size());
        std::copy(other.mVector.begin(),other.mVector.end(), mVector.begin());
        
        mMap = std::move(other.mMap);

        mTmin = other.mTmin;
        mTmax = other.mTmax;
        mStep = other.mStep;

        other.mName = nullptr;
        other.mDescription = nullptr;

        other.mRepartition.clear();
        other.mVector.clear();
        other.mMap.clear();

        other.mPluginId = nullptr;
        other.mPlugin = nullptr;
        
        return *this;
    }

    double interpolate(double t) const {
        // We need at least two points to interpolate
        if (mVector.size() < 2 || t <= mTmin) {
            return mVector.first();

        } else if (t >= mTmax) {
            return mVector.last();
        }

        const double prop = (t - mTmin) / (mTmax - mTmin);
        const double idx = prop * (mVector.size() - 1); // tricky : if (tmax - tmin) = 2000, then calib size is 2001 !
        const int idxUnder = (int)floor(idx);
        const int idxUpper = (int)ceil(idx);//idxUnder + 1;

        if (idxUnder == idxUpper) {
            return mVector[idxUnder];

        } else if (mVector[idxUnder] != 0. && mVector[idxUpper] != 0.) {
            // Important for gate: no interpolation around gates

            return std::lerp(mVector[idxUnder], mVector[idxUpper], (idx - idxUnder) / (idxUpper - idxUnder));
            //return interpolate( idx, (double)idxUnder, (double)idxUpper, mVector[idxUnder], mVector[idxUpper]);

        } else {
            return 0.;
        }
    }
    double repartition_interpolate(double t) const {
        // We need at least two points to interpolate
        if (mRepartition.size() < 2 || t <= mTmin) {
            return mRepartition.first();

        } else if (t >= mTmax) {
            return mRepartition.last();
        }

        const double prop = (t - mTmin) / (mTmax - mTmin);
        const double idx = prop * (mRepartition.size() - 1); // tricky : if (tmax - tmin) = 2000, then calib size is 2001 !
        const int idxUnder = (int)floor(idx);
        const int idxUpper = (int)ceil(idx);//idxUnder + 1;

        if (idxUnder == idxUpper) {
            return mRepartition[idxUnder];

        } else if (mRepartition[idxUnder] != 0. && mRepartition[idxUpper] != 0.) {
            // Important for gate: no interpolation around gates
#ifdef DEBUG
            if (idxUnder< 0 || idxUpper>=mRepartition.size())
                qDebug()<<"[repartition_interpolate] idxUnder<= 0 || idxUpper>=mRepartition.size()";
#endif
            return std::lerp(mRepartition[idxUnder], mRepartition[idxUpper], (idx - idxUnder) / (idxUpper - idxUnder));
            //return interpolate( idx, (double)idxUnder, (double)idxUpper, mVector[idxUnder], mVector[idxUpper]);

        } else {
            return 0.;
        }
    }
};

QDataStream &operator<<( QDataStream &stream, const CalibrationCurve &data );
QDataStream &operator>>( QDataStream &stream, CalibrationCurve &data );


#endif // CALIBRATIONCURVE_H
