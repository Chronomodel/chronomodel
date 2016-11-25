#ifndef CALIBRATIONCURVE_H
#define CALIBRATIONCURVE_H

#include <QString>
#include <QMap>
#include <MCMCSettings.h>
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
        mMethod (other.mMethod),
        mMCMCSetting (other.mMCMCSetting),
        mPluginId (other.mPluginId),

        mPlugin (other.mPlugin),
        mRepartition(other.mRepartition),
        mCurve (other.mCurve),
        mTmin (other.mTmin),
        mTmax (other.mTmax),
        mStep (other.mStep)
   {
        other.mName = nullptr;
        other.mDescription = nullptr;
        //other.mMethod=0;
        other.mRepartition.clear();
        other.mCurve.clear();
        /*other.mTmin = nullptr;
        other.mTmax = nullptr;
        other.mStep = nullptr;*/

        //other.mMCMCSetting;
        other.mPluginId = nullptr;
        other.mPlugin =nullptr;

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
        mMCMCSetting =other.mMCMCSetting;
        mPluginId = other.mPluginId;
        mPlugin = other.mPlugin;

        mDescription = other.mDescription;
        mMethod = other.mMethod;
        mRepartition.resize(other.mRepartition.size());
        std::copy(other.mRepartition.begin(), other.mRepartition.end(), mRepartition.begin());
        mCurve .resize(other.mCurve.size());
        std::copy(other.mCurve.begin(),other.mCurve.end(), mCurve.begin());
        mTmin = other.mTmin;
        mTmax = other.mTmax;
        mStep = other.mStep;

        other.mName = nullptr;
        other.mDescription = nullptr;
        //other.mMethod=0;
        other.mRepartition.clear();
        other.mCurve.clear();
        /*other.mTmin = nullptr;
        other.mTmax = nullptr;
        other.mStep = nullptr;

        other.mMCMCSetting;*/
        other.mPluginId = nullptr;
        other.mPlugin =nullptr;
        return *this;
    }

    enum Method{
        eFromRef = 0,
        eFromMCMC = 1,
    };

    QString mName;
    QString mDescription;

    Method mMethod;

    // Parameter refere to the Method
    MCMCSettings mMCMCSetting;

    QString mPluginId;
    PluginAbstract* mPlugin;

    QVector<double> mRepartition;
    QVector<double> mCurve;

    double mTmin;
    double mTmax;
    double mStep;

};

QDataStream &operator<<( QDataStream &stream, const CalibrationCurve &data );
QDataStream &operator>>( QDataStream &stream, CalibrationCurve &data );


#endif // CALIBRATIONCURVE_H
