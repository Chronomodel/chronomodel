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
//        mMCMCSetting =other.mMCMCSetting;
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

/*    enum Method{
        eFromRef = 0,
        eFromMCMC = 1,
    };
*/
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
};

QDataStream &operator<<( QDataStream &stream, const CalibrationCurve &data );
QDataStream &operator>>( QDataStream &stream, CalibrationCurve &data );


#endif // CALIBRATIONCURVE_H
