#ifndef RefCurve_H
#define RefCurve_H

#include <QString>
#include <QMap>


class RefCurve
{
public:
    RefCurve();
    virtual ~RefCurve();
    
public:
    QString mName;
    
    QMap<double, double> mDataMean;
    QMap<double, double> mDataError;
    QMap<double, double> mDataSup;
    QMap<double, double> mDataInf;
    
    double mTmin;
    double mTmax;
    
    double mDataMeanMin;
    double mDataMeanMax;
    
    double mDataErrorMin;
    double mDataErrorMax;
    
    double mDataSupMin;
    double mDataSupMax;
    
    double mDataInfMin;
    double mDataInfMax;
};

#endif

