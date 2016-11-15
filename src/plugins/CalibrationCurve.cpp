#include "CalibrationCurve.h"

CalibrationCurve::CalibrationCurve():
mName(QString("unkown")),
mDescription(QString("undefined")),
mMethod(CalibrationCurve::Method::eFromRef)
{
    // Parameter refere to the Method
    mMCMCSetting = MCMCSettings();
    //mRefCurve();
    //mData = QJsonObject();
    mPlugin = 0;

    mRepartition = QVector< double>();
    mCurve = QVector< double>();
    mTmin = -INFINITY;
    mTmax = +INFINITY;
    mStep = 1.;
}
CalibrationCurve::CalibrationCurve(const CalibrationCurve& other)
{
    mName = other.mName;
    mDescription = other.mDescription;
    //mData = other.mData;
    mMethod = other.mMethod;
    mRepartition.resize(other.mRepartition.size());
    std::copy(other.mRepartition.begin(), other.mRepartition.end(), mRepartition.begin());
    mCurve .resize(other.mCurve.size());
    std::copy(other.mCurve.begin(),other.mCurve.end(), mCurve.begin());
    mTmin = other.mTmin;
    mTmax = other.mTmax;
    mStep = other.mStep;

}
CalibrationCurve::~CalibrationCurve()
{
    mRepartition.clear();
}

QDataStream &operator<<( QDataStream &stream, const CalibrationCurve &data )
{
    stream << data.mName;
    stream << data.mDescription;
    //stream << data.mData;

    switch (data.mMethod) {
       case CalibrationCurve::eFromRef : stream << (quint8)(0);
        break;
       case CalibrationCurve::eFromMCMC : stream << (quint8)(1);
          break;
    };

     stream << data.mRepartition;
     stream << data.mCurve;
     stream << data.mTmin;
     stream << data.mTmax;
     stream << data.mStep;

    return stream;
}

QDataStream &operator>>( QDataStream &stream, CalibrationCurve &data )
{
    //QJsonObject tmpJSON;

    stream >> data.mName;
    stream >> data.mDescription;
    //stream >> tmpJSON;

    //data.mData = tmpJSON;

    qint8 tmp8;
    stream >> tmp8;
    switch ((int) tmp8) {
      case 0 : data.mMethod = CalibrationCurve::eFromRef;
       break;
      case 1 : data.mMethod = CalibrationCurve::eFromMCMC;
         break;
    };

    stream >> data.mRepartition;
    stream >> data.mCurve;
    stream >> data.mTmin;
    stream >> data.mTmax;
    stream >> data.mStep;

    return stream;

}
CalibrationCurve & CalibrationCurve::operator=(const CalibrationCurve& other)
{

    mName = other.mName;
    mDescription = other.mDescription;
    //mData = other.mData;
    mMethod = other.mMethod;
    mRepartition.resize(other.mRepartition.size());
    std::copy(other.mRepartition.begin(), other.mRepartition.end(), mRepartition.begin());
    mCurve .resize(mCurve.size());
    std::copy(other.mCurve.begin(), other.mCurve.end(), mCurve.begin());
    mTmin = other.mTmin;
    mTmax = other.mTmax;
    mStep = other.mStep;

    return *this;
}
