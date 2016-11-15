#ifndef CALIBRATIONCURVE_H
#define CALIBRATIONCURVE_H

#include <QString>
#include <QMap>
#include <MCMCSettings.h>
#include "PluginAbstract.h"


class CalibrationCurve
{
public:
    CalibrationCurve();
    CalibrationCurve(const CalibrationCurve& other);
    virtual ~CalibrationCurve();
    CalibrationCurve& operator=(const CalibrationCurve& other);

    enum Method{
        eFromRef = 0,
        eFromMCMC = 1,
    };

    QString mName;
    QString mDescription;


    Method mMethod;

    // Parameter refere to the Method
    MCMCSettings mMCMCSetting;
    //RefCurve mRefCurve;
    //QJsonObject mData;
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
